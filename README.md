# Traductor de JSON a XML

Este proyecto implementa un **traductor de JSON simplificado a XML** en lenguaje C, usando un **analizador sintáctico descendente recursivo** y aplicando una estrategia de recuperación de errores mediante **Panic Mode con sincronización**.

## Autores:

1. Ian Delvalle - 5550211
2. Matias Gaona - 4553979

## Estructura

- `traductor.c`: Código fuente principal del traductor.
- `fuente.json`: Ejemplo de archivo JSON de entrada.
- `salida.xml`: Archivo de salida generado con la traducción a XML.
- `output.xml`: Archivo de referencia con la salida esperada.

## Uso

### Compilación

```bash
gcc traductor.c -o traductor
```

### Ejecución

```bash
./traductor fuente.json output.xml
```

El programa leerá `fuente.json`, validará su sintaxis y generará `output.xml` con la traducción correspondiente.

## Características

- Traducción directa de estructuras JSON a etiquetas XML.
- Traducción de arreglos como listas de `<item>...</item>`.
- Manejo de errores sintácticos sin detener la ejecución (Panic Mode).
- Soporte para tipos: cadenas, números, booleanos, null, objetos y arreglos.

## Requisitos

- Compilador `gcc` (Linux o Windows vía `djgpp`).
- Compatible con cualquier sistema que soporte C ANSI.

> Proyecto desarrollado como parte de la Tarea 3 - Traducción Dirigida por Sintaxis.
