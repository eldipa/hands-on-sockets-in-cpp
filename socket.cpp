#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include "socket.h"
#include "resolver.h"
#include "liberror.h"

#include <stdexcept>

Socket::Socket(
        const char *hostname,
        const char *servname) {
    Resolver resolver(hostname, servname, false);

    int s = -1;
    int skt = -1;
    this->closed = true;

    /*
     * Por cada dirección obtenida tenemos que ver cual es realmente funcional.
     * `getaddrinfo` puede darnos direcciones IP validas pero que apuntan
     * a servidores que no están activos (`getaddrinfo` simplemente no
     * lo puede saber).
     *
     * Es responsabilidad nuestra probar cada una de ellas hasta encontrar
     * una que funcione.
     * */
    while (resolver.has_next()) {
        struct addrinfo *addr = resolver.next();

        /* Cerramos el socket si nos quedo abierto de la iteración
         * anterior
         * */
        if (skt != -1)
            ::close(skt);

        /*
         * Con esta llamada creamos/obtenemos un socket.
         * */
        skt = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (skt == -1) {
            continue;
        }

        /* Intentamos conectarnos al servidor cuya dirección
         * fue dada por `getaddrinfo`
         *
         * Esta operación es **bloqueante** lo que significa que el programa
         * va a detenerse unos momentos hasta poder conectarse al server
         * o detectar y notificar de un error.
         * */
        s = connect(skt, addr->ai_addr, addr->ai_addrlen);
        if (s == -1) {
            continue;
        }

        /*
         * Conexión exitosa!
         * */
        this->closed = false;
        this->skt = skt;
        return;
    }

    /*
     * Si terminamos el while-loop es por que no nos pudimos
     * conectar.
     * */
    int saved_errno = errno;

    /*
     * Si el `skt` es -1 es por q (1) no entramos nunca al for-loop
     * o (2) la ultima llamada `socket` fallo y retorno -1.
     * En ambos casos no debemos cerrar ningun socket.
     *
     * Si en cambio `skt` es distinto de -1 significa q tenemos
     * un socket abierto.
     * */
    if (skt != -1)
        ::close(skt);

    throw LibError(
            saved_errno,
            "socket construction failed (connect to %s:%s)",
            (hostname ? hostname : ""),
            (servname ? servname : ""));
}

Socket::Socket(const char *servname) {
    Resolver resolver(nullptr, servname, true);

    int s = -1;
    int skt = -1;
    this->closed = true;
    while (resolver.has_next()) {
        struct addrinfo *addr = resolver.next();

        if (skt != -1)
            ::close(skt);

        skt = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (skt == -1) {
            continue;
        }

        /*
         * Configuramos al socket para q no falle si la dirección local
         * en la que queremos levantar el servidor (con el bind)
         * "estuvo ocupada hace poco".
         *
         * Nota el uso del tiempo pasado: "estuvo ocupada hace poco".
         *
         * Cuando un servidor cualquiera bind'ea en una dirección/puerto
         * local, ese puerto queda en el estado LISTEN (o ESCUCHANDO).
         *
         * Ningún otro programa puede levantase y escuchar en ese puerto
         * "que esta siendo usado".
         *
         * Cuando dicho programa finaliza, podrías pensar q el puerto
         * queda libre, pero no.
         *
         * Queda en el estado WAIT, una especie de estado tipo
         * "estuvo ocupado hace poco".
         *
         * Recién luego de unos segundos el OS deja realmente libre el puerto.
         *
         * Por que? Long story short: el OS no sabe si hay paquetes de red
         * "aun viajando" hacia el puerto y prefiere reservar el puerto
         * para evitar q algún otro servidor desprevenido se levanta ahí
         * y se "morfe" paquetes destinados para el server anterior.
         *
         * Con SO_REUSEADDR le decimos al OS q "esta todo bien" si el
         * puerto aun esta en WAIT, y q nos debe dejar hacer un bind.
         *
         * De ahí el nombre "reuse address" o "SO_REUSEADDR".
         **/
        int optval = 1;
        s = setsockopt(skt, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
        if (s == -1) {
            continue;
        }

        /*
         * Hacemos le bind: enlazamos el socket a una dirección local.
         * A diferencia de lo que hacemos en `Socket::init_for_connection`
         * que obtenemos una dirección de una máquina remota y nos conectamos
         * a ella, el resolver nos dará direcciones locales (véase el flag
         * `is_passive` de `Resolver`).
         *
         * Con `bind` asociaremos el socket a dicha dirección local
         * y con `listen` pondremos el socket a escuchar conexiones entrantes.
         * */
        s = bind(skt, addr->ai_addr, addr->ai_addrlen);
        if (s == -1) {
            continue;
        }

        /*
         * Ponemos el socket a escuchar. Ese 20 (podría ser otro valor)
         * indica cuantas conexiones a la espera de ser aceptadas se toleraran
         *
         * No tiene nada q ver con cuantas conexiones totales el server tendrá.
         * */
        s = listen(skt, 20);
        if (s == -1) {
            continue;
        }

        /*
         * Setup exitoso!
         * */
        this->closed = false;
        this->skt = skt;
        return;
    }

    int saved_errno = errno;

    if (skt != -1)
        ::close(skt);

    throw LibError(
            saved_errno,
            "socket construction failed (listen on %s)",
            (servname ? servname : ""));
}

Socket::Socket(Socket&& other) {
    /* Nos copiamos del otro socket... */
    this->skt = other.skt;
    this->closed = other.closed;

    /* ...pero luego le sacamos al otro socket
     * el ownership del recurso.
     * Efectivamente el ownership pasó de él
     * a nosotros: el ownership se movió.
     *
     * En el caso de `Socket` podemos marcar el file
     * descriptor del otro socket como invalido (-1)
     * y marcarlo como cerrado (closed).
     * De esa manera el destructor de `other` no
     * va a intentar cerrar el file descriptor.
     * */
    other.skt = -1;
    other.closed = true;
}

Socket& Socket::operator=(Socket&& other) {
    /* Si el usuario hace algo como tratar de moverse
     * a si mismo (`skt = skt;`) simplemente no hacemos
     * nada.
     * */
    if (this == &other)
        return *this;

    /* A diferencia del constructor por movimiento,
     * `this` (nosotros) es un socket completamente creado
     * y debemos desinicializarlo primero antes de pisarle
     * el recurso con el que le robaremos al otro socket (`other`)
     * */
    if (not this->closed) {
        ::shutdown(this->skt, 2);
        ::close(this->skt);
    }

    /* Ahora hacemos los mismos pasos que en el move constructor */
    this->skt = other.skt;
    this->closed = other.closed;
    other.skt = -1;
    other.closed = true;

    return *this;
}

int Socket::recvsome(
        void *data,
        unsigned int sz,
        bool *was_closed) {
    chk_skt_or_fail();
    *was_closed = false;
    int s = recv(this->skt, (char*)data, sz, 0);
    if (s == 0) {
        /*
         * Puede ser o no un error, dependerá del protocolo.
         * Alguno protocolo podría decir "se reciben datos hasta
         * que la conexión se cierra" en cuyo caso el cierre del socket
         * no es un error sino algo esperado.
         * */
        *was_closed = true;
        return 0;
    } else if (s == -1) {
        /*
         * 99% casi seguro que es un error real
         * */
        throw LibError(errno, "socket recv failed");
    } else {
        return s;
    }
}

int Socket::sendsome(
        const void *data,
        unsigned int sz,
        bool *was_closed) {
    chk_skt_or_fail();
    *was_closed = false;
    /*
     * Cuando se hace un send, el sistema operativo puede aceptar
     * la data pero descubrir luego que el socket fue cerrado
     * por el otro endpoint quedando la data sin enviar.
     *
     * Esto se lo conoce como "tubería rota" o "broken pipe".
     *
     * En Linux, el sistema operativo envía una señal (`SIGPIPE`) que
     * si no es manejada termina matando al proceso.
     * Manejo de señales esta fuera del alcance de este proyecto.
     *
     * Por suerte si le pasamos a send el flag `MSG_NOSIGNAL`
     * la señal `SIGPIPE` no es enviada y por ende no nos matara el proceso.
     *
     * Esta en nosotros luego hace el chequeo correspondiente
     * (ver más abajo).
     * */
    int s = send(this->skt, (char*)data, sz, MSG_NOSIGNAL);
    if (s == -1) {
        /*
         * Este es un caso especial: cuando enviamos algo pero en el medio
         * se detecta un cierre del socket no se sabe bien cuanto se logro
         * enviar (y fue recibido por el peer) y cuanto se perdió.
         *
         * Este es el famoso broken pipe.
         * */
        if (errno == EPIPE) {
            /*
             * Puede o no ser un error (véase el comentario en `Socket::recvsome`)
             * */
            *was_closed = true;
            return 0;
        }

        /* En cualquier otro caso supondremos un error
         * y lanzamos una excepción.
         * */
        *was_closed = true;
        throw LibError(errno, "socket send failed");
    } else if (s == 0) {
        /*
         * Jamas debería pasar.
         * */
        assert(false);
    } else {
        return s;
    }
}

int Socket::recvall(
        void *data,
        unsigned int sz,
        bool *was_closed) {
    unsigned int received = 0;
    *was_closed = false;

    while (received < sz) {
        int s = recvsome(
                (char*)data + received,
                sz - received,
                was_closed);

        if (s <= 0) {
            /*
             * Si el socket fue cerrado (`s == 0`) o hubo un error
             * `Socket::recvsome` ya debería haber seteado `was_closed`
             * y haber notificado el error.
             *
             * Nosotros podemos entonces meramente
             *  - lanzar excepción si recibimos algunos bytes pero no todos los pedidos
             *  - propagar la excepción `Socket::recvsome` si esto falló.
             *  - retornar end of stream (0) si es lo q recibimos de `Socket::recvsome`
             * */
            assert(s == 0);
            if (received)
                throw LibError(
                        EPIPE,
                        "socket received only %d of %d bytes",
                        received,
                        sz);
            else
                return 0;
        } else {
            /*
             * OK, recibimos algo pero no necesariamente todo lo que
             * esperamos. La condición del `while` checkea eso justamente.
             * */
            received += s;
        }
    }

    return sz;
}


int Socket::sendall(
        const void *data,
        unsigned int sz,
        bool *was_closed) {
    unsigned int sent = 0;
    *was_closed = false;

    while (sent < sz) {
        int s = sendsome(
                (char*)data + sent,
                sz - sent,
                was_closed);

        /* Véase los comentarios de `Socket::recvall` */
        if (s <= 0) {
            assert(s == 0);
            if (sent)
                throw LibError(
                        EPIPE,
                        "socket sent only %d of %d bytes",
                        sent,
                        sz);
            else
                return 0;
        } else {
            sent += s;
        }
    }

    return sz;
}

Socket::Socket(int skt) {
    this->skt = skt;
    this->closed = false;
}

Socket Socket::accept() {
    chk_skt_or_fail();
    /*
     * `accept` nos bloqueara hasta que algún cliente se conecte a nosotros
     * y la conexión se establezca.
     *
     * Una vez que eso suceda, `accept` retornara el file descriptor
     * de un *nuevo* socket (`peer_skt`) que representara a la
     * conexión establecida con *ese* cliente.
     *
     * En todo momento podemos seguir usando *nuestro* file descriptor
     * (`this->skt`) para seguir haciendo más llamadas a `accept`
     * independientemente de que enviemos/recibamos del socket `peer`.
     * */
    int peer_skt = ::accept(this->skt, nullptr, nullptr);
    if (peer_skt == -1)
        throw LibError(errno, "socket accept failed");

    /*
     * `peer_skt` es un file descriptor crudo y no queremos
     * que nuestro cliente manipule recursos crudos sino que
     * los use a través de un TDA.
     *
     * Por eso creamos un `Socket` y lo pasamos por movimiento
     * */
    return Socket(peer_skt);
}

void Socket::shutdown(int how) {
    chk_skt_or_fail();
    if (::shutdown(this->skt, how) == -1) {
        throw LibError(errno, "socket shutdown failed");
    }
}

int Socket::close() {
    chk_skt_or_fail();
    this->closed = true;
    return ::close(this->skt);
}

Socket::~Socket() {
    if (not this->closed) {
        ::shutdown(this->skt, 2);
        ::close(this->skt);
    }
}

void Socket::chk_skt_or_fail() const {
    if (skt == -1) {
        throw std::runtime_error(
                "socket with invalid file descriptor (-1), "
                "perhaps you are using a *previously moved* "
                "socket (and therefore invalid)."
                );
    }
}
