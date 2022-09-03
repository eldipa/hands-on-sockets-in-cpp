#include "resolver.h"
#include <exception>
#include <iostream>

/*
 * Includes necesarios para `inet_ntoa`
 * */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <cstdio>
#include <cstring>

/*
 * Modo de uso:
 *
 *  ./resolve_name <hostname> [<servname>]
 *
 * Por ejemplo:
 *
 *  ./resolve_name google.com
 *  ./resolve_name fi.uba.ar https
 *
 * El resolver imprime por pantalla la o las IPv4
 * que el hostname tiene asociado. Si se pasa un servicio
 * también se imprime el puerto TCP.
 *
 * */
int main(int argc, char *argv[]) { try {
    const char *hostname = NULL;
    const char *servname = NULL;

    if (argc == 2) {
        hostname = argv[1];
    } else if (argc == 3) {
        hostname = argv[1];
        servname = argv[2];
    } else {
        printf(
                "Bad program call. Expected %s <hostname> [<servname>]\n",
                argv[0]);
        return -1;
    }

    /*
     * El TDA `Resolver` se encargara de resolver el hostname/service name
     * encapsulando todos los detalles que no nos interesan saber.
     *
     * En particular el TDA se encarga de realizar todos los chequeos de errores
     * por nosotros y retornar un único código que en C (así como en Golang)
     * tenemos que chequear nosotros.
     * */
    Resolver resolver(hostname, servname, false);

    /*
     * Recorda que `Resolver` te da una lista de direcciones
     * (que vienen de `getaddrinfo`)
     *
     * Un mismo nombre puede resolverse a múltiples direcciones que
     * sirven como balanceo de carga y redundancia.
     *
     * Para los fines de `resolve_name`, solo nos interesa imprimir
     * la dirección IPv4 con la notación típica `a.b.c.d`.
     *
     * `inet_ntoa` es solo para direcciones IPv4! Este programa
     * no soporta IPv6.
     * */
    while (resolver.has_next()) {
        struct addrinfo *rp = resolver.next();

        struct sockaddr_in *skt_addr = (struct sockaddr_in*)rp->ai_addr;
        struct in_addr internet_addr = skt_addr->sin_addr;

        /*
         * Según la documentación de `inet_ntoa`, el puerto (`uint16_t`)
         * que esta en la estructura `sockaddr_in` esta en big endian
         * no importa el endianness de la máquina.
         *
         * Para imprimir el número y que tenga sentido hay que
         * convertirlo al endianness nativo de la máquina.
         *
         * network-to-host-short, o `ntohs`.
         * */
        uint16_t port = ntohs(skt_addr->sin_port);

        if (port)
            printf("IPv4: %s (port %d)\n", inet_ntoa(internet_addr), port);
        else
            printf("IPv4: %s\n", inet_ntoa(internet_addr));
    }

    return 0;
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
