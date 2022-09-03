#ifndef LIB_ERROR_H
#define LIB_ERROR_H

#include <exception>

/*
 * Clase para encapsular el `errno` de C, "el ultimó error".
 * Es una excepción genérica que se encarga de los detalles
 * de decodificar el `errno` en un mensaje más entendible.
 * */
class LibError : public std::exception {
    char msg_error[256];

    public:
    /*
     * Dado que `errno` es una variable global y puede ser modificada por
     * cualquier función, hay que hacer una copia apenas se haya detectado
     * el error, para luego lanzar `LibError` o bien lanzar `LibError` apenas
     * ya se haya detectado el error.
     *
     * El constructor `LibError` es variadico y recibe, ademas del `errno`,
     * un format-string (como `printf`) y cero o más argumentos que
     * formaran parte del mensaje.
     *
     * int ret = foo();
     * if (ret == -1)
     *      throw LibError(errno, "The function %s has failed: ", "foo");
     *  */
    LibError(int error_code, const char* fmt, ...) noexcept;

    virtual const char* what() const noexcept;

    virtual ~LibError();
};

#endif
