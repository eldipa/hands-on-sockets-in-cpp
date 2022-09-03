#include "resolver.h"
#include "resolvererror.h"
#include "liberror.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

Resolver::Resolver(
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
     * */
    if (s != 0) {
        if (s == EAI_SYSTEM) {
            /*
             * Como `errno` es global y puede ser modificada por *cualquier* otra
             * función, es *importantísimo* copiarla apenas detectemos el error.
             *
             * En este caso, `LibError` lo hara por nosotros.
             */
            throw LibError(
                    errno,
                    "Name resolution failed for hostname '%s' y servname '%s'",
                    (hostname ? hostname : ""),
                    (servname ? servname : ""));

        } else {
            /*
             * La documentación de `getaddrinfo` dice que en este caso
             * debemos usar `gai_strerror` para obtener el mensaje de error.
             * */
            throw ResolverError(s);
        }
    }

    this->_next = this->result;
}

Resolver::Resolver(Resolver&& other) {
    /* Nos copiamos del otro resolver... */
    this->result = other.result;
    this->_next = other._next;

    /* ...pero luego le sacamos al otro resolver
     * el ownership del recurso.
     * Efectivamente el ownership pasó de él
     * a nosotros: el ownership se movió.
     *
     * En el caso de `Resolver` podemos marcar los
     * punteros como `nullptr`.
     * Tendremos que chequear en el destructor `~Resolver`
     * este caso y evitar llamar a `freeaddrinfo` si es `nullptr`.
     * */
    other.result = nullptr;
    other._next = nullptr;
}

Resolver& Resolver::operator=(Resolver&& other) {
    /* Si el usuario hace algo como tratar de moverse
     * a si mismo (`resolver = resolver;`) simplemente no hacemos
     * nada.
     * */
    if (this == &other)
        return *this;

    /* A diferencia del constructor por movimiento,
     * `this` (nosotros) es un resolver completamente creado
     * y debemos desinicializarlo primero antes de pisarle
     * el recurso con el que le robaremos al otro resolver (`other`)
     * */
    if (this->result)
        freeaddrinfo(this->result);

    /* Ahora hacemos los mismos pasos que en el move constructor */
    this->result = other.result;
    this->_next = other._next;
    other.result = nullptr;
    other._next = nullptr;

    return *this;
}

bool Resolver::has_next() {
    return this->_next != NULL;
}

struct addrinfo* Resolver::next() {
    struct addrinfo *ret = this->_next;
    this->_next = ret->ai_next;
    return ret;
}

Resolver::~Resolver() {
    /*
     * `getaddrinfo` reservó recursos en algún lado (posiblemente el heap).
     * Es nuestra obligación liberar dichos recursos cuando no los necesitamos
     * más.
     *
     * La manpage dice q debemos usar `freeaddrinfo` para ello y
     * así lo hacemos.
     * */
    if (this->result)
        freeaddrinfo(this->result);
}

