// ===========================================================================
// PROYECTO 2 - AED : Suffix Tree (Ukkonen) - Buscador indexado de documentos
//
// PARTE 1 (este archivo): demo de CONSOLA que
//   1. carga un documento (.txt) o usa un texto de ejemplo,
//   2. lo normaliza,
//   3. construye el Suffix Tree con Ukkonen (midiendo el tiempo),
//   4. ejecuta consultas (contains / count / find) y
//   5. VALIDA y COMPARA los resultados contra la busqueda ingenua.
//
// La visualizacion con SFML se conecta en la Parte 2.
//
// Uso:
//   ./PROYECTO2_AED [ruta_documento.txt] [patron1 patron2 ...]
// Sin argumentos usa data/sample.txt (copiado junto al ejecutable).
// ===========================================================================
#include "DocumentLoader.h"
#include "NaiveSearch.h"
#include "SuffixTree.h"
#include "TextNormalizer.h"

#include <chrono>
#include <iostream>
#include <string>
#include <vector>

using Clock = std::chrono::high_resolution_clock;

static double msSince(Clock::time_point t0) {
    return std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
}

// Texto por defecto si no se pasa archivo y no existe data/sample.txt.
static const char* kFallback =
    "el banano y la banana no son lo mismo aunque suenen parecido. "
    "ana ama a banana y banana ama a ana. abracadabra abracadabra.";

static std::string loadText(int argc, char** argv) {
    std::vector<std::string> candidates;
    if (argc > 1) candidates.push_back(argv[1]);
    candidates.push_back("data/sample.txt");
    candidates.push_back("../data/sample.txt");

    for (const auto& path : candidates) {
        try {
            std::string raw = docload::load(path);
            std::cerr << "[info] documento cargado: " << path
                      << " (" << raw.size() << " bytes)\n";
            return raw;
        } catch (const std::exception&) { /* probar siguiente */ }
    }
    std::cerr << "[info] usando texto de ejemplo embebido.\n";
    return kFallback;
}

static void runQuery(const SuffixTree& st, const std::string& text,
                     const std::string& pattern) {
    std::cout << "\n--- patron: \"" << pattern << "\" (m=" << pattern.size() << ") ---\n";

    // Suffix Tree (instrumentado)
    auto t0 = Clock::now();
    SuffixTree::SearchResult r = st.search(pattern);
    double stMs = msSince(t0);

    // Busqueda ingenua
    t0 = Clock::now();
    naive::Result nv = naive::search(text, pattern);
    double nvMs = msSince(t0);

    const bool ok = (r.count == nv.count);
    std::cout << "  SuffixTree : found=" << (r.found ? "si" : "no")
              << "  count=" << r.count
              << "  nodosVisitados=" << r.nodesVisited
              << "  comparaciones=" << r.charsCompared
              << "  tiempo=" << stMs << " ms\n";
    std::cout << "  Ingenua    : found=" << (nv.found ? "si" : "no")
              << "  count=" << nv.count
              << "  comparaciones=" << nv.charsCompared
              << "  tiempo=" << nvMs << " ms\n";
    std::cout << "  VALIDACION : " << (ok ? "OK (coinciden)" : "FALLO (difieren)") << "\n";

    if (r.found) {
        std::vector<int> pos = st.findOccurrences(pattern);
        std::cout << "  posiciones (max 10): ";
        for (size_t i = 0; i < pos.size() && i < 10; ++i) std::cout << pos[i] << ' ';
        if (pos.size() > 10) std::cout << "...";
        std::cout << "\n";
    }
}

int main(int argc, char** argv) {
    std::cout << "==== Suffix Tree (Ukkonen) - Parte 1: validacion en consola ====\n";

    std::string raw = loadText(argc, argv);
    std::string text = textnorm::normalize(raw);
    std::cout << "[info] texto normalizado: " << text.size() << " caracteres\n";

    SuffixTree st;
    auto t0 = Clock::now();
    st.buildUkkonen(text);
    double buildMs = msSince(t0);
    std::cout << "[info] arbol construido: " << st.nodeCount()
              << " nodos en " << buildMs << " ms\n";

    // Patrones: los pasados por CLI (argv[2..]) o un set por defecto.
    std::vector<std::string> patterns;
    for (int i = 2; i < argc; ++i) patterns.push_back(argv[i]);
    if (patterns.empty())
        patterns = {"ana", "banana", "ban", "abracadabra", "xyz", text.substr(0, 5)};

    for (const auto& p : patterns)
        runQuery(st, text, p);

    std::cout << "\n[fin] Parte 1 completada.\n";
    return 0;
}
