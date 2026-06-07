# Proyecto 2 - AED: Suffix Tree (Ukkonen)

Buscador indexado de documentos basado en un **Suffix Tree** construido con el
**algoritmo de Ukkonen** (O(n)), comparado contra una **busqueda ingenua**
(busqueda directa en texto). Estructura asignada: **Suffix Tree** (Opcion D).

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
include/   SuffixTree.h  NaiveSearch.h  TextNormalizer.h  DocumentLoader.h
src/       SuffixTree.cpp NaiveSearch.cpp TextNormalizer.cpp DocumentLoader.cpp main.cpp
data/      sample.txt
```

El nucleo (Suffix Tree de Ukkonen) esta implementado **desde cero**: nodos
propios, suffix links, active point, remainder y caracter terminal unico.
`std::unordered_map` se usa solo como adyacencia auxiliar de aristas dentro de
cada nodo (no reemplaza el nucleo algoritmico), conforme a la regla 4.

## Dependencias

- **SFML 2.5.x** (visualizacion). En macOS: `brew install sfml@2`.
- **poppler-cpp** (OPCIONAL, para leer PDF). En macOS: `brew install poppler pkg-config`.
  Si no esta, el proyecto compila igual y soporta solo `.txt`.
- CMake >= 3.28, compilador C++20.

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

Nota sobre PDF: el indice se construye sobre el **texto extraido**, no sobre el
binario. Los PDFs escaneados (imagen) requeririan OCR y no estan soportados.

## Experimentos (punto 6)

Comparacion **Suffix Tree (Ukkonen) vs busqueda ingenua** con 3 tamanos de texto.
Justificacion de tamanos: 100k = demo basica, 500k = intermedia, 1M = avanzada
(los que sugiere el enunciado). Corpus sintetico reproducible (semilla fija) de
palabras repetidas, asi los patrones tienen ocurrencias. Por cada tamano se
muestrean 40 patrones de longitudes m = 4, 8 y 16. Maquina: macOS (Apple
Silicon), build Release-like.

Reproducir:

```bash
cmake --build build
./build/benchmark            # genera data/benchmark_results.csv
```

| n (chars) | m | construccion (ms) | consulta ST (us) | consulta ingenua (us) | nodos ST | comp. ST | comp. ingenua | speedup |
|----------:|--:|------------------:|-----------------:|----------------------:|---------:|---------:|--------------:|--------:|
| 100 000   | 4 | 107.2  | 0.26 | 1 473  | 3.1 | 4  | 108 848   | 5 698x  |
| 100 000   | 8 | 107.2  | 0.44 | 1 491  | 5.0 | 8  | 109 919   | 3 377x  |
| 100 000   |16 | 107.2  | 0.66 | 1 484  | 6.6 | 16 | 109 676   | 2 245x  |
| 500 000   | 4 | 573.6  | 0.28 | 7 464  | 3.4 | 4  | 549 491   | 26 962x |
| 500 000   | 8 | 573.6  | 0.42 | 7 519  | 4.8 | 8  | 554 029   | 17 862x |
| 500 000   |16 | 573.6  | 0.73 | 7 604  | 7.6 | 16 | 557 103   | 10 472x |
| 1 000 000 | 4 | 1 258  | 0.27 | 14 999 | 3.3 | 4  | 1 103 940 | 55 875x |
| 1 000 000 | 8 | 1 258  | 0.45 | 14 757 | 5.1 | 8  | 1 093 534 | 33 098x |
| 1 000 000 |16 | 1 258  | 0.73 | 14 790 | 7.7 | 16 | 1 095 068 | 20 148x |

### Interpretacion

- **Construccion O(n):** 107 -> 573 -> 1258 ms al pasar de 100k a 1M (≈ lineal;
  el ligero sobrecosto viene del factor constante del hash de hijos por nodo).
- **Consulta del Suffix Tree O(m), independiente de n:** el tiempo (~0.26-0.73 us)
  y las comparaciones (= exactamente m) **no cambian** al crecer el texto; solo
  dependen de la longitud del patron.
- **Consulta ingenua O(n):** tiempo y comparaciones (~n) crecen de forma lineal
  con el tamano del texto (1.5k -> 7.5k -> 15k us; el patron casi no influye
  porque la mayoria de posiciones falla en el primer caracter).
- **Speedup creciente:** la ventaja del indice **aumenta con n** (de ~2 000-5 700x
  en 100k a ~20 000-55 000x en 1M). Es el costo de construir el indice una vez a
  cambio de consultas casi gratis: util cuando se consulta muchas veces el mismo
  documento (como un Ctrl+F indexado).
- **Efecto de m:** a mayor m, mas profundo se recorre el arbol (mas nodos/comparaciones)
  y menos ocurrencias, pero la consulta sigue en el orden del microsegundo.

Resultados crudos en [data/benchmark_results.csv](data/benchmark_results.csv).

## Datasets

Para los experimentos se usaran textos con contenido seleccionable (libros de
**OpenStax**, https://openstax.org/, u otros `.txt` largos). La fuente y licencia
de cualquier dataset externo se citaran aqui cuando se incorpore.
