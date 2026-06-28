# Proyecto 2 - AED: Suffix Tree (Ukkonen)

Buscador indexado de documentos basado en un **Suffix Tree** construido con el
**algoritmo de Ukkonen** (O(n)), comparado contra una **busqueda ingenua**
(busqueda directa en texto). Estructura asignada: **Suffix Tree** (Opcion D).

> Explicacion completa del algoritmo (fases, suffix links, active point,
> remainder, ejemplo paso a paso) y resultados: ver **[REPORTE.md](REPORTE.md)**.

## Estado del proyecto

- [x] **Parte 1** — Nucleo: Suffix Tree con Ukkonen desde cero, busqueda ingenua,
  normalizador, loader de `.txt` y demo de consola con validacion/comparacion.
- [x] **Parte 2** — Visualizacion con SFML: render del documento con scroll,
  barra de busqueda, resaltado de ocurrencias, panel de metricas (Suffix Tree
  vs ingenua) y ruta del patron recorrida en el arbol.
- [x] **Parte 2b** — Benchmark de 3 tamanos (100k / 500k / 1M caracteres) con
  tabla de resultados e interpretacion (ver seccion "Experimentos").
- [x] **Parte 3** — Extraccion de PDF (poppler-cpp) detras de `DocumentLoader`:
  funciona con PDFs de texto seleccionable; soporte opcional en CMake.

### Controles de la aplicacion (GUI)

- Escribe un patron y presiona **Enter** para buscar.
- **Backspace** borra; **flechas / rueda / RePag-AvPag / Home-End** hacen scroll.
- **Esc** cierra.

## Estructura

```
include/   SuffixTree.h  ChildMap.h  NaiveSearch.h  TextNormalizer.h  DocumentLoader.h
src/       SuffixTree.cpp NaiveSearch.cpp TextNormalizer.cpp DocumentLoader.cpp App.cpp main.cpp benchmark.cpp
data/      (documentos de entrada .txt / .pdf)
```

El nucleo (Suffix Tree de Ukkonen) esta implementado **desde cero**: nodos
propios, suffix links, active point, remainder y caracter terminal unico.
**No se usa** `std::map` / `std::set` / `std::unordered_map` en ninguna parte
(regla 4): la adyacencia de hijos por caracter usa `ChildMap`, una tabla hash
propia (direccionamiento abierto + sondeo lineal, sobre `std::vector`).

## Dependencias

- **SFML 2.5.x / 2.6.x** (visualizacion). En Linux suele instalarse con
  `libsfml-dev`.
- **poppler-cpp** (OPCIONAL, para leer PDF). En Linux suele instalarse con
  `libpoppler-cpp-dev` y `pkg-config`.
  Si no esta, el proyecto compila igual y soporta solo `.txt`.
- CMake >= 3.28, compilador C++20.

Ejemplo en Debian/Ubuntu:

```bash
sudo apt update
sudo apt install cmake g++ libsfml-dev libpoppler-cpp-dev pkg-config
```

## Compilar y ejecutar

```bash
cmake -S . -B build
cmake --build build

# GUI (por defecto)
./build/PROYECTO2_AED                 # usa data/sample.txt
./build/PROYECTO2_AED documento.txt   # archivo de texto
./build/PROYECTO2_AED documento.pdf   # PDF con texto seleccionable (requiere poppler)

# Modo consola (valida el indice contra la busqueda ingenua)
./build/PROYECTO2_AED --console documento.pdf issi banana
```

Si tu distribucion coloca SFML en una ruta no estandar, define `SFML_DIR` antes
de ejecutar `cmake -S . -B build`.

Nota sobre PDF: el indice se construye sobre el **texto extraido**, no sobre el
binario. Los PDFs escaneados (imagen) requeririan OCR y no estan soportados.

## Experimentos (punto 6)

Comparacion **Suffix Tree (Ukkonen) vs busqueda ingenua** con 3 tamanos de texto.
Justificacion de tamanos: 100k = demo basica, 500k = intermedia, 1M = avanzada
(los que sugiere el enunciado). Corpus sintetico reproducible (semilla fija) de
palabras repetidas, asi los patrones tienen ocurrencias. Por cada tamano se
muestrean 40 patrones de longitudes m = 4, 8 y 16. La prueba se puede ejecutar
igual en Linux; la tabla siguiente es una referencia de resultados de ejemplo.

Reproducir:

```bash
cmake --build build
./build/benchmark            # genera data/benchmark_results.csv
```

| n (chars) | m | construccion (ms) | consulta ST (us) | consulta ingenua (us) | nodos ST | comp. ST | comp. ingenua | speedup |
|----------:|--:|------------------:|-----------------:|----------------------:|---------:|---------:|--------------:|--------:|
| 100 000   | 4 | 74.8  | 0.13 | 1 493  | 3.1 | 4  | 108 848   | 11 864x |
| 100 000   | 8 | 74.8  | 0.23 | 1 534  | 5.0 | 8  | 109 919   | 6 747x  |
| 100 000   |16 | 74.8  | 0.38 | 1 523  | 6.6 | 16 | 109 676   | 3 983x  |
| 500 000   | 4 | 426.1 | 0.13 | 7 687  | 3.4 | 4  | 549 491   | 57 570x |
| 500 000   | 8 | 426.1 | 0.23 | 7 812  | 4.8 | 8  | 554 029   | 34 430x |
| 500 000   |16 | 426.1 | 0.44 | 8 552  | 7.6 | 16 | 557 103   | 19 602x |
| 1 000 000 | 4 | 960.6 | 0.14 | 17 210 | 3.3 | 4  | 1 103 940 | 118 815x |
| 1 000 000 | 8 | 960.6 | 0.25 | 16 928 | 5.1 | 8  | 1 093 534 | 66 603x  |
| 1 000 000 |16 | 960.6 | 0.41 | 15 275 | 7.7 | 16 | 1 095 068 | 37 351x  |

### Interpretacion

- **Construccion O(n):** 75 -> 426 -> 961 ms al pasar de 100k a 1M (≈ lineal;
  la adyacencia de hijos usa ChildMap, una tabla hash propia con sondeo lineal,
  mas cache-friendly que std::unordered_map).
- **Consulta del Suffix Tree O(m), independiente de n:** el tiempo (~0.13-0.44 us)
  y las comparaciones (= exactamente m) **no cambian** al crecer el texto; solo
  dependen de la longitud del patron.
- **Consulta ingenua O(n):** tiempo y comparaciones (~n) crecen de forma lineal
  con el tamano del texto (1.5k -> 7.7k -> 17k us; el patron casi no influye
  porque la mayoria de posiciones falla en el primer caracter).
- **Speedup creciente:** la ventaja del indice **aumenta con n** (de ~4 000-11 800x
  en 100k a ~37 000-118 000x en 1M). Es el costo de construir el indice una vez a
  cambio de consultas casi gratis: util cuando se consulta muchas veces el mismo
  documento (como un Ctrl+F indexado).
- **Efecto de m:** a mayor m, mas profundo se recorre el arbol (mas nodos/comparaciones)
  y menos ocurrencias, pero la consulta sigue en el orden del microsegundo.

Resultados crudos en [data/benchmark_results.csv](data/benchmark_results.csv).

## Datasets

Para los experimentos se usaran textos con contenido seleccionable (libros de
**OpenStax**, https://openstax.org/, u otros `.txt` largos). La fuente y licencia
de cualquier dataset externo se citaran aqui cuando se incorpore.
