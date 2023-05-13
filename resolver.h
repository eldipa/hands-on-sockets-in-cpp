#ifndef RESOLVER_H
#define RESOLVER_H

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

/*
 * "Resolvedor" de hostnames y service names.
 *
 * Por simplificación este TDA se enfocara solamente
 * en direcciones IPv4 para TCP.
 * */
class Resolver {
    private:
    struct addrinfo *result;
    struct addrinfo *_next;

    /*
     * Checkea que la lista de direcciones (result) sea "valida".
     *
     * No hace un checkeo muy exhaustivo, simplemente verifica que
     * no sea null.
     *
     * En un object RAII como este, el atributo result esta siempre
     * bien definido (distinto de -1) **salvo** cuando es movido.
     *
     * Este check es para ayudarte a detectar ese caso y lanzar una
     * excepción. No es lo más bonito del universo pero te dará una
     * pista de que puede estar andando mal.
     * */
    void chk_addr_or_fail() const;

    public:
/* Crea el objeto y resuelve el dado nombre del host y servicio.
 *
 * Si `is_passive` es `true` y `hostname` es `nullptr`,
 * las direcciones retornadas serán aptas para hacer un `bind`
 * y poner al socket en modo escucha para recibir conexiones.
 *
 * En caso de error se lanza una excepción.
 * */
Resolver(
        const char* hostname,
        const char* servname,
        bool is_passive);

/*
 * Deshabilitamos el constructor por copia y operador asignación por copia
 * ya que no queremos que se puedan copiar objetos `Resolver`.
 *
 * Se podrían copiar?, sí, pero no le veo mucha utilidad y prefiero
 * evitarlo.
 * */
Resolver(const Resolver&) = delete;
Resolver& operator=(const Resolver&) = delete;

/*
 * Hacemos que el `Resolver` sea movible.
 * */
Resolver(Resolver&&);
Resolver& operator=(Resolver&&);

/* Retorna si hay o no una dirección siguiente para testear.
 * Si la hay, se deberá llamar a `Resolver::next()` para obtenerla.
 *
 * Si no la hay se puede asumir que el resolver está extinguido.
 * */
bool has_next();

/* Retorna la siguiente dirección para testear e internamente
 * mueve el iterador a la siguiente dirección.
 *
 * Si no existe una siguiente dirección el resultado es indefinido.
 * */
struct addrinfo* next();

/*
 * Libera los recursos.
 * */
~Resolver();
};
#endif
