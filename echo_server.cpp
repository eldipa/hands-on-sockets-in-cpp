#include <iostream>
#include <exception>
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
int main(int argc, char *argv[]) { try {
    int ret = -1;

    const char *servname = NULL;

    if (argc == 2) {
        servname = argv[1];
    } else {
        std::cerr << "Bad program call. Expected "
                << argv[0]
                << " <servname>\n";
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
    while (true) {
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
        int sz = peer.recvsome(buf, sizeof(buf));
        if (peer.is_stream_recv_closed())
            break;

        peer.sendall(buf, sz);
        if (peer.is_stream_send_closed())
            break;
    }

    ret = 0;
    return ret;
} catch (const std::exception& err) {
    std::cerr
        << "Something went wrong and an exception was caught: "
        << err.what()
        << "\n";
    return -1;
} catch (...) {
    std::cerr << "Something went wrong and an unknown exception was caught.\n";
    return -1;
} }
