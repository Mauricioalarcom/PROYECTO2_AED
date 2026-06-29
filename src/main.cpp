#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <algorithm>
#include "SuffixTree.h"
#include "NaiveSearch.h"
#include "TextNormalizer.h"
#include "DocumentLoader.h"

using namespace std;
using Clock = chrono::high_resolution_clock;

double msSince(Clock::time_point t0) {
    return chrono::duration<double, milli>(Clock::now() - t0).count();
}

// Fragmento del texto alrededor de la posicion dada
string fragmento(const string& text, int pos, int m, int ctx = 35) {
    int desde = max(0, pos - ctx);
    int hasta = min((int)text.size(), pos + m + ctx);
    string f = text.substr(desde, hasta - desde);
    return (desde > 0 ? "..." : "") + f + (hasta < (int)text.size() ? "..." : "");
}

void mostrarRuta(const SuffixTree& st, const vector<Node*>& path) {
    cout << "Ruta: raiz";
    for (int i = 1; i < (int)path.size(); i++)
        cout << " -> [" << st.edgeLabel(path[i]) << "]";
    cout << "\n";
}

void buscar(const SuffixTree& st, const string& text, const string& patron) {
    cout << "\n--- \"" << patron << "\" ---\n";

    auto t0 = Clock::now();
    SuffixTree::SearchResult r = st.search(patron);
    double stMs = msSince(t0);

    t0 = Clock::now();
    naive::Result nv = naive::search(text, patron);
    double nvMs = msSince(t0);

    if (!r.found) {
        cout << "No encontrado.\n";
    } else {
        cout << "Ocurrencias: " << r.count << "\n";

        vector<int> pos = st.findOccurrences(patron);
        sort(pos.begin(), pos.end());

        cout << "Posiciones: ";
        int n = min((int)pos.size(), 10);
        for (int i = 0; i < n; i++) cout << pos[i] << " ";
        if ((int)pos.size() > 10) cout << "(+" << pos.size() - 10 << " mas)";
        cout << "\n";

        cout << "Fragmentos:\n";
        for (int i = 0; i < min(3, n); i++)
            cout << "  " << i + 1 << ". \"" << fragmento(text, pos[i], patron.size()) << "\"\n";

        mostrarRuta(st, r.path);
    }

    cout << "  SuffixTree : " << stMs << " ms | comparaciones=" << r.charsCompared << " | nodos=" << r.nodesVisited << "\n";
    cout << "  Ingenua    : " << nvMs << " ms | comparaciones=" << nv.charsCompared << "\n";
    cout << "  Validacion : " << (r.count == nv.count ? "OK" : "FALLO") << "\n";
}

int main(int argc, char** argv) {
    cout << "=== DocFinder: Buscador con Suffix Tree (Ukkonen) ===\n\n";

    string path;
    if (argc > 1) {
        path = argv[1];
    } else {
        cout << "Archivo (.txt o .pdf): ";
        getline(cin, path);
    }

    string raw;
    try {
        raw = docload::load(path);
    } catch (const exception& e) {
        cerr << "Error al cargar: " << e.what() << "\n";
        return 1;
    }

    string text = textnorm::normalize(raw);
    cout << "Texto cargado: " << text.size() << " caracteres\n";

    SuffixTree st;
    auto t0 = Clock::now();
    st.buildUkkonen(text);
    cout << "Suffix Tree construido: " << st.nodeCount() << " nodos en " << msSince(t0) << " ms\n";

    string patron;
    while (true) {
        cout << "\nPatron (enter para salir): ";
        if (!getline(cin, patron) || patron.empty()) break;
        buscar(st, text, patron);
    }

    return 0;
}
