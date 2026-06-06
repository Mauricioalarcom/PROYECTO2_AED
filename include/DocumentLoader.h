#ifndef DOCUMENT_LOADER_H
#define DOCUMENT_LOADER_H

#include <string>

// ===========================================================================
// Carga de documentos del pipeline ("PDF o TXT -> extraccion/lectura").
//
// Parte 1: soporte de archivos .txt.
// Parte 2: se conectara la extraccion de PDF (poppler-cpp / libpoppler) detras
//          de esta MISMA interfaz, sin tocar el resto del codigo.
// ===========================================================================
namespace docload {

enum class Type { Txt, Pdf, Unknown };

Type detectType(const std::string& path);

// Lee el contenido textual del documento. Lanza std::runtime_error si el
// archivo no existe o si el formato aun no esta soportado (PDF en Parte 1).
std::string load(const std::string& path);

// Lee un .txt plano.
std::string loadTxt(const std::string& path);

// Extrae el texto de un PDF con texto seleccionable.
// Parte 1: no implementado todavia (lanza std::runtime_error).
std::string loadPdf(const std::string& path);

} // namespace docload

#endif // DOCUMENT_LOADER_H
