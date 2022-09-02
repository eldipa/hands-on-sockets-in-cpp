#ifndef SOCKET_H
#define SOCKET_H

/*
 * TDA Socket.
 * Por simplificación este TDA se enfocará solamente
 * en sockets IPv4 para TCP.
 * */
class Socket {
    private:
    int skt;
    bool closed;

    /*
     * Inicializa el socket pasándole directamente el file descriptor.
     *
     * No queremos que el código del usuario este manipulando el file descriptor,
     * queremos q interactúe con él *solo* a través de `Socket`.
     * */
    int init_with_file_descriptor(Socket *peer, int skt);

    public:
/*
 * Constructores para `Socket` tanto para conectarse a un servidor
 * (`Socket::Socket(const char*, const char*)`) como para ser usado
 * por un servidor (`Socket::Socket(const char*)`).
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
 * Para `Socket::Socket(const char*, const char*)`,  <hostname>/<servname> es la dirección
 * de la máquina remota a la cual se quiere conectar.
 *
 * Para `Socket::Socket(const char*)`, buscara una dirección local válida
 * para escuchar y aceptar conexiones automáticamente en el <servname> dado.
 *
 * En caso de error los constructores lanzaran una excepción.
 * */
Socket(
        const char *hostname,
        const char *servname);

explicit Socket(const char *servname);

/*
 * Constructor por default de `Socket`. Inicializa el socket como si no estuviese
 * conectado. La única utilidad de construir sockets con este constructor está
 * en pasarlos por `Socket::accept` y que sea dicho método quien lo conecte.
 * */
Socket();

/* `Socket::sendsome` lee hasta `sz` bytes del buffer y los envía. La función
 * puede enviar menos bytes sin embargo.
 *
 * `Socket::recvsome` por el otro lado recibe hasta `sz` bytes y los escribe
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
 * `Socket::sendall` envía exactamente `sz` bytes leídos del buffer, ni más,
 * ni menos. `Socket::recvall` recibe exactamente sz bytes.
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
 * Dicho socket peer debe estar *sin* inicializar y si `Socket::accept` es
 * exitoso, se debe llamar a `Socket::shutdown`, `Socket::close` y
 * `Socket::deinit` sobre él.
 *
 * Retorna -1 en caso de error, 0 de otro modo.
 * */
int accept(Socket *peer);

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
 * se llamara a `Socket::shutdown` y `Socket::close`
 * automáticamente.
 * */
void deinit();
};
#endif

