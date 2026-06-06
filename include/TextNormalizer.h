#ifndef TEXT_NORMALIZER_H
#define TEXT_NORMALIZER_H

#include <string>

// ===========================================================================
// Normalizacion del texto antes de construir el Suffix Tree (paso del pipeline:
// "extraccion -> normalizacion -> construccion"). Deja el texto en una forma
// estable para indexar y buscar.
// ===========================================================================
namespace textnorm {

struct Options {
    bool toLowercase   = true;   // pasar a minusculas
    bool collapseSpaces = true;  // colapsar espacios/tabs/newlines en un solo espacio
    bool trim          = true;   // quitar espacios al inicio/fin
    bool keepDigits    = true;   // conservar digitos
    bool keepPunct     = false;  // conservar signos de puntuacion
};

// Devuelve el texto normalizado. Garantiza que no contenga el caracter
// terminal reservado del Suffix Tree ('\x01').
std::string normalize(const std::string& input, const Options& opt = {});

} // namespace textnorm

#endif // TEXT_NORMALIZER_H
