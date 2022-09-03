#include "http_protocol.h"

#include <string>
#include <sstream>

/*
 * Este es un ejemplo práctico de la Member Initialization List.
 *
 * En la definición de un constructor podemos llamar a los constructores
 * de nuestros atributos antes de que empiece el cuerpo del constructor.
 *
 * Esto nos permite inicializar objetos que no tenga un constructor
 * por default (como Socket) y objetos que van a ser constantes
 * (como `const hostname`).
 *
 * Es en la Member Initialization List donde tenemos control sobre la
 * construcción de los atributos.
 *
 * En el cuerpo del constructor lo único que podemos hacer es pisar
 * los valores.
 *
 * Es un error muy común toparse con este problema y al no saber como resolverlo
 * se terminan usando punteros.
 *
 * Por ejemplo:
 *
 *  class HTTPProtocol {
 *      Socket *skt
 *      ...
 *      HTTPProtocol(const std::string& hostname, const std::string& servname)
 *          this->skt = new Socket(hostname.c_str(), servname.c_str());
 *      }
 *  }
 *
 * Esto es totalmente incorrecto. Aunque funciona, al usar el heap
 * (operador `new`) el lifetime de skt ya no estará atado al lifetime
 * del objeto this y por lo tanto perdes los beneficios RAII.
 *
 * Evita todo lo posible esos diseños. Atá el lifetime de los atributos
 * al lifetime del objeto así, cuando instancies un objeto `HTTPProtocol`
 * en el stack y este se vaya de scope, no solo el sino también
 * sus atributos se destruirán automáticamente y no tendrás leaks.
 *
 * */
HTTPProtocol::HTTPProtocol(
        const std::string& hostname,
        const std::string& servname) :
    hostname(hostname),  /* <-- construimos un `const std::string` */
    skt(hostname.c_str(), servname.c_str()) /* <-- construimos un `Socket` */
{
    /* Esto *no* funcionaría ya que estaríamos pisando `skt` ya creado,
     * no construyéndolo desde cero.
     * */
    // this->skt = Socket(hostname, servname);


    /* Esto *no* funcionaría ya que estaríamos pisando `hostname`,
     * y `this->hostname` lo hicimos `const` y por ende, inmutable.
     * */
    // this->hostname = hostname;
}

void HTTPProtocol::async_get(const std::string& resource) {
    bool was_closed = false;

    /*
     * HTTP/1.1 es un protocolo de texto en donde el cliente (nosotros)
     * le hace un pedido a un servidor.
     *
     * Exactamente que es un "recurso" depende del servidor. Para Google
     * un recurso es una página web HTML. Para muchos servidores HTTP
     * sucede lo mismo pero no te confundas: nada te impide usar HTTP
     * para otros fines que no sean páginas web HTML.
     * */
    std::ostringstream request;
    request << "GET " << resource << " HTTP/1.1\r\n"
            << "Accept: */*\r\n"
               "Connection: close\r\n"
               "Host: " << this->hostname << "\r\n"
               "\r\n";

    /*
     * En C++ 20 podremos usar `view` para evitarnos una copia aquí.
     * */
    auto buf = request.str();
    skt.sendall(buf.data(), buf.size(), &was_closed);
}

std::string HTTPProtocol::wait_response(bool include_headers) {
    bool was_closed = false;

    /*
     * C++ tiene una librería bastante rica
     * para el manejo de strings.
     *
     * En este caso `std::ostringstream` permite ir guardando
     * un string parcial y obtenerlo completo con `partial.str`
     * al final.
     * */
    std::ostringstream partial;
    while (not was_closed) {
        char buf[512] = {0};
        skt.recvsome(buf, sizeof(buf) - 1, &was_closed);
        if (was_closed)
            break;

        for (int i = 0; buf[i]; ++i)
            if (not isascii(buf[i]))
                buf[i] = '@';

        partial << buf;
    }

    auto response = partial.str();
    if (include_headers) {
        return response;
    }

    /*
     * La intención de toda clase protocolo es la de abstraer al código
     * cliente los detalles del protocolo.
     *
     * Cosas como los headers de HTTP no son de interés para él.
     *
     * Salvo que se pida explícitamente, `HTTPProtocol::wait_response`
     * va a retornar el payload de la respuesta HTTP.
     *
     * El protocolo HTTP establece que dicho payload viene luego de los headers
     * separado por una línea vacía.
     *
     * En el caso de un server web, el payload es la página web,
     * típicamente HTML.
     *
     * `HTTPProtocol` podría ademas parsear los headers y ver si la respuesta
     * fue correcta (200 OK). Sino, el protocolo debería fallar (sea seteando
     * algún flag que el cliente deberá chequear o lanzando una excepción).
     *
     * En este caso asumiré que no hay errores.
     * */
    auto pos = response.find("\r\n\r\n");

    if (pos == std::string::npos)
        return "";
    else
        return response.substr(pos + 4);
}


std::string HTTPProtocol::get(
        const std::string& resource,
        bool include_headers) {
    async_get(resource);
    return wait_response(include_headers);
}
