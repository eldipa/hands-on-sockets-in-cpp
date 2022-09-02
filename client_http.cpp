#include "resolver.h"

/*
 * Includes para `socket` y compañía
 * */
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdio>
#include <cassert>
#include <cstring>
#include <cerrno>

/*
 * Este programa es un mini cliente HTTP que se conecta a un servidor
 * (www.google.com.ar) y le pide una página web y la imprime por pantalla.
 * */
int main(int argc, char *argv[]) {
    int s = -1;

    if (argc != 1) {
        printf("Bad program call. Expected %s without arguments.\n", argv[0]);
        return -1;
    }

    /*
     * HTTP/1.1 es un protocolo de texto en donde el cliente (nosotros)
     * le hace un pedido a un servidor (Google).
     *
     * Que clase de pedido dependerá del servidor pero tradicionalmente
     * se soportan obtener un recurso (GET), guardar uno nuevo (PUT),
     * actualizarlo (POST) y borrarlo (DELETE).
     *
     * Exactamente que es un "recurso" depende del servidor. Para Google
     * un recurso es una página web HTML. Para muchos servidores HTTP
     * sucede lo mismo pero no te confundas: nada te impide usar HTTP
     * para otros fines que no sean paginas web HTML.
     *
     * En este ejemplo nos limitaremos únicamente a pedir la home-page
     * de Google.
     * */
    const char req[] = "GET / HTTP/1.1\r\n"
                       "Accept: */*\r\n"
                       "Connection: close\r\n"
                       "Host: www.google.com.ar\r\n"
                       "\r\n";

    /*
     * El TDA `resolver_t` se encargara de resolver el hostname/service name
     * encapsulando todos los detalles que no nos interesan saber.
     * */
    struct resolver_t resolver;
    s = resolver_init(&resolver, "www.google.com.ar", "http");
    if (s == -1)
        return -1;


    /*
     * Para código user (nuestro programa), un socket es tan solo un número.
     * Pero es más que eso, es un "file descriptor".
     *
     * Para el sistema operativo crear un socket requiere reservar varios
     * recursos que accederemos usando este file descriptor (podes pensar
     * que es como si fuese un "puntero al socket del sistema operativo".
     *
     * La manpage the socket dice que -1 es un file descriptor invalido
     * así que usaremos el -1 para marcar que el socket no es válido.
     * */
    int skt = -1;
    bool connected = false;

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
        } else {
            connected = true;
            break;
        }
    }

    if (not connected) {
        /*
         * La manpage tanto de socket como de connect nos dicen que si fallan
         * retornan -1 y ponen en la variable global `errno` el código de error.
         *
         * Aquí preservamos una copia para imprimir el mensaje luego
         * con `strerror`.
         * */
        int saved_errno = errno;
        printf("Connection failed: %s\n", strerror(saved_errno));

        /*
         * No nos pudimos conectar, finalizamos el programa y nada más.
         * */
        return -1;
    }

    resolver_deinit(&resolver);

    /*
     * Con el socket creado y conectado, ahora enviamos el request HTTP
     * con `send`.
     *
     * `send` es *bloqueante*. Si la red o el server están muy lentos
     * nuestro sistema operativo no podrá enviar toda la información
     * y empezara a bloquearnos/frenarnos (para que no sigamos enviando
     * data).
     *
     * El request está en el buffer por lo que le pasamos un puntero a él
     * y su tamaño.
     *
     * El sistema operativo podría **no enviar todo lo que le pedimos
     * enviar**. Los sockets pueden enviar **menos** data.
     * Esto lo vamos a tener que arreglar en un próximo commit....
     *
     * Nota: de `MSG_NOSIGNAL` hablaremos luego.
     *
     * Nota: nótese que pasamos `sizeof(req) - 1` y no `sizeof(req)`.
     * El protocolo HTTP es solo de texto y explícitamente nos pide
     * que los mensajes terminen en `\r\n` y **no** en un `\0`
     * (cosa que el buffer `req` tiene en su último byte, de ahí el `-1`).
     *
     * */
    s = send(skt, req, sizeof(req) - 1, MSG_NOSIGNAL);
    if (s == -1) {
        /*
         * O bien detectamos que la conexión se cerró (el servidor cerró
         * la conexión) o bien detectamos un error en la comunicación.
         *
         * Que el servidor cierre la conexión puede ser o no un error,
         * dependerá del protocolo.
         *
         * Algún protocolo podría decir "luego de enviar un request
         * la conexión se cerrara" en cuyo caso el cierre del socket
         * no es un error sino algo esperado.
         *
         * El protocolo HTTP sin embargo no es así y un cierre
         * marca un error.
         *
         * Si `s == -1` es por que hubo un error en la comunicación,
         * es seguro que es un error real.
         *
         * Este código hay que arreglarlo por que no distingue
         * una cierre de un error. Por ahora supondré que es un cierre.
         * */
        printf("The connection was closed by the other end.\n");
        return -1;
    } else if (s == 0) {
        /*
         * Esto jamás debería pasar ya que `send` retorna la cantidad
         * de bytes enviados (mínimo 1) o `-1`.
         * */
        assert(false);
    }

    /*
     * El valor de retorno de `send`, si es un número positivo,
     * es la cantidad de bytes que realmente se pudieron enviar.
     * */
    printf("Sent %d bytes\n", s);


    /*
     * Ahora recibimos la respuesta.
     *
     * `recv` es *bloqueante*. Si la red o el server están muy lentos
     * nuestro sistema operativo no podrá recibir toda la información
     * y empezara a bloquearnos/frenarnos.
     *
     * El tamaño del buffer **no** dice cuantos bytes vamos a recibir
     * sino **hasta cuantos**  bytes podemos recibir.
     *
     * El sistema operativo podría **no darnos todo lo que le pedimos
     * recibir**. Los sockets pueden recibir **menos** data.
     * Esto lo vamos a tener que arreglar en un próximo commit....
     *
     * */
    char buf[512] = {0};
    s = recv(skt, buf, sizeof(buf) - 1, 0);
    if (s == 0) {
        /*
         * Detectamos que la conexión se cerró (el servidor cerró la conexión
         * de forma ordenada con un `shutdown`).
         *
         * Puede ser o no un error, dependerá del protocolo.
         *
         * El protocolo HTTP sin embargo no es así y un cierre
         * marca un error ya que esperábamos recibir una página web.
         * */
        printf("The connection was closed by the other end.\n");
        return -1;
    } else if (s == -1) {
        /*
         * 99% casi seguro que es un error sea por que hubo
         * un error en la comunicación o por que el servidor
         * hizo un cierre desordenado.
         * */
        perror("socket recv failed");
        return s;
    }


    /*
     * Si `recv` retorno un número positivo, esta es la cantidad
     * de bytes que realmente fueron recibidos.
     *
     * Si llegase haber más data nos la estaríamos perdiendo.
     *
     * **Muy probablemente** no vamos a recibir toda la página web.
     *
     * Esto lo vamos a tener que arreglar en un próximo commit....
     * */
    printf("Page:\n%s\n", buf);

    /*
     * Cerramos la conexión. Los sockets TCP son full-duplex
     * por lo que se puede enviar (write) y recibir (read)
     * independientemente.
     *
     * La función `shutdown` nos permite cerrar uno de los flujos
     * (read o write) o ambos.
     *
     * En este caso cerramos ambos (read y write).
     * */
    shutdown(skt, SHUT_RDWR);

    /*
     * Como todo file descriptor cerramos el file.
     * */
    close(skt);

    return 0;
}
