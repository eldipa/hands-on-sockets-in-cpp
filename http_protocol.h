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
     * En esta implementación de `HTTPProtocol` preferí que sea
     * el protocolo quien cree el socket.
     * */
    explicit HTTPProtocol(
            const std::string& hostname,
            const std::string& servname = "http");

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
