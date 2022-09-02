#include "socket.h"

#include <cstdio>
#include <cassert>
#include <cstring>
#include <cerrno>
#include <cctype>

/*
 * Este programa es un mini cliente HTTP que se conecta a un servidor
 * (www.google.com.ar) y le pide una página web y la imprime por pantalla.
 * */
int main(int argc, char *argv[]) {
    int ret = -1;
    int s = -1;

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

    if (argc != 1) {
        printf("Bad program call. Expected %s without arguments.\n", argv[0]);
        goto bad_prog_call;
    }

    /*
     * El TDA `socket_t` se encargara de resolver el hostname/service name
     * y se conectará a dicho server via TCP/IP.
     * */
    struct socket_t skt;
    s = skt.init_for_connection("www.google.com.ar", "http");
    if (s == -1)
        goto connection_failed;

    /*
     * Con el socket creado y conectado, ahora enviamos el request HTTP
     * con `socket_t::sendall`.
     *
     * `socket_t::sendall` se encarga de llamar a `send` varias veces hasta
     * lograr enviar todo lo pedido o fallar.
     *
     * Esta es la manera de asegurarse el envío completo de los
     * datos ante un short-write.
     * */
    bool was_closed;
    s = skt.sendall(req, sizeof(req) - 1, &was_closed);

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
        goto connection_closed;
    }

    assert(s != 0);
    if (s == -1) {
        perror("socket send failed");
        goto send_failed;
    }

    /*
     * El valor de retorno de `send`, si es un número positivo,
     * es la cantidad de bytes que realmente se pudieron enviar.
     * */
    printf("Sent %d bytes\n", s);


    /*
     * Ahora recibimos la respuesta.
     *
     * Al igual que `send`, el `recv` puede recibir menos bytes de
     * los pedidos.
     *
     * `recv` sufre de short-reads.
     *
     * Si quisiéramos bloquearnos hasta leer por completo la
     * página web deberíamos llamar a `socket_t::recvall` que implementa
     * el loop y resuelve el short-read.
     *
     * Sin embargo, para que queremos leer toda la página
     * y solo luego trabajar? Por que no leer lo que se pueda,
     * imprimir y volver a loopear? Seria más eficiente!
     *
     * Realmente no necesitamos toda la página hasta poder
     * imprimirla. Podemos ir imprimiéndola a medida que nos
     * llega.
     *
     * El short-read no es un bug, es un feature!
     *
     * Por eso aquí usamos `socket_t::recvsome` y no `socket_t::recvall`.
     * */
    printf("Page:\n");
    while (not was_closed) {
        char buf[512] = {0};
        s = skt.recvsome(buf, sizeof(buf) - 1, &was_closed);
        if (was_closed) {
            break;
        }

        if (s == -1) {
            /*
             * 99% casi seguro que es un error
             * */
            perror("socket recv failed");
            goto recv_failed;
        }

        /*
         * Pregunta: está garantizado que `buf` tiene un `\0` al final? Quien lo
         * garantiza?
         * (este for-loop no tiene nada que ver con los sockets)
         * */
        for (int i = 0; buf[i]; ++i)
            if (not isascii(buf[i]))
                buf[i] = '@';

        printf("%s", buf);
    }
    printf("\n");

    /*
     * Si llegamos hasta acá es por que no nos topamos con ningún error
     * por lo tanto return-code de esta función (y de este programa)
     * debe ser 0 (éxito).
     *
     * Si hubiéramos detectado algo error, nunca ejecutaríamos
     * este código y main retornaría el valor de `ret` por default
     * que es -1 (error).
     * */
    ret = 0;

recv_failed:
send_failed:
connection_closed:
    /*
     * Liberamos los recursos.
     *
     * El TDA socket_t que se implementó se encargará de
     * hacer el shutdown y el close por nosotros.
     * */
    skt.deinit();

connection_failed:
bad_prog_call:
    return ret;
}
