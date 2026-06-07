# Reporte — Proyecto 2 (AED): Suffix Tree con Ukkonen

**Curso:** CS2023 — Algoritmos y Estructuras de Datos
**Estructura asignada:** Suffix Tree (Opción D)
**Aplicación:** Buscador indexado de documentos (TXT / PDF), tipo *Ctrl + F* pero
resolviendo cada consulta sobre un índice construido una sola vez.

---

## 1. Introducción y problema

Queremos buscar un patrón `P` (palabra o frase) dentro de un documento `T` y
encontrar **todas** sus ocurrencias. La solución **ingenua** recorre el texto
posición por posición comparando con el patrón: cuesta `O(n·m)` en el peor caso
(`n = |T|`, `m = |P|`) y **repite todo el trabajo en cada consulta**.

La idea del proyecto es invertir ese costo: construimos **una vez** un índice
—un **Suffix Tree**— y luego cada búsqueda cuesta solo `O(m + k)` (con `k` =
número de ocurrencias), **independiente del tamaño del documento**. Esto es ideal
para el caso real de un buscador: se indexa el documento una vez y se consulta
muchas veces.

---

## 2. El Suffix Tree

Un **Suffix Tree** de un texto `T` de longitud `n` es un árbol que contiene
**todos los sufijos** de `T` como caminos desde la raíz hasta una hoja.

### Invariantes

1. Cada **arista** se rotula con una subcadena de `T`; se almacena como un par de
   índices `[start, end]` (no se copian caracteres), así el árbol ocupa `O(n)`.
2. Las aristas que salen de un mismo nodo empiezan con **caracteres distintos**
   (un nodo tiene a lo sumo un hijo por carácter).
3. Cada nodo interno (que no sea la raíz) tiene **al menos 2 hijos**.
4. Hay exactamente **`n` hojas** (una por sufijo), y cada hoja guarda el
   **índice de inicio** del sufijo que representa.
5. Cada nodo interno tiene un **suffix link** (ver §3.4).

### Carácter terminal único

Si un sufijo es prefijo de otro (p. ej. en `banana`, `"a"` es prefijo de
`"ana"`), no terminaría en una hoja propia y el árbol quedaría mal formado. Para
evitarlo se añade un **carácter terminal único** `$` que no aparece en el texto.
Así **ningún sufijo es prefijo de otro** y cada uno termina en su propia hoja.

> En la implementación, el terminal es el byte `'\x01'` (`SuffixTree::kTerminal`).
> El normalizador garantiza que el texto nunca lo contenga.

---

## 3. Construcción con el algoritmo de Ukkonen — `O(n)`

Ukkonen construye el árbol de forma **incremental** y **on-line**: procesa el
texto carácter por carácter, manteniendo en todo momento el suffix tree del
prefijo ya leído. Su costo total es **lineal**, `O(n)`.

### 3.1 Fases y extensiones

- El algoritmo tiene `n` **fases**, una por carácter `T[i]`.
- En la fase `i` hay que asegurar que **todos los sufijos** del prefijo
  `T[0..i]` estén en el árbol. Cada uno de esos sufijos pendientes es una
  **extensión**.

Hecho ingenuamente esto sería `O(n²)` o peor. Ukkonen lo vuelve `O(n)` con cuatro
ideas: el **truco del fin global**, el **active point**, el **remainder** y los
**suffix links**.

### 3.2 Truco del *fin global* (extensión de hojas en O(1))

Toda hoja representa un sufijo que crece en cada fase. En lugar de actualizar el
`end` de cada hoja una por una, todas comparten una variable global `leafEnd`
que se incrementa una vez por fase. Así la **"Regla 1"** (extender una hoja) es
implícita y cuesta `O(1)`.

### 3.3 Active point y remainder

- **`remainder`**: cuántos sufijos quedan **pendientes** de insertar. Sube `+1`
  al inicio de cada fase y baja `-1` cada vez que insertamos uno. Cuando un
  sufijo "ya está" (Regla 3), no se inserta y `remainder` se conserva: esos
  sufijos quedan representados *implícitamente* y se materializarán después.
- **Active point** `(active_node, active_edge, active_length)`: indica desde
  dónde continúa la siguiente inserción, evitando volver a recorrer desde la raíz.
  - `active_node`: nodo actual,
  - `active_edge`: qué arista (por su primer carácter) estamos recorriendo,
  - `active_length`: cuántos caracteres llevamos avanzados sobre esa arista.

Las dos reglas de inserción dentro de una fase:

- **Regla 2 (crear/dividir):** si desde el active point el siguiente carácter
  **no** está, se crea una hoja nueva; si caemos en mitad de una arista, esta se
  **divide** (split) creando un nodo interno. Se decrementa `remainder`.
- **Regla 3 (ya está):** si el siguiente carácter **sí** está en el camino,
  solo avanzamos `active_length` y **terminamos la fase** (los sufijos restantes
  quedan implícitos). Es la "show-stopper rule".

Tras una Regla 2, el active point se actualiza:
- si `active_node` es la raíz y `active_length > 0`: `active_length--` y se
  recoloca `active_edge`;
- si no es la raíz: se salta por el **suffix link**.

### 3.4 Suffix links

Un **suffix link** conecta un nodo interno que representa la cadena `cα` (un
carácter `c` seguido de `α`) con el nodo que representa `α`. Permiten, tras
insertar el sufijo que empieza en `c`, **saltar instantáneamente** al lugar
donde toca insertar el siguiente sufijo (el que empieza en `α`) sin recorrer
desde la raíz. Son la pieza que hace que el total sea `O(n)` amortizado.

### 3.5 Ejemplo paso a paso: construir el árbol de `banana$`

Texto con índices: `b(0) a(1) n(2) a(3) n(4) a(5) $(6)`.
Notación: `R` = raíz; active point = `(nodo, arista, longitud)`.

| Fase | Carácter | Qué ocurre | remainder (fin) | active point (fin) |
|-----:|:--------:|:-----------|:---------------:|:-------------------|
| 0 | `b` | No hay arista `b` en R → **Regla 2**: hoja `b…` (sufijo 0) | 0 | `(R,-,0)` |
| 1 | `a` | No hay arista `a` → hoja `a…` (sufijo 1) | 0 | `(R,-,0)` |
| 2 | `n` | No hay arista `n` → hoja `n…` (sufijo 2) | 0 | `(R,-,0)` |
| 3 | `a` | Ya hay arista `a` (**Regla 3**): avanzamos | 1 | `(R, a, 1)` |
| 4 | `n` | Sobre la arista `a` el siguiente es `n` = `T[4]` (**Regla 3**) | 2 | `(R, a, 2)` |
| 5 | `a` | El siguiente sobre la arista es `a` = `T[5]` (**Regla 3**) | 3 | `(R, a, 3)` |
| 6 | `$` | Tres **splits** + creación de suffix links (ver abajo) | 0 | `(R,-,0)` |

Las fases 0–2 solo crean hojas. Las fases 3–5 disparan la **Regla 3**: el
patrón `ana` se va leyendo *implícitamente* sobre la arista `a…`, sin crear
nodos, mientras `remainder` se acumula (1 → 2 → 3). Toda la magia ocurre en la
fase 6, cuando llega `$` y hay que materializar esos 4 sufijos pendientes:

**Fase 6 (`$`), `remainder = 4`:**

1. **Extensión 1** — active `(R, a, 3)`. Sobre la arista `a…` el carácter
   siguiente sería `T[4]='n'`, pero llega `$` → **mismatch → split**. Se crea el
   nodo interno **S1** con rótulo `ana`; cuelga de él la antigua hoja (`na$`) y
   una nueva hoja `$`. `remainder → 3`. `active = (R, n, 2)`.
2. **Extensión 2** — active `(R, n, 2)`. Sobre la arista `n…` (`nana$`) el
   siguiente sería `n` pero llega `$` → **split**. Se crea **S2** con rótulo
   `na`. Se enlaza `S1.link → S2`. `remainder → 2`. `active = (R, a, 1)`.
3. **Extensión 3** — active `(R, a, 1)`. La arista `a` lleva a **S1**(`ana`);
   en la posición 1 el siguiente es `n` pero llega `$` → **split**. Se crea
   **S3** con rótulo `a`. Se enlaza `S2.link → S3`. `remainder → 1`.
   `active = (R, -, 0)`.
4. **Extensión 4** — active `(R, -, 0)`. No hay arista `$` en R → **Regla 2**:
   hoja `$` (sufijo 6). Se enlaza `S3.link → R`. `remainder → 0`. Fin.

**Cadena de suffix links resultante:** `S1(ana) → S2(na) → S3(a) → R`.
Esa cadena es exactamente lo que evita recorrer desde la raíz en cada extensión
y da el costo lineal.

**Árbol final** (entre comillas el rótulo de la arista; `→` hijo):

```
R
├─ "banana$"            (hoja, sufijo 0)
├─ "a" ─ S3
│        ├─ "na" ─ S1
│        │         ├─ "na$"   (hoja, sufijo 1)  → "anana$"
│        │         └─ "$"     (hoja, sufijo 3)  → "ana$"
│        └─ "$"            (hoja, sufijo 5)      → "a$"
├─ "na" ─ S2
│         ├─ "na$"        (hoja, sufijo 2)       → "nana$"
│         └─ "$"          (hoja, sufijo 4)       → "na$"
└─ "$"                    (hoja, sufijo 6)
```

Se cumplen las invariantes: 7 hojas (sufijos 0..6), cada nodo interno con ≥2
hijos, y ningún sufijo es prefijo de otro gracias a `$`.

---

## 4. Operaciones y complejidad

Tras construir el árbol, una pasada en post-orden (`finalize()`, **iterativa**
para no desbordar la pila con textos largos) calcula en cada nodo:

- el **índice de sufijo** de las hojas, y
- `leafCount` = número de hojas del subárbol → permite **contar ocurrencias en
  `O(1)`** una vez localizado el patrón.

Buscar un patrón = recorrer su camino desde la raíz comparando carácter a
carácter sobre las aristas. Si el camino existe, el nodo donde termina tiene en
su subárbol **todas** las ocurrencias (las hojas).

| Operación | Implementación | Complejidad |
|-----------|----------------|-------------|
| `buildUkkonen(T)` | construcción incremental | `O(n)` |
| `contains(P)` | recorrer el camino del patrón | `O(m)` |
| `countOccurrences(P)` | localizar + `leafCount` | `O(m)` |
| `findOccurrences(P)` | localizar + recoger `k` hojas | `O(m + k)` |
| Búsqueda **ingenua** | escaneo directo del texto | `O(n·m)` peor; `~O(n)` promedio |

> Nota: los hijos de cada nodo se guardan en un `std::unordered_map<char,int>`
> **como adyacencia auxiliar** (lookup `O(1)` promedio). Esto **no** reemplaza el
> núcleo —la construcción de Ukkonen, los suffix links y el active point son
> propios—, conforme a la regla 4 del enunciado.

---

## 5. La aplicación

**Pipeline:** `PDF/TXT → extracción → normalización → Suffix Tree (Ukkonen) →
búsqueda → ocurrencias → resaltado`.

- **Carga** (`DocumentLoader`): `.txt` directo y `.pdf` con **poppler-cpp**
  (extrae el texto seleccionable; el árbol se construye sobre el texto, no sobre
  el binario). Soporte de PDF opcional en CMake.
- **Normalización** (`TextNormalizer`): minúsculas, colapso de espacios, *trim*;
  garantiza que el texto no contenga el terminal reservado.
- **Núcleo** (`SuffixTree`): la estructura descrita arriba.
- **Comparación** (`NaiveSearch`): búsqueda directa instrumentada (cuenta
  comparaciones) para la comparación experimental.
- **Visualización** (SFML): muestra el documento, una barra de búsqueda, las
  **ocurrencias resaltadas**, el panel de métricas (coincidencias, nodos
  visitados, comparaciones y tiempos de ambos métodos) y la **ruta del patrón
  recorrida en el árbol** (cadena raíz → arista → nodo, con el índice de sufijo
  en las hojas).

Controles GUI: escribir + **Enter** busca; flechas/rueda hacen scroll; **Esc**
cierra.

---

## 6. Comparación experimental (punto 6)

Corpus sintético reproducible (semilla fija) de palabras repetidas; por cada
tamaño se muestrean 40 patrones de longitudes `m = 4, 8, 16`. Tamaños
justificados por el enunciado: 100k (demo), 500k (intermedia), 1M (avanzada).
Reproducir con `./build/benchmark` (genera `data/benchmark_results.csv`).

| n (chars) | m | construcción (ms) | consulta ST (µs) | consulta ingenua (µs) | nodos ST | comp. ST | comp. ingenua | speedup |
|----------:|--:|------------------:|-----------------:|----------------------:|---------:|---------:|--------------:|--------:|
| 100 000   | 4 | 107.2  | 0.26 | 1 473  | 3.1 | 4  | 108 848   | 5 698× |
| 100 000   | 8 | 107.2  | 0.44 | 1 491  | 5.0 | 8  | 109 919   | 3 377× |
| 100 000   |16 | 107.2  | 0.66 | 1 484  | 6.6 | 16 | 109 676   | 2 245× |
| 500 000   | 4 | 573.6  | 0.28 | 7 464  | 3.4 | 4  | 549 491   | 26 962× |
| 500 000   | 8 | 573.6  | 0.42 | 7 519  | 4.8 | 8  | 554 029   | 17 862× |
| 500 000   |16 | 573.6  | 0.73 | 7 604  | 7.6 | 16 | 557 103   | 10 472× |
| 1 000 000 | 4 | 1 258  | 0.27 | 14 999 | 3.3 | 4  | 1 103 940 | 55 875× |
| 1 000 000 | 8 | 1 258  | 0.45 | 14 757 | 5.1 | 8  | 1 093 534 | 33 098× |
| 1 000 000 |16 | 1 258  | 0.73 | 14 790 | 7.7 | 16 | 1 095 068 | 20 148× |

### Interpretación

- **Construcción `O(n)`:** 107 → 573 → 1258 ms al pasar de 100k a 1M (≈ lineal;
  el ligero sobrecosto es el factor constante del hash de hijos).
- **Consulta del Suffix Tree `O(m)`, independiente de `n`:** el tiempo
  (~0.26–0.73 µs) y las comparaciones (**= exactamente `m`**) **no cambian** al
  crecer el texto; solo dependen de la longitud del patrón.
- **Consulta ingenua `O(n)`:** tiempo y comparaciones (~`n`) crecen linealmente
  con el texto (1.5k → 7.5k → 15k µs). El patrón casi no influye porque la
  mayoría de posiciones falla en el primer carácter.
- **Speedup creciente:** la ventaja del índice **aumenta con `n`** (de
  ~2 000–5 700× en 100k a ~20 000–55 000× en 1M). Es el intercambio esperado:
  se paga la construcción una vez a cambio de consultas casi gratuitas.
- **Efecto de `m`:** a mayor `m` se recorre más profundo el árbol (más
  nodos/comparaciones) y hay menos ocurrencias, pero la consulta sigue en el
  orden del microsegundo.

---

## 7. Conclusiones

- El Suffix Tree construido con Ukkonen resuelve la búsqueda de patrones en
  `O(m + k)` por consulta, frente al `O(n·m)`/`O(n)` de la ingenua.
- La construcción `O(n)` se paga **una sola vez**; a partir de ahí las consultas
  son prácticamente independientes del tamaño del documento, lo que se confirma
  empíricamente (speedup de hasta ~55 000× en 1M de caracteres).
- La estructura encaja de forma natural en un buscador indexado de documentos
  (TXT/PDF), cumpliendo el objetivo del proyecto: demostrar que una estructura
  avanzada mejora de forma medible una solución ingenua.

---

## 8. Cómo compilar y ejecutar

Ver [README.md](README.md). Resumen:

```bash
cmake -S . -B build && cmake --build build
./build/PROYECTO2_AED documento.pdf     # GUI
./build/PROYECTO2_AED --console doc.txt issi banana   # validación en consola
./build/benchmark                        # experimentos
```

## 9. Referencias

- E. Ukkonen, *On-line construction of suffix trees*, Algorithmica, 1995.
- Dan Gusfield, *Algorithms on Strings, Trees, and Sequences*, 1997.
- Datasets sugeridos (citar si se usan): OpenStax (https://openstax.org/).
