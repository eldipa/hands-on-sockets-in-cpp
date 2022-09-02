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

int socket_init(
        struct socket_t *self,
        const char *hostname,
        const char *servname) {
    struct resolver_t resolver;
    int s = resolver_init(&resolver, hostname, servname);
    if (s == -1)
        return -1;

    int skt = -1;
    self->closed = true;

    /*
     * Por cada dirección obtenida tenemos que ver cual es realmente funcional.
     * `getaddrinfo` puede darnos direcciones IP validas pero que apuntan
     * a servidores que no están activos (`getaddrinfo` simplemente no
     * lo puede saber).
     *
     * Es responsabilidad nuestra probar cada una de ellas hasta encontrar
     * una que funcione.
     * */
    while (resolver_has_next(&resolver)) {
        struct addrinfo *addr = resolver_next(&resolver);

        /* Cerramos el socket si nos quedo abierto de la iteración
         * anterior
         * */
        if (skt != -1)
            close(skt);

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
        self->closed = false;
        self->skt = skt;
        return 0;
    }

    /*
     * Si terminamos el while-loop es por que no nos pudimos
     * conectar.
     * */
    int saved_errno = errno;
    printf("Connection failed: %s\n", strerror(saved_errno));

    resolver_deinit(&resolver);
    return -1;
}


int socket_recvsome(
        struct socket_t *self,
        void *data,
        unsigned int sz,
        bool *was_closed) {
    *was_closed = false;
    int s = recv(self->skt, (char*)data, sz, 0);
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

int socket_sendsome(
        struct socket_t *self,
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
    int s = send(self->skt, (char*)data, sz, MSG_NOSIGNAL);
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
             * Puede o no ser un error (véase el comentario en `socket_recvsome`)
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

int socket_recvall(
        struct socket_t *self,
        void *data,
        unsigned int sz,
        bool *was_closed) {
    unsigned int received = 0;
    *was_closed = false;

    while (received < sz) {
        int s = socket_recvsome(
                self,
                (char*)data + received,
                sz - received,
                was_closed);

        if (s <= 0) {
            /*
             * Si el socket fue cerrado (`s == 0`) o hubo un error (`s == -1`)
             * `socket_recvsome` ya debería haber seteado `was_closed`
             * y haber notificado el error.
             *
             * Nosotros podemos entonces meramente retornar
             *  - error (-1) si recibimos algunos bytes pero no todos los pedidos
             *  - error (-1) si `socket_recvsome` falló con error.
             *  - end of stream (0) si es lo q recibimos de `socket_recvsome`
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


int socket_sendall(
        struct socket_t *self,
        const void *data,
        unsigned int sz,
        bool *was_closed) {
    unsigned int sent = 0;
    *was_closed = false;

    while (sent < sz) {
        int s = socket_sendsome(
                self,
                (char*)data + sent,
                sz - sent,
                was_closed);

        /* Véase los comentarios de `socket_recvall` */
        if (s <= 0) {
            return (sent ? -1 : s);
        } else {
            sent += s;
        }
    }

    return sz;
}

int socket_shutdown(struct socket_t *self, int how) {
    if (shutdown(self->skt, how) == -1) {
        perror("socket shutdown failed");
        return -1;
    }

    return 0;
}

int socket_close(struct socket_t *self) {
    self->closed = true;
    return close(self->skt);
}

void socket_deinit(struct socket_t *self) {
    if (not self->closed) {
        shutdown(self->skt, 2);
        close(self->skt);
    }
}
