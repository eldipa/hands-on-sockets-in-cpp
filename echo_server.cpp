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
    int s = -1;
    bool was_closed = false;

    const char *servname = NULL;

    if (argc == 2) {
        servname = argv[1];
    } else {
        printf(
                "Bad program call. Expected %s <servname>\n",
                argv[0]);
        goto bad_prog_call;
    }

    /*
     * Inicializamos nuestro socket "server" o "aceptador"
     * que usaremos para escuchar y aceptar conexiones entrantes.
     *
     * En este mini-ejemplo tendremos un socket `srv` (server) y
     * otro llamado peer que representara a nuestro cliente.
     * En general cualquier servidor real tendrá N+1 sockets,
     * uno para escuchar y aceptar y luego N sockets para sus
     * N clientes.
     * */
    class socket_t peer, srv;
    s = srv.init_for_listen(servname);
    if (s == -1)
        goto listen_failed;

    /*
     * Bloqueamos el programa hasta q haya una conexión entrante
     * y sea aceptada. Hablaremos (`send`/`recv`) con ese cliente
     * conectado en particular usando un socket distinto, el `peer`,
     * inicializado aquí.
     * */
    s = srv.accept(&peer);
    if (s == -1)
        goto accept_failed;

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
         * Usamos `socket_t::recvsome` por q no sabemos cuanto vamos a
         * recibir exactamente pero usamos `socket_t::sendall` por
         * que sabemos cuanto queremos enviar.
         *
         * Podríamos usar también `socket_t::sendsome` para hacer
         * ciertas optimizaciones pero acá nos quedamos con la
         * version simple (y fácil de entender).
         * Si queres indagar más podes ver la implementación
         * de `tiburoncin` pero te advierto, es heavy.
         * https://github.com/eldipa/tiburoncin
         *
         * Pregunta: por que usamos `sizeof(buf)` en este `socket_t::recvsome`
         * pero usamos `sizeof(buf)-1` en el `socket_t::recvsome`
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
            goto recv_failed;
        }

        s = peer.sendall(buf, sz, &was_closed);

        if (was_closed)
            break;

        if (s == -1) {
            perror("socket send failed");
            goto send_failed;
        }
    }

    ret = 0;

    /*
     * Nótese el orden de los `goto` (labels) y de la liberación de cada
     * recurso.
     * El orden es exactamente el inverso a como se fueron reservando
     * dichos recursos.
     *
     * Ambos forman un "stack":
     *
     *      ret = -1; // failure by default         Recursos
     *
     *      reservo A;                              [ )
     *      if (fail)
     *         goto reservar_A_failed;
     *
     *      reservo B;                              [ A )
     *      if (fail)
     *         goto reservar_B_failed;
     *
     *      reservo C;                              [ A | B )
     *      if (fail)
     *         goto reservar_C_failed;
     *
     *      ...                                     [ A | B | C )
     *
     *      ret = 0; // exito
     *
     *      libero C;
     *
     *  reservar_C_failed:                          [ A | B )
     *      libero B;
     *
     *  reservar_B_failed:                          [ A )
     *      libero A;
     *
     *  reservar_A_failed:                          [ )
     *      return ret;
     *
     * */
send_failed:
recv_failed:
    peer.deinit();

accept_failed:
    srv.deinit();

listen_failed:
bad_prog_call:
    return ret;
}
