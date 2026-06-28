// ===========================================================================
// PROYECTO 2 - AED : Suffix Tree (Ukkonen) - Buscador indexado de documentos
//
// Punto de entrada. Dos modos:
//   - GUI (por defecto): abre la aplicacion visual SFML.
//   - Consola (--console): valida el nucleo contra la busqueda ingenua e
//     imprime metricas. Util para verificar correctitud sin abrir ventana.
//
// Uso:
//   ./PROYECTO2_AED [archivo.txt]                 -> GUI con ese documento
//   ./PROYECTO2_AED --console [archivo.txt] [patrones...]
// Sin archivo usa data/sample.txt (copiado junto al ejecutable).
// ===========================================================================
#include "App.h"
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

static const char* kFallback =
    "el banano y la banana no son lo mismo aunque suenen parecido. "
    "ana ama a banana y banana ama a ana. abracadabra abracadabra.";

// Carga el documento para modo consola: solo texto crudo.
static std::string loadText(const std::string& path, std::string& label) {
    std::vector<std::string> candidates;
    if (!path.empty()) candidates.push_back(path);
    candidates.push_back("data/prueba_theory_of_com.pdf");
    candidates.push_back("../data/prueba_theory_of_com.pdf");

    for (const auto& p : candidates) {
        try {
            std::string raw = docload::load(p);
            std::cerr << "[info] documento cargado: " << p
                      << " (" << raw.size() << " bytes)\n";
            label = p;
            return raw;
        } catch (const std::exception&) { /* probar siguiente */ }
    }
    std::cerr << "[info] usando texto de ejemplo embebido.\n";
    label = "ejemplo embebido";
    return kFallback;
}

// Carga el documento para modo GUI: texto + offsets de pagina (PDF).
static docload::LoadResult loadDoc(const std::string& path, std::string& label) {
    std::vector<std::string> candidates;
    if (!path.empty()) candidates.push_back(path);
    candidates.push_back("data/SuffixT1withFigures.pdf");
    candidates.push_back("../data/SuffixT1withFigures.pdf");

    for (const auto& p : candidates) {
        try {
            docload::LoadResult r = docload::loadFull(p);
            std::cerr << "[info] documento cargado: " << p
                      << " (" << r.text.size() << " bytes, "
                      << r.pageStarts.size() << " paginas)\n";
            label = p;
            return r;
        } catch (const std::exception&) { /* probar siguiente */ }
    }
    std::cerr << "[info] usando texto de ejemplo embebido.\n";
    label = "ejemplo embebido";
    return { std::string(kFallback), {} };
}

// --------------------------- modo consola ----------------------------------
static void runQuery(const SuffixTree& st, const std::string& text,
                     const std::string& pattern) {
    std::cout << "\n--- patron: \"" << pattern << "\" (m=" << pattern.size() << ") ---\n";

    auto t0 = Clock::now();
    SuffixTree::SearchResult r = st.search(pattern);
    double stMs = msSince(t0);

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
}

static int runConsole(const std::string& path, const std::vector<std::string>& cliPatterns) {
    std::cout << "==== Suffix Tree (Ukkonen) - modo consola (validacion) ====\n";

    std::string label;
    std::string raw  = loadText(path, label);
    std::string text = textnorm::normalize(raw);
    std::cout << "[info] texto normalizado: " << text.size() << " caracteres\n";

    SuffixTree st;
    auto t0 = Clock::now();
    st.buildUkkonen(text);
    std::cout << "[info] arbol construido: " << st.nodeCount()
              << " nodos en " << msSince(t0) << " ms\n";

    std::vector<std::string> patterns = cliPatterns;
    if (patterns.empty())
        patterns = {"ana", "banana", "ban", "abracadabra", "xyz"};
    for (const auto& p : patterns)
        runQuery(st, text, p);

    std::cout << "\n[fin] modo consola completado.\n";
    return 0;
}

// --------------------------------- main ------------------------------------
int main(int argc, char** argv) {
    // Separar el flag --console del resto de argumentos posicionales.
    bool console = false;
    std::vector<std::string> positional;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--console") console = true;
        else positional.push_back(a);
    }

    const std::string path = positional.empty() ? "" : positional.front();

    if (console) {
        std::vector<std::string> patterns(positional.begin() +
            (positional.empty() ? 0 : 1), positional.end());
        return runConsole(path, patterns);
    }

    // Modo GUI (por defecto)
    std::string label;
    docload::LoadResult doc = loadDoc(path, label);
    App app(std::move(doc.text), std::move(doc.pageStarts), label);
    app.run();
    return 0;
}
