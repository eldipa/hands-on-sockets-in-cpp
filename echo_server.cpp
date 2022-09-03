#include <iostream>
#include "socket.h"

/*
 * Este programa es un mini echo server, un servidor TCP/IP que espera
 * una única conexión y luego, todo lo que recibe desde el cliente
 * se lo envía de nuevo.
 *
 * Es un echo server!
 *
 * Este servidor olo acepta a un único cliente y solo termina una vez
 * que el cliente decida desconectarse.
 *
 * Cuando veamos threads podremos implementar servidores multi-clientes
 * y podremos cerrar todo en orden.
 *
 * Si queres probar el server corriendo:
 *
 *  ./echo_server 8080
 *
 * y en otra consola:
 *
 *  nc 127.0.0.1 8080
 *
 **/
int main(int argc, char *argv[]) {
    int ret = -1;
    bool was_closed = false;

    const char *servname = NULL;

    if (argc == 2) {
        servname = argv[1];
    } else {
        printf(
                "Bad program call. Expected %s <servname>\n",
                argv[0]);
        return ret;
    }

    /*
     * Construimos nuestro socket "server" o "aceptador"
     * que usaremos para escuchar y aceptar conexiones entrantes.
     *
     * En este mini-ejemplo tendremos un socket `srv` (server) y
     * otro llamado peer que representara a nuestro cliente.
     * En general cualquier servidor real tendrá N+1 sockets,
     * uno para escuchar y aceptar y luego N sockets para sus
     * N clientes.
     * */
    Socket srv(servname);

    /*
     * Bloqueamos el programa hasta q haya una conexión entrante
     * y sea aceptada. Hablaremos (`send`/`recv`) con ese cliente
     * conectado en particular usando un socket distinto, el `peer`,
     * construido aquí.
     * */
    Socket peer = srv.accept();

    /*
     * A partir de aquí podríamos volver a usar `srv` para aceptar
     * nuevos clientes a la vez q hablamos con `peer` pero
     * en este mini-ejemplo nos quedaremos con algo simple
     * de un solo cliente.
     * */

    char buf[512];
    while (not was_closed) {
        /*
         * Loop principal: lo que el servidor recibe lo vuelve a enviar
         * al cliente. Es un *echo* server después de todo!
         *
         * Usamos `Socket::recvsome` por q no sabemos cuanto vamos a
         * recibir exactamente pero usamos `Socket::sendall` por
         * que sabemos cuanto queremos enviar.
         *
         * Podríamos usar también `Socket::sendsome` para hacer
         * ciertas optimizaciones pero acá nos quedamos con la
         * version simple (y fácil de entender).
         * Si queres indagar más podes ver la implementación
         * de `tiburoncin` pero te advierto, es heavy.
         * https://github.com/eldipa/tiburoncin
         *
         * Pregunta: por que usamos `sizeof(buf)` en este `Socket::recvsome`
         * pero usamos `sizeof(buf)-1` en el `Socket::recvsome`
         * de `cliente_http.cpp`?
         * */
        int sz = peer.recvsome(buf, sizeof(buf), &was_closed);

        if (was_closed)
            break;

        if (sz == -1) {
            /*
             * 99% casi seguro que es un error
             * */
            perror("socket recv failed");
            return ret;
        }

        int s = -1;
        s = peer.sendall(buf, sz, &was_closed);

        if (was_closed)
            break;

        if (s == -1) {
            perror("socket send failed");
            return ret;
        }
    }

    ret = 0;
    return ret;
}
