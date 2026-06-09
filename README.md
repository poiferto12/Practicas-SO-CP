# Prácticas de Sistemas Operativos y Paralelismo y Concurrencia

Repositorio con ejercicios y proyectos académicos desarrollados en las asignaturas de Sistemas Operativos y Paralelismo y Concurrencia del Grado en Ingeniería Informática.

## Contenidos

### Sistemas Operativos

Desarrollo progresivo de una shell en C y de distintas funcionalidades relacionadas con la interacción con el sistema operativo:

* Procesamiento e historial de comandos.
* Gestión de archivos, directorios y descriptores.
* Creación y control de procesos.
* Gestión de memoria dinámica.
* Uso de memoria compartida.
* Mapeo de archivos en memoria mediante `mmap`.
* Uso de llamadas al sistema en entornos Linux.

### Paralelismo y Concurrencia

Implementación de ejercicios de paralelismo, sincronización y comunicación entre procesos:

* Paralelización de algoritmos en C.
* Comparación entre versiones secuenciales y paralelas.
* Generación paralela del conjunto de Mandelbrot.
* Hilos POSIX.
* Mutex recursivos y mutex de lectores/escritores.
* Semáforos y resolución del problema de los barberos dormidos.
* Comunicación mediante paso de mensajes en Erlang.
* Implementación de topologías de procesos en anillo y línea.

## Compilación

La mayoría de las prácticas en C incluyen un archivo `Makefile`:

```bash
cd ruta/de/la/practica
make
./nombre_del_ejecutable
```

Para los ejercicios en Erlang:

```bash
erlc archivo.erl
erl
```

Dentro del intérprete de Erlang:

```erlang
modulo:funcion().
```

## Aviso

Este repositorio se publica únicamente como muestra del trabajo realizado. No debe utilizarse para entregar total o parcialmente estas soluciones en asignaturas académicas.
