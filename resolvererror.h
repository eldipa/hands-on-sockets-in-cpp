#ifndef RESOLVER_ERROR_H
#define RESOLVER_ERROR_H

#include <exception>

/*
 * Clase que encapsula un "gai" error. Vease getaddrinfo()
 * */
class ResolverError : public std::exception {
    private:
    int gai_errno;

    public:
    explicit ResolverError(int gai_errno);

    virtual const char* what() const noexcept;

    /*
     * Siempre que heredemos debemos hacer a nuestros
     * destructores polimórficos (`virtual`)
     *
     * Esto garantiza que será llamado.
     * */
    virtual ~ResolverError();
};
#endif
