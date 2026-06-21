#ifndef DOCUMENT_LOADER_H
#define DOCUMENT_LOADER_H

#include <string>
#include <vector>

// ===========================================================================
// Carga de documentos del pipeline ("PDF o TXT -> extraccion/lectura").
// ===========================================================================
namespace docload {

enum class Type { Txt, Pdf, Unknown };

Type detectType(const std::string& path);

// Resultado extendido: texto crudo + offsets de inicio de cada pagina en el
// texto crudo (solo para PDF; vacio para TXT).
struct LoadResult {
    std::string      text;
    std::vector<int> pageStarts;  // pageStarts[i] = offset en 'text' donde empieza la pagina i
};

// Lee el contenido textual del documento. Lanza std::runtime_error si falla.
std::string load(const std::string& path);

// Como load(), pero tambien devuelve los offsets de pagina (PDF) o vector vacio (TXT).
LoadResult loadFull(const std::string& path);

// Lee un .txt plano.
std::string loadTxt(const std::string& path);

// Extrae el texto de un PDF con texto seleccionable.
std::string loadPdf(const std::string& path);

} // namespace docload

#endif // DOCUMENT_LOADER_H
