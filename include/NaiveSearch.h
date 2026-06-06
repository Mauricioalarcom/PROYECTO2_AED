#ifndef NAIVE_SEARCH_H
#define NAIVE_SEARCH_H

#include <string>
#include <vector>

// ===========================================================================
// Solucion INGENUA de busqueda de patrones (la que exige el enunciado para la
// comparacion experimental: "busqueda directa en texto"). No usa indice; cada
// consulta recorre el texto completo -> O(n*m) en el peor caso.
// Se instrumenta con el numero de comparaciones de caracteres para comparar
// de forma justa contra el Suffix Tree.
// ===========================================================================
namespace naive {

struct Result {
    bool found = false;
    long long count = 0;
    long long charsCompared = 0;       // comparaciones de caracteres realizadas
    std::vector<int> positions;        // posiciones de inicio de cada ocurrencia
};

// Busca todas las ocurrencias (con solapamiento) de 'pattern' en 'text'.
Result search(const std::string& text, const std::string& pattern);

// Solo cuenta (sin guardar posiciones), util para benchmark.
Result count(const std::string& text, const std::string& pattern);

} // namespace naive

#endif // NAIVE_SEARCH_H
