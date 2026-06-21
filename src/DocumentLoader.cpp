#include "DocumentLoader.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

#ifdef HAVE_POPPLER
#include <poppler-document.h>
#include <poppler-page.h>
#include <memory>
#endif

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

#ifdef HAVE_POPPLER
// Extrae texto y offsets de inicio de pagina de un PDF con texto seleccionable.
static LoadResult loadPdfFull(const std::string& path) {
    std::unique_ptr<poppler::document> doc(
        poppler::document::load_from_file(path));
    if (!doc)
        throw std::runtime_error("No se pudo abrir el PDF: " + path);
    if (doc->is_locked())
        throw std::runtime_error("El PDF esta protegido/encriptado: " + path);

    std::string out;
    std::vector<int> pageStarts;
    const int pages = doc->pages();
    for (int i = 0; i < pages; ++i) {
        pageStarts.push_back(static_cast<int>(out.size()));
        std::unique_ptr<poppler::page> pg(doc->create_page(i));
        if (!pg) continue;
        const poppler::byte_array utf8 = pg->text().to_utf8();
        out.append(utf8.begin(), utf8.end());
        out.push_back('\n');
    }
    if (out.empty())
        throw std::runtime_error(
            "El PDF no tiene texto seleccionable (quiza es escaneado/imagen): " + path);
    return {out, pageStarts};
}

std::string loadPdf(const std::string& path) { return loadPdfFull(path).text; }
#else
std::string loadPdf(const std::string& /*path*/) {
    throw std::runtime_error(
        "Soporte de PDF no compilado. Instala poppler y reconfigura: "
        "brew install poppler pkg-config  (o convierte el PDF a .txt).");
}
#endif

std::string load(const std::string& path) {
    switch (detectType(path)) {
        case Type::Txt: return loadTxt(path);
        case Type::Pdf: return loadPdf(path);
        default:
            throw std::runtime_error("Formato no soportado (use .txt o .pdf): " + path);
    }
}

LoadResult loadFull(const std::string& path) {
    switch (detectType(path)) {
        case Type::Txt: return { loadTxt(path), {} };
        case Type::Pdf:
#ifdef HAVE_POPPLER
            return loadPdfFull(path);
#else
            loadPdf(path);   // lanza "soporte no compilado"
            return {};
#endif
        default:
            throw std::runtime_error("Formato no soportado (use .txt o .pdf): " + path);
    }
}

} // namespace docload
