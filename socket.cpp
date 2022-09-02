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
    int s = send(self->skt, (char*)data, sz, MSG_NOSIGNAL);
    if (s == -1) {
        /*
         * O bien detectamos que la conexión se cerró (el servidor cerró
         * la conexión) o bien detectamos un error en la comunicación.
         *
         * Que el servidor cierre la conexión puede ser o no un error,
         * dependerá del protocolo.
         *
         * Este código hay que arreglarlo por que no distingue
         * una cierre de un error. Por ahora supondré que es un cierre.
         * */
        *was_closed = true;
        return 0;
    } else if (s == 0) {
        /*
         * Jamas debería pasar.
         * */
        assert(false);
    } else {
        return s;
    }
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
