#include "http_protocol.h"

#include <iostream>
#include <exception>

/*
 * Este programa es un mini cliente HTTP que se conecta a un servidor
 * (www.google.com.ar) y le pide una página web y la imprime por pantalla.
 * */
int main(int argc, char *argv[]) { try {
    int ret = -1;

    if (argc != 1) {
        std::cerr << "Bad program call. Expected "
                  << argv[0]
                  << " without arguments.\n";
        return ret;
    }

    /*
     * Nos permitimos romper un poquito la separación de concerns
     * y dejamos al cliente HTTP que construya él el socket
     * para luego crear el protocolo.
     *
     * Salvo durante esta inicialización,
     * nuestro código no tendrá que lidiar ni saber de sockets ni del
     * protocolo HTTP.
     *
     * Nuestro código meramente usara los conceptos de alto nivel
     * y el como hablar con el server sera trabajo del protocolo.
     *
     * Mantene siempre el código separado!
     * */
    const std::string hostname = "www.google.com.ar";
    Socket skt(hostname.c_str(), "http");
    HTTPProtocol http(std::move(skt), hostname);

    /*
     * Enviamos el request GET pidiéndole al servidor (Google) que
     * nos de el recurso `"/"` (la página web principal).
     * */
    http.async_get("/");

    /*
     * Ahora recibimos la respuesta.
     * */
    std::cout << "Page:\n" << http.wait_response(true) << "\n";

    /*
     * Si llegamos hasta acá es por que no nos topamos con ningún error
     * por lo tanto return-code de esta función (y de este programa)
     * debe ser 0 (éxito).
     * */
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
