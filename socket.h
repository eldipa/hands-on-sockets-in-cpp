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
     * Construye el socket pasándole directamente el file descriptor.
     * */
    explicit Socket(int skt);

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
 * Deshabilitamos el constructor por copia y operador asignación por copia
 * ya que no queremos que se puedan copiar objetos `Socket`.
 *
 * Se podrían copiar?, no. Si bien un `Socket` el un `int` y un `bool` y seria
 * trivial copiarlo, conceptualmente ese `int` no es meramente un número
 * sino un file descriptor, un identificador que tiene asociado varios
 * recursos del lado del sistema operativo.
 *
 * Copiar un `int` no implica que esos recursos son copiados (es como
 * copiar un puntero y pretender que el objeto apuntado se copie mágicamente).
 *
 * Más aun no tiene sentido pensar en copiar un socket. Un socket
 * representa una conexión algo q no se puede copiar.
 *
 * Lo mismo pasa con otros recursos. Un archivo referenciado por `FILE*`.
 * Copiar ese puntero no implica que realmente tendrás una copia del archivo
 * en tu disco rígido.
 *
 * Por eso deshabilitamos la copia. No tiene sentido.
 * */
Socket(const Socket&) = delete;
Socket& operator=(const Socket&) = delete;

/*
 * Hacemos que el `Socket` sea movible.
 * */
Socket(Socket&&);
Socket& operator=(Socket&&);

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
 * Retorna 0 si se cerro el socket,
 * o positivo que indicara cuantos bytes realmente se enviaron/recibieron.
 *
 * Si hay un error se lanza una excepción.
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
 * Si hay un error se lanza una excepción.
 *
 * Si no hubo un error pero el socket se cerro durante el envio/recibo
 * de los bytes y algunos bytes fueron enviados/recibidos,
 * se lanza también una excepción.
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
 * Acepta una conexión entrante y retorna un nuevo socket
 * construido a partir de ella.
 *
 * En caso de error, se lanza una excepción.
 * */
Socket accept();

/*
 * Cierra la conexión ya sea parcial o completamente.
 * Lease manpage de `shutdown`
 * */
void shutdown(int how);

/*
 * Cierra el socket. El cierre no implica un `shutdown`
 * que debe ser llamado explícitamente.
 * */
int close();

/*
 * Destruye el socket. Si aun esta conectado,
 * se llamara a `Socket::shutdown` y `Socket::close`
 * automáticamente.
 * */
~Socket();
};
#endif

