#include "DocumentLoader.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace docload {

static std::string lowerExt(const std::string& path) {
    const auto dot = path.find_last_of('.');
    if (dot == std::string::npos) return "";
    std::string ext = path.substr(dot + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext;
}

Type detectType(const std::string& path) {
    const std::string ext = lowerExt(path);
    if (ext == "txt") return Type::Txt;
    if (ext == "pdf") return Type::Pdf;
    return Type::Unknown;
}

std::string loadTxt(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in)
        throw std::runtime_error("No se pudo abrir el archivo: " + path);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

std::string loadPdf(const std::string& /*path*/) {
    // Parte 2: aqui se integrara poppler-cpp para extraer texto seleccionable.
    throw std::runtime_error(
        "Soporte de PDF aun no implementado (Parte 2). "
        "Por ahora convierte el PDF a .txt o usa un archivo de texto.");
}

std::string load(const std::string& path) {
    switch (detectType(path)) {
        case Type::Txt: return loadTxt(path);
        case Type::Pdf: return loadPdf(path);
        default:
            throw std::runtime_error("Formato no soportado (use .txt o .pdf): " + path);
    }
}

} // namespace docload
