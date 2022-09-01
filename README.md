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

## Licencia

GPL v2

## Puedo usar este código en el Trabajo Práctico?

Si, pero tenes que decir de donde lo sacaste y respetar la licencia.

Ademas tenes que recordad que este código es parte de un tutorial y
aunque trata de ser exacto y correcto, no está libre de simplificaciones
que en el Trabajo Práctico pueden ser exigidos.

En todo momento seras responsable ya que la inclusion parcial o total
de este código y su uso es decisión tuya.
