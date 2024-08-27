#ifndef HTTP_PROTOCOL_H
#define HTTP_PROTOCOL_H

#include "socket.h"
#include <string>
#include <sstream>

/*
 * Versión simplificada del protocolo HTTP desde el punto de vista
 * de un cliente HTTP (no del server).
 * */
class HTTPProtocol {
    private:
    const std::string hostname;
    Socket skt;

    public:
    /*
     * `HTTPProtocol` establece automáticamente una conexión
     * con el servidor remoto.
     *
     * Si tal conexión falla, lanzará una excepción.
     *
     * Algunas implementaciones de protocolos reciben por parámetro
     * el socket ya creado y otras lo crean dentro.
     *
     * Las primeras permiten recibir objetos similares a un socket
     * (misma interfaz) pero distintos lo que permite reutilizar
     * el código del protocolo independientemente de la tecnología
     * de comunicación.
     *
     * En esta implementación de `HTTPProtocol` usa la segunda categoría:
     * `HTTPProtocol` puede o bien crear su propio socket o recibirlo como
     * parámetro.
     * */
    explicit HTTPProtocol(
            const std::string& hostname,
            const std::string& servname = "http");

    /*
     * Constructor de `HTTPProtocol` que recibe un `Socket` *ya* conectado.
     *
     * Notar que esta implementación va un poco en contra de RAII ya que
     * ahora es posible que el usuario incorrectamente pase un socket
     * invalido o que haya sido usado parcialmente (por ejemplo que algunos bytes
     * fueron leido sin que `HTTPProtocol` lo sepa, lo cual rompe el protocolo).
     *
     * Al permitirle al usuario que pase un Socket explícitamente,
     * le dejamos que él decida que tipo de implementación concreta
     * de `Socket` quiere usar. Por ahora hay una sola (la clase `Socket`)
     * pero podría haber varias:
     *  - un socket que use QUIC over UDP/IP en vez de TCP/IP (cambiaste la tecnología)
     *  - un socket que encripte todo el tráfico con TLS o SSL (agregaste funcionalidad)
     *  - un socket de juguete que no envie/reciba nada para testing (permitis el testing).
     *
     * Cambiar la implementación te da mucha mas flexibilidad pero con un costo:
     * tendrás que usar el heap, polimorfismo y punteros y/o templates.
     * Nada de eso lo veremos en este hands-on, esto queda para más adelante.
     * */
    HTTPProtocol(Socket&& skt, const std::string& hostname);

    /*
     * API sincrónica para GET.
     *
     * Con `HTTPProtocol::async_get` se puede pedir un recurso.
     * El método retornara luego de haber enviado el pedido pero
     * no esperara a la respuesta.
     *
     * Con `HTTPProtocol::wait_response` se puede esperar a
     * la respuesta.
     *
     * Esta implementación asincrónica permite al caller enviar
     * tantos pedidos como el desee sin bloquearse a la espera
     * de recibir algún respuesta.
     *
     * Típicamente el caller usara un thread para enviar los pedidos
     * y otro thread para recibir las respuesta.
     *
     * Véase `HTTPProtocol::get` para una implementación sincrónica de GET
     * */
    void async_get(const std::string& resource);
    std::string wait_response(bool include_headers=false);

    /*
     * API sincrónica para GET.
     *
     * Con `HTTPProtocol::get` se puede pedir un recurso.
     * El método retornara luego de haber enviado el pedido y haber recibido
     * la respuesta.
     *
     * Si se quiere enviar múltiples pedidos sin bloquearse al esperar
     * las respuestas y recibirlas concurrentemente (posiblemente en otro
     * thread), véase `HTTPProtocol::async_get` y `HTTPProtocol::wait_response`.
     * */
    std::string get(const std::string& resource, bool include_headers=false);

    /*
     * No queremos permitir que alguien haga copias
     * */
    HTTPProtocol(const HTTPProtocol&) = delete;
    HTTPProtocol& operator=(const HTTPProtocol&) = delete;

    /*
     * Queremos permitir mover a los objetos (move semantics).
     *
     * Como todos nuestros atributos son movibles, la implementación
     * por default de C++ nos alcanza.
     * */
    HTTPProtocol(HTTPProtocol&&) = default;
    HTTPProtocol& operator=(HTTPProtocol&&) = default;
};

#endif
