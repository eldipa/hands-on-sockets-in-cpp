#include "socket.h"

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
     * El TDA `socket_t` se encargara de resolver el hostname/service name
     * y se conectará a dicho server via TCP/IP.
     * */
    struct socket_t skt;
    s = socket_init(&skt, "www.google.com.ar", "http");
    if (s == -1)
        return -1;

    /*
     * Con el socket creado y conectado, ahora enviamos el request HTTP
     * con `send`.
     *
     * El sistema operativo podría **no enviar todo lo que le pedimos
     * enviar**. Los sockets pueden enviar **menos** data.
     * Esto lo vamos a tener que arreglar en un próximo commit....
     *
     * */
    bool was_closed;
    s = socket_sendsome(&skt, req, sizeof(req) - 1, &was_closed);

    if (was_closed) {
        /*
         * Que el servidor cierre la conexión puede ser o no un error,
         * dependerá del protocolo.
         *
         * Algún protocolo podría decir "luego de enviar un request
         * la conexión se cerrara" en cuyo caso el cierre del socket
         * no es un error sino algo esperado.
         *
         * El protocolo HTTP sin embargo no es así y un cierre
         * marca un error.
         * */
        printf("The connection was closed by the other end.\n");
        return -1;
    }

    assert(s != 0);
    if (s == -1) {
        perror("socket send failed");
        return -1;
    }

    /*
     * El valor de retorno de `send`, si es un número positivo,
     * es la cantidad de bytes que realmente se pudieron enviar.
     * */
    printf("Sent %d bytes\n", s);


    /*
     * Ahora recibimos la respuesta.
     *
     * El sistema operativo podría **no darnos todo lo que le pedimos
     * recibir**. Los sockets pueden recibir **menos** data.
     * Esto lo vamos a tener que arreglar en un próximo commit....
     *
     * */
    char buf[512] = {0};
    s = socket_recvsome(&skt, buf, sizeof(buf) - 1, &was_closed);
    if (was_closed) {
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
    }

    if (s == -1) {
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
     * Liberamos los recursos.
     *
     * El TDA socket_t que se implementó se encargará de
     * hacer el shutdown y el close por nosotros.
     * */
    socket_deinit(&skt);

    return 0;
}
