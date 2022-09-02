#ifndef SOCKET_H
#define SOCKET_H

/*
 * TDA Socket.
 * Por simplificación este TDA se enfocará solamente
 * en sockets IPv4 para TCP.
 * */
struct socket_t {
    int skt; // privado, no accedas a este atributo
    bool closed; // privado, no accedas a este atributo
};

/*
 * Inicializamos el socket tanto para conectarse a un servidor
 *
 * Retorna 0 en caso de éxito, -1 en caso de error.
 * */
int socket_init(
        struct socket_t *self,
        const char *hostname,
        const char *servname);

/* `socket_sendsome` lee hasta `sz` bytes del buffer y los envía. La función
 * puede enviar menos bytes sin embargo.
 *
 * `socket_recvsome` por el otro lado recibe hasta `sz` bytes y los escribe
 * en el buffer (que debe estar pre-allocado). La función puede recibir
 * menos bytes sin embargo.
 *
 * Si el socket detecto que la conexión fue cerrada, la variable
 * `was_closed` es puesta a `true`, de otro modo sera `false`.
 *
 * Retorna 0 si se cerro el socket, -1 si hubo un error
 * o positivo que indicara cuantos bytes realmente se enviaron/recibieron.
 *
 * Lease manpage de `send` y `recv`
 * */
int socket_sendsome(
        struct socket_t *self,
        const void *data,
        unsigned int sz,
        bool *was_closed);
int socket_recvsome(
        struct socket_t *self,
        void *data,
        unsigned int sz,
        bool *was_closed);

/*
 * Cierra la conexión ya sea parcial o completamente.
 * Lease manpage de `shutdown`
 * */
int socket_shutdown(struct socket_t *self, int how);

/*
 * Cierra el socket. El cierre no implica un `shutdown`
 * que debe ser llamado explícitamente.
 * */
int socket_close(struct socket_t *self);

/*
 * Desinicializa el socket. Si aun esta conectado,
 * se llamara a `socket_shutdown` y `socket_close`
 * automáticamente.
 * */
void socket_deinit(struct socket_t *self);

#endif

