/*
 * Includes necesarios para `getaddrinfo`
 * */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

/*
 * Includes necesarios para `inet_ntoa`
 * */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <cstdio>
#include <cstring>
#include <cerrno>

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
int main(int argc, char *argv[]) {
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
     * Ver más adelante
     * */
    struct addrinfo hints;
    struct addrinfo *result;

    /*
     * `getaddrinfo` nos resuelve el nombre de una máquina (host) y de un
     * servicio a una dirección.
     * Nos puede retornar múltiples direcciones incluso de
     * protocolos/tecnologías que no nos interesan.
     * Para pre-seleccionar que direcciones nos interesan le pasamos
     * un hint, una estructura con algunos campos completados (no todos)
     * que le indicaran que tipo de direcciones queremos.
     *
     * Para nuestros fines queremos direcciones de internet IPv4
     * y para servicios de TCP.
     * */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;       /* IPv4 (or AF_INET6 for IPv6)     */
    hints.ai_socktype = SOCK_STREAM; /* TCP  (or SOCK_DGRAM for UDP)    */
    hints.ai_flags = 0;  /* ya hablaremos de este flag */


    /* Obtengo la (o las) direcciones según el nombre de host y servicio que
     * busco
     *
     * De todas las direcciones posibles, solo me interesan aquellas que sean
     * IPv4 y TCP (según lo definido en hints)
     *
     * El resultado lo guarda en result que es un puntero al primer nodo
     * de una lista simplemente enlazada.
     * */
    int s = getaddrinfo(hostname, servname, &hints, &result);

    /* Es muy importante chequear los errores.
     *
     * En C, Golang, Rust, la forma de comunicar errores al caller (a quien
     * nos llamó) es retornando un código de error.
     *
     * La página de manual de `getaddrinfo` aclara que si `s == 0`
     * entonces todo salio bien.
     *
     * Si `s == EAI_SYSTEM` entonces el error es del sistema y deberemos
     * inspeccionar la variable global `errno`.
     *
     * Si `s != EAI_SYSTEM`, entonces el valor de retorno debe ser
     * inspeccionado con `gai_strerror`.
     *
     * Lo siguiente a continuación es una muy primitiva y simplificada
     * forma de manejo de errores. Ya lo mejoraremos.
     * */
    if (s != 0) {
        if (s == EAI_SYSTEM) {
            /*
             * Como `errno` es global y puede ser modificada por *cualquier* otra
             * función, es *importantísimo* copiarla apenas detectemos el error.
             * De otro modo nos arriesgamos a que cualquier otra función
             * que llamemos, como `printf`, pueda pisarnos la variable y
             * y perdamos el código del error.
             */
            int saved_errno = errno;

            /*
             * Podemos usar `strerror` para traducir ese código de error
             * en un mensaje por un humano y así imprimirlo.
             *
             * `strerror` *no* es thread-safe así que esto es solo una
             * version draft y *no* debería ser usado (la version thread-safe
             * es `strerror_r`)
             * */
            printf(
                    "Host/service name resolution failed (getaddrinfo): %s\n",
                    strerror(saved_errno));

        } else {
            /*
             * La documentación de `getaddrinfo` dice que en este caso
             * debemos usar `gai_strerror` para obtener el mensaje de error.
             * */
            printf(
                    "Host/service name resolution failed (getaddrinfo): %s\n",
                    gai_strerror(s));
        }

        /*
         * Finalizamos el programa. Esto lo podemos hacer sin problemas
         * ya que al fallar `getaddrinfo` este *no* reservo ningún recurso
         * y por lo tanto *no* tenemos que liberar ninguno.
         * */
        return -1;
    }

    /*
     * Recorda que `getaddrinfo` te da una lista de direcciones.
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
    for (struct addrinfo *rp = result; rp != NULL; rp = rp->ai_next) {
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

    /*
     * `getaddrinfo` reservó recursos en algún lado (posiblemente el heap).
     * Es nuestra obligación liberar dichos recursos cuando no los necesitamos
     * más.
     *
     * La manpage dice q debemos usar `freeaddrinfo` para ello y
     * así lo hacemos.
     * */
    freeaddrinfo(result);

    return 0;
}
