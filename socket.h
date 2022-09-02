#ifndef SOCKET_H
#define SOCKET_H

/*
 * TDA Socket.
 * Por simplificación este TDA se enfocará solamente
 * en sockets IPv4 para TCP.
 * */
struct socket_t {
    private:
    int skt;
    bool closed;

    /*
     * Inicializa el socket pasándole directamente el file descriptor.
     *
     * No queremos que el código del usuario este manipulando el file descriptor,
     * queremos q interactúe con él *solo* a través de `socket_t`.
     * */
    int init_with_file_descriptor(struct socket_t *peer, int skt);

    public:
/*
 * Inicializamos el socket tanto para conectarse a un servidor
 * (`socket_t::init_for_connection`) como para inicializarlo para ser usado
 * por un servidor (`socket_t::init_for_listen`).
 *
 * Muchas librerías de muchos lenguajes ofrecen una única formal de inicializar
 * los sockets y luego métodos (post-inicialización) para establecer
 * la conexión o ponerlo en escucha.
 *
 * Otras librerías/lenguajes van por tener una inicialización para
 * el socket activo y otra para el pasivo.
 *
 * Este TDA va por ese lado.
 *
 * Para `socket_t::init_for_connection`,  <hostname>/<servname> es la dirección
 * de la máquina remota a la cual se quiere conectar.
 *
 * Para `socket_t::init_for_listen`, buscara una dirección local válida
 * para escuchar y aceptar conexiones automáticamente en el <servname> dado.
 *
 * Ambas funciones retornan 0 si se pudo conectar/poner en escucha
 * o -1 en caso de error.
 * */
int init_for_connection(
        const char *hostname,
        const char *servname);
int init_for_listen(const char *servname);

/* `socket_t::sendsome` lee hasta `sz` bytes del buffer y los envía. La función
 * puede enviar menos bytes sin embargo.
 *
 * `socket_t::recvsome` por el otro lado recibe hasta `sz` bytes y los escribe
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
int sendsome(
        const void *data,
        unsigned int sz,
        bool *was_closed);
int recvsome(
        void *data,
        unsigned int sz,
        bool *was_closed);

/*
 * `socket_t::sendall` envía exactamente `sz` bytes leídos del buffer, ni más,
 * ni menos. `socket_t::recvall` recibe exactamente sz bytes.
 *
 * Si hay un error se retorna -1.
 *
 * Si no hubo un error pero el socket se cerro durante el envio/recibo
 * de los bytes y algunos bytes fueron enviados/recibidos,
 * se retorna -1 también.
 *
 * Si en cambio ningún byte se pudo enviar/recibir, se retorna 0.
 *
 * En ambos casos, donde el socket se cerró, `was_closed` es puesto a `true`.
 *
 * En caso de éxito se retorna la misma cantidad de bytes pedidos
 * para envio/recibo, lease `sz`.
 *
 * */
int sendall(
        const void *data,
        unsigned int sz,
        bool *was_closed);
int recvall(
        void *data,
        unsigned int sz,
        bool *was_closed);

/*
 * Acepta una conexión entrante e inicializa con ella el socket peer.
 *
 * Dicho socket peer debe estar *sin* inicializar y si `socket_t::accept` es
 * exitoso, se debe llamar a `socket_t::shutdown`, `socket_t::close` y
 * `socket_t::deinit` sobre él.
 *
 * Retorna -1 en caso de error, 0 de otro modo.
 * */
int accept(struct socket_t *peer);

/*
 * Cierra la conexión ya sea parcial o completamente.
 * Lease manpage de `shutdown`
 * */
int shutdown(int how);

/*
 * Cierra el socket. El cierre no implica un `shutdown`
 * que debe ser llamado explícitamente.
 * */
int close();

/*
 * Desinicializa el socket. Si aun esta conectado,
 * se llamara a `socket_t::shutdown` y `socket_t::close`
 * automáticamente.
 * */
void deinit();
};
#endif

