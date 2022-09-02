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
struct resolver_t {
    struct addrinfo *result; // privado, no accedas a este atributo
    struct addrinfo *next; // privado, no accedas a este atributo
};

/* Inicializa la estructura y resuelve el dado nombre del host y servicio.
 *
 * Retorna 0 en caso de éxito, -1 en caso de error.
 * */
int resolver_init(
        struct resolver_t *self,
        const char* hostname,
        const char* servname);


/* Retorna si hay o no una dirección siguiente para testear.
 * Si la hay, se deberá llamar a `resolver_next()` para obtenerla.
 *
 * Si no la hay se puede asumir que el resolver está extinguido.
 * */
bool resolver_has_next(struct resolver_t *self);

/* Retorna la siguiente dirección para testear e internamente
 * mueve el iterador a la siguiente dirección.
 *
 * Si no existe una siguiente dirección el resultado es indefinido.
 * */
struct addrinfo* resolver_next(struct resolver_t *self);

/*
 * Libera los recursos.
 * */
void resolver_deinit(struct resolver_t *self);

#endif
