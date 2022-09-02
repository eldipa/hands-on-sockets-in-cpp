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

int Socket::init_for_connection(
        const char *hostname,
        const char *servname) {
    Resolver resolver;
    int s = resolver.init(hostname, servname, false);
    if (s == -1)
        return -1;

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
        resolver.deinit();
        return 0;
    }

    /*
     * Si terminamos el while-loop es por que no nos pudimos
     * conectar.
     * */
    int saved_errno = errno;
    printf("Connection failed: %s\n", strerror(saved_errno));

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

    resolver.deinit();
    return -1;
}

int Socket::init_for_listen(const char *servname) {
    Resolver resolver;
    int s = resolver.init(nullptr, servname, true);
    if (s == -1)
        return -1;

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
        resolver.deinit();
        return 0;
    }

    int saved_errno = errno;
    printf("Socket setup failed: %s\n", strerror(saved_errno));

    if (skt != -1)
        ::close(skt);

    resolver.deinit();
    return -1;
}

int Socket::recvsome(
        void *data,
        unsigned int sz,
        bool *was_closed) {
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
        perror("socket recv failed");
        return s;
    } else {
        return s;
    }
}

int Socket::sendsome(
        const void *data,
        unsigned int sz,
        bool *was_closed) {
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
         * y retornamos -1.
         * */
        *was_closed = true;
        return -1;
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
             * Si el socket fue cerrado (`s == 0`) o hubo un error (`s == -1`)
             * `Socket::recvsome` ya debería haber seteado `was_closed`
             * y haber notificado el error.
             *
             * Nosotros podemos entonces meramente retornar
             *  - error (-1) si recibimos algunos bytes pero no todos los pedidos
             *  - error (-1) si `Socket::recvsome` falló con error.
             *  - end of stream (0) si es lo q recibimos de `Socket::recvsome`
             * */
            return (received ? -1 : s);
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
            return (sent ? -1 : s);
        } else {
            sent += s;
        }
    }

    return sz;
}

int Socket::init_with_file_descriptor(Socket *peer, int skt) {
    peer->skt = skt;
    peer->closed = false;

    return 0;
}

int Socket::accept(Socket *peer) {
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
        return -1;

    /*
     * `peer_skt` es un file descriptor crudo y no queremos
     * que nuestro cliente manipule recursos crudos sino que
     * los use a través de un TDA.
     *
     * Por eso inicializamos el TDA `peer` con el nuevo file
     * descriptor.
     *
     * Nota: `peer` debe ser un `Socket` *sin inicializar*
     * para que seamos nosotros quienes lo inicializamos aquí.
     * */
    int s = init_with_file_descriptor(peer, peer_skt);
    if (s == -1)
        return -1;

    return 0;
}

int Socket::shutdown(int how) {
    if (::shutdown(this->skt, how) == -1) {
        perror("socket shutdown failed");
        return -1;
    }

    return 0;
}

int Socket::close() {
    this->closed = true;
    return ::close(this->skt);
}

void Socket::deinit() {
    if (not this->closed) {
        ::shutdown(this->skt, 2);
        ::close(this->skt);
    }
}
