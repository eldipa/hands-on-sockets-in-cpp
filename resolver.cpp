#include "resolver.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

int resolver_t::init(
        const char* hostname,
        const char* servname,
        bool is_passive) {
    struct addrinfo hints;
    this->result = this->_next = nullptr;

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
    hints.ai_flags = is_passive ? AI_PASSIVE : 0;

    /* Obtengo la (o las) direcciones según el nombre de host y servicio que
     * busco
     *
     * De todas las direcciones posibles, solo me interesan aquellas que sean
     * IPv4 y TCP (según lo definido en hints)
     *
     * El resultado lo guarda en result que es un puntero al primer nodo
     * de una lista simplemente enlazada.
     * */
    int s = getaddrinfo(hostname, servname, &hints, &this->result);

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

    this->_next = this->result;
    return 0;
}

bool resolver_t::has_next() {
    return this->_next != NULL;
}

struct addrinfo* resolver_t::next() {
    struct addrinfo *ret = this->_next;
    this->_next = ret->ai_next;
    return ret;
}

void resolver_t::deinit() {
    /*
     * `getaddrinfo` reservó recursos en algún lado (posiblemente el heap).
     * Es nuestra obligación liberar dichos recursos cuando no los necesitamos
     * más.
     *
     * La manpage dice q debemos usar `freeaddrinfo` para ello y
     * así lo hacemos.
     * */
    freeaddrinfo(this->result);
}

