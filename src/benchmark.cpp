// ===========================================================================
// PROYECTO 2 - AED : Benchmark experimental (punto 6 del enunciado)
//
// Compara el Suffix Tree (Ukkonen) contra la busqueda ingenua en >= 3 tamanos
// de texto (por defecto 100k / 500k / 1M caracteres) y, para varias longitudes
// de patron, reporta:
//   - tiempo de construccion del Suffix Tree;
//   - tiempo promedio de consulta (Suffix Tree y ingenua);
//   - nodos visitados / comparaciones por el Suffix Tree;
//   - comparaciones (elementos revisados) por la ingenua;
//   - numero de ocurrencias encontradas;
//   - efecto del tamano del texto y del patron.
//
// Imprime una tabla y guarda un CSV para el reporte.
//
// Uso:
//   ./benchmark                         -> 100000,500000,1000000  (texto sintetico)
//   ./benchmark 10000,50000             -> tamanos personalizados (rapido)
//   ./benchmark 100000,500000 doc.txt   -> usa prefijos de un archivo real
// ===========================================================================
#include "NaiveSearch.h"
#include "SuffixTree.h"
#include "TextNormalizer.h"
#include "DocumentLoader.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

using Clock = std::chrono::high_resolution_clock;
static double msBetween(Clock::time_point a, Clock::time_point b) {
    return std::chrono::duration<double, std::milli>(b - a).count();
}

// Vocabulario fijo para generar texto sintetico "tipo lenguaje natural":
// palabras repetidas -> los patrones tienen varias ocurrencias (caso util para
// evidenciar O(m+k) del arbol frente a O(n) del escaneo ingenuo).
static const char* kVocab[] = {
    "el","la","de","que","y","en","un","una","ser","se","no","haber","por",
    "con","su","para","como","estar","tener","le","lo","todo","pero","mas",
    "hacer","poder","decir","este","ir","otro","ese","si","me","ya","ver",
    "porque","dar","cuando","muy","sin","vez","mucho","saber","sobre","mismo",
    "banana","abracadabra","mississippi","algoritmo","suffix","ukkonen","arbol",
    "texto","busqueda","patron","ocurrencia","indexado","documento","cadena",
};

// Genera un corpus deterministico de al menos 'minLen' caracteres ([a-z ]).
static std::string makeCorpus(size_t minLen) {
    const int vocabN = static_cast<int>(sizeof(kVocab) / sizeof(kVocab[0]));
    std::mt19937 rng(12345);                       // semilla fija -> reproducible
    std::uniform_int_distribution<int> pick(0, vocabN - 1);
    std::string s;
    s.reserve(minLen + 32);
    while (s.size() < minLen) {
        s += kVocab[pick(rng)];
        s += ' ';
    }
    s.resize(minLen);
    return s;
}

struct Row {
    size_t n; int L;
    double buildMs;
    double stUs, nvUs;          // tiempo promedio por consulta (microsegundos)
    double stNodes, stCmp;      // nodos visitados / comparaciones del arbol
    double nvCmp;               // comparaciones de la ingenua
    double occ;                 // ocurrencias promedio
    double speedup;             // nvUs / stUs
};

int main(int argc, char** argv) {
    // ---- tamanos ----
    std::vector<size_t> sizes = {100000, 500000, 1000000};
    if (argc > 1) {
        sizes.clear();
        std::string a = argv[1], cur;
        for (char c : a) {
            if (c == ',') { if (!cur.empty()) sizes.push_back(std::stoul(cur)); cur.clear(); }
            else cur += c;
        }
        if (!cur.empty()) sizes.push_back(std::stoul(cur));
    }
    const size_t maxN = *std::max_element(sizes.begin(), sizes.end());

    // ---- corpus (archivo real opcional, si no, sintetico) ----
    std::string corpus;
    if (argc > 2) {
        try {
            corpus = textnorm::normalize(docload::load(argv[2]));
            std::cerr << "[bench] corpus real: " << argv[2]
                      << " (" << corpus.size() << " chars normalizados)\n";
        } catch (const std::exception& e) {
            std::cerr << "[bench] no se pudo cargar '" << argv[2]
                      << "': " << e.what() << " -> uso texto sintetico\n";
        }
    }
    if (corpus.size() < maxN) {
        if (!corpus.empty())
            std::cerr << "[bench] el archivo es mas corto que el tamano maximo; uso sintetico\n";
        corpus = makeCorpus(maxN);
        std::cerr << "[bench] corpus sintetico de " << corpus.size() << " chars\n";
    }

    const std::vector<int> patternLens = {4, 8, 16};
    const int K = 40;            // patrones muestreados por longitud
    std::mt19937 rng(98765);

    std::vector<Row> rows;

    for (size_t n : sizes) {
        std::string text = corpus.substr(0, n);

        SuffixTree st;
        auto t0 = Clock::now();
        st.buildUkkonen(text);
        auto t1 = Clock::now();
        const double buildMs = msBetween(t0, t1);
        std::cerr << "[bench] n=" << n << "  nodos=" << st.nodeCount()
                  << "  construccion=" << buildMs << " ms\n";

        for (int L : patternLens) {
            // Muestrear K patrones que existen en el texto (substrings reales).
            std::vector<std::string> pats;
            std::uniform_int_distribution<size_t> pos(0, n - L - 1);
            for (int i = 0; i < K; ++i) pats.push_back(text.substr(pos(rng), L));

            // Repeticiones para que el tiempo por consulta sea medible.
            const int stReps = 5000;
            const int nvReps = (n <= 100000) ? 200 : (n <= 500000 ? 60 : 20);

            double sumStUs = 0, sumNvUs = 0, sumNodes = 0, sumStCmp = 0,
                   sumNvCmp = 0, sumOcc = 0;
            volatile long long sink = 0;

            for (const auto& p : pats) {
                // Metricas del arbol (una sola vez, no temporizadas).
                SuffixTree::SearchResult r = st.search(p);
                naive::Result nv = naive::count(text, p);
                if (r.count != nv.count)
                    std::cerr << "[bench] WARN: discrepancia en '" << p << "'\n";

                sumNodes += r.nodesVisited;
                sumStCmp += r.charsCompared;
                sumNvCmp += static_cast<double>(nv.charsCompared);
                sumOcc   += static_cast<double>(r.count);

                // Tiempo Suffix Tree.
                auto a = Clock::now();
                for (int rep = 0; rep < stReps; ++rep) sink += st.countOccurrences(p);
                auto b = Clock::now();
                sumStUs += msBetween(a, b) * 1000.0 / stReps;

                // Tiempo ingenua.
                a = Clock::now();
                for (int rep = 0; rep < nvReps; ++rep) sink += naive::count(text, p).count;
                b = Clock::now();
                sumNvUs += msBetween(a, b) * 1000.0 / nvReps;
            }
            (void)sink;

            Row row;
            row.n = n; row.L = L; row.buildMs = buildMs;
            row.stUs    = sumStUs / K;
            row.nvUs    = sumNvUs / K;
            row.stNodes = sumNodes / K;
            row.stCmp   = sumStCmp / K;
            row.nvCmp   = sumNvCmp / K;
            row.occ     = sumOcc / K;
            row.speedup = (row.stUs > 0) ? row.nvUs / row.stUs : 0.0;
            rows.push_back(row);
            std::cerr << "  L=" << L << " listo\n";
        }
    }

    // ---- tabla por consola ----
    std::printf("\n%-9s %-3s %-10s %-12s %-12s %-9s %-12s %-12s %-9s %-8s\n",
                "n", "m", "build_ms", "st_query_us", "nv_query_us",
                "st_nodos", "st_comp", "nv_comp", "occ", "speedup");
    for (const auto& r : rows) {
        std::printf("%-9zu %-3d %-10.3f %-12.4f %-12.2f %-9.1f %-12.1f %-12.1f %-9.1f %-8.0fx\n",
                    r.n, r.L, r.buildMs, r.stUs, r.nvUs,
                    r.stNodes, r.stCmp, r.nvCmp, r.occ, r.speedup);
    }

    // ---- CSV ----
    const std::string csvPath = "data/benchmark_results.csv";
    std::ofstream csv(csvPath);
    if (csv) {
        csv << "text_size,pattern_len,build_ms,st_query_us,naive_query_us,"
               "st_nodes_visited,st_comparisons,naive_comparisons,avg_occurrences,speedup\n";
        for (const auto& r : rows)
            csv << r.n << "," << r.L << "," << r.buildMs << "," << r.stUs << ","
                << r.nvUs << "," << r.stNodes << "," << r.stCmp << "," << r.nvCmp
                << "," << r.occ << "," << r.speedup << "\n";
        std::cerr << "\n[bench] CSV guardado en " << csvPath << "\n";
    } else {
        std::cerr << "\n[bench] no se pudo escribir " << csvPath
                  << " (ejecuta desde la raiz del repo)\n";
    }
    return 0;
}
