# Sockets en C++

Este proyecto esta diseñado para que lo leas commit por commit, desde el
más antiguo al más actual.

En el mensaje de cada commit encontrar información de los cambios hechos
y de algunos puntos adicionales que no están en el código.

Para facilitarte la navegación entre los commits podes moverte al
siguiente commit, al anterior commit o al inicio con `make next-commit`,
`make prev-commit` y `make first-commit`. Podes ver el mensaje del
commit en el cual estas parado y su diff/patch con `git show`.

Ademas de los mensajes de cada commit te encontraras
con que el código fuente está documentado bloque a bloque yendo más al
detalle.

Se implementan un resolvedor de nombres, un
cliente HTTP y un servidor echo.

Inicialmente las implementaciones serán en C, estarán simplificadas,
con un código poco robusto e incluso con errores.

Commit a commit se irán refactorizando en tipos de datos abstractos
(TDA) en C y con manejo de errores *"a la C"*.

Luego, se introducirán más refactors portando el código a C++ con
conceptos como move semantics, RAII, y manejo de errores con excepciones.

No creas que lo que veras solo aplica a C o C++. Conceptos *core* del
manejo de recursos como move semantics y RAII son aplicables a Rust.

Conceptos como abstracciones, TDAs y manejo de errores (con o sin
excepciones) son aplicables a cualquier lenguaje de programación.

Y por supuesto están los sockets.

Las comunicaciones por Internet están en todos lados. Aun cuando uses
librerías de alto nivel (API REST), saber como funcionan los sockets y
sus limitaciones te servirán para diseñar mejores aplicaciones.

## Como compilar / correr los tests?

Simplemente corre:

```shell
make
make tests
```

Necesitaras `g++` y `byexample`. Este ultimó lo podes instalar
corriendo:

```shell
pip install byexample
```

## Name Resolver

`resolve_name` recibe un hostname y resuelve via `getaddrinfo` la
dirección IPv4.

```shell
$ ./resolve_name localhost
IPv4: 127.0.0.1<...>
```

Acepta opcionalmente el nombre de un servicio que resuelve a un puerto
TCP.

```shell
$ ./resolve_name localhost http
IPv4: 127.0.0.1 (port 80)<...>
```

`getaddrinfo`, y por ende `resolve_name` aceptan direcciones IP y
puertos:

```shell
$ ./resolve_name 8.8.8.8 53
IPv4: 8.8.8.8 (port 53)<...>
```

Nombres inválidos o que no puedan resolverse por alguna falla
en la red (DNS) terminan en error:

```shell
$ ./resolve_name 127.0.0.1 esto-no-es-un-servicio
Host/service name resolution failed (getaddrinfo): <...>
```

`getaddrinfo` soporta perfectamente tanto IPv4 como IPv6,
asi como otros protocoles ademas de TCP como UDP.

Sin embargo `resolve_name` no tiene dicho soporte: se lo deja
al lector como challenge.

## Cliente HTTP

`client_http` es un mini cliente HTTP que se conecta a un servidor
(`www.google.com.ar`), le pide una página web y la imprime por pantalla.

```shell
$ ./client_http
Sent 75 bytes
Page:
HTTP/1.1 200 OK<...>
```

`client_http` es extraordinariamente simple y por lo tanto tiene varios
puntos a desarrollar.

Como challenges podes:

 - recibir por línea de comandos que página web obtener y de que server
(otro que no sea `www.google.com.ar`). (challenge fácil)
 - darle soporte a HTTPS (challenge difícil, requiere usar alguna lib)
 - darle soporte a HTTP/3 (challenge difícil, requiere usar alguna lib)

## Echo Server

`echo_server` es un mini servidor que acepta una única conexión y todo
lo que recibe de ella lo reenvía.

En una consola podes levantar el server en background:

```shell
$ ./echo_server 8080  &
[<job-id>] <pid>
```

Y luego enviar cosas con un `netcat`. Deberías ver que lo que envies se
te retorna.

<!--
$ sleep 0.5
-->

```shell
$ echo "hello world!" | nc 127.0.0.1 8080   # byexample: +stop-on-silence +stop-signal=interrupt
hello world!
```

<!--
$ kill -9 $(jobs -p) && wait        # byexample: +pass
-->

Tendrás que finalizar al `netcat` con un `ctrl-c` (interrupción)
para que cierre la conexión y el server, al detectar el cierre, finalize.

El servidor escucha en la primer dirección local libre. `getaddrinfo`
permite seleccionar una dirección en particular y así poner el servidor
a escuchar ahí. Se deja esto como challenge.

También, el servidor solo acepta a un único cliente. Se deja como
challenge darle soporte para múltiples clientes.

## Licencia

GPL v2

## Puedo usar este código en el Trabajo Práctico?

Si, pero tenes que decir de donde lo sacaste y respetar la licencia.

Ademas tenes que recordad que este código es parte de un tutorial y
aunque trata de ser exacto y correcto, no está libre de simplificaciones
que en el Trabajo Práctico pueden ser exigidos.

En todo momento seras responsable ya que la inclusion parcial o total
de este código y su uso es decisión tuya.

<!--
No debería ser necesario pero por si acaso matamos a todos los procesos
lanzados durante la ejecución del README.md
$ kill -9 $(jobs -p) && wait        # byexample: -skip +pass
-->
