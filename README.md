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
- [ ] **Parte 2b** — Benchmark de 3 tamanos (100k / 500k / 1M caracteres).
- [ ] **Parte 3** — Extraccion de PDF (poppler-cpp) detras de `DocumentLoader`.

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
- CMake >= 3.28, compilador C++20.

## Compilar y ejecutar

```bash
cmake -S . -B build
cmake --build build
./build/PROYECTO2_AED                 # usa data/sample.txt
./build/PROYECTO2_AED archivo.txt     # tu propio documento
./build/PROYECTO2_AED archivo.txt issi banana   # con patrones propios
```

## Datasets

Para los experimentos se usaran textos con contenido seleccionable (libros de
**OpenStax**, https://openstax.org/, u otros `.txt` largos). La fuente y licencia
de cualquier dataset externo se citaran aqui cuando se incorpore.
