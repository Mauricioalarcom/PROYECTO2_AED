#ifndef SUFFIX_TREE_H
#define SUFFIX_TREE_H

#include <string>
#include <vector>
#include <unordered_map>

// ===========================================================================
// Suffix Tree construido con el ALGORITMO DE UKKONEN (construccion O(n)).
//
// Implementado DESDE CERO segun las reglas del proyecto:
//   - nodos propios (no se usa std::map/std::set como el arbol),
//   - suffix links explicitos,
//   - active point (active node / active edge / active length),
//   - remainder (numero de sufijos pendientes),
//   - caracter terminal unico al final del texto,
//   - hojas con indice de sufijo para recuperar ocurrencias.
//
// Nota sobre std::unordered_map: se usa UNICAMENTE como adyacencia auxiliar
// de aristas (hijo por caracter) dentro de cada nodo. No reemplaza el nucleo
// algoritmico, que es la construccion incremental de Ukkonen. Esto cae dentro
// de "estructuras auxiliares basicas" permitidas por la regla 4.
// ===========================================================================
class SuffixTree {
public:
    // Metricas instrumentadas de una busqueda, utiles para la visualizacion
    // y para la comparacion experimental contra la busqueda ingenua.
    struct SearchResult {
        bool found = false;
        long long count = 0;          // numero de ocurrencias del patron
        int nodesVisited = 0;         // nodos del arbol recorridos
        int charsCompared = 0;        // comparaciones de caracteres realizadas
        std::vector<int> path;        // ids de nodos en la ruta del patron
    };

    SuffixTree() = default;

    // Construye el arbol sobre 'text'. Internamente agrega un caracter
    // terminal unico (kTerminal) que NO debe aparecer en el texto.
    void buildUkkonen(const std::string& text);

    // ---- API minima exigida por el enunciado ----
    bool contains(const std::string& pattern) const;
    long long countOccurrences(const std::string& pattern) const;
    std::vector<int> findOccurrences(const std::string& pattern) const; // posiciones de inicio

    // Version instrumentada: devuelve metricas + ruta recorrida.
    SearchResult search(const std::string& pattern) const;

    // ---- Soporte para la visualizacion del arbol ----
    struct NodeView {
        int id = -1;
        int parent = -1;
        int start = 0, end = 0;       // arista entrante [start, end] (inclusivo) sobre el texto
        int suffixLink = -1;
        int suffixIndex = -1;         // -1 si es nodo interno
        bool isLeaf = false;
        std::vector<int> children;    // ids de hijos
    };
    std::vector<NodeView> exportNodes() const;
    std::string edgeLabel(const NodeView& n) const;     // substring de la arista entrante

    const std::string& text() const { return text_; }
    int nodeCount() const { return static_cast<int>(nodes_.size()); }

    // Caracter terminal unico ('\x01'): el texto normalizado nunca debe contenerlo.
    static constexpr char kTerminal = '\x01';

private:
    static constexpr int LEAF = -1;   // marca de "end" para hojas (usa leafEnd_)

    struct Node {
        int start;                                  // inicio de la arista entrante
        int end;                                    // fin (inclusivo); LEAF para hojas
        int suffixLink = 0;                         // 0 = raiz (fallback de Ukkonen)
        int suffixIndex = -1;                       // indice de sufijo (solo hojas)
        std::unordered_map<char, int> children;     // adyacencia auxiliar por caracter
        Node(int s, int e) : start(s), end(e) {}
    };

    std::string text_;
    std::vector<Node> nodes_;
    std::vector<int> leafCount_;     // hojas en el subarbol (para countOccurrences en O(1))

    // ---- Estado del algoritmo de Ukkonen ----
    int root_ = 0;
    int activeNode_ = 0;
    int activeEdge_ = -1;            // indice dentro de text_
    int activeLength_ = 0;
    int remainder_ = 0;             // sufijos pendientes
    int leafEnd_ = -1;              // fin global de las hojas (truco de Ukkonen)
    int lastNewNode_ = -1;

    int newNode(int start, int end);
    int edgeLength(int node) const;
    bool walkDown(int node);
    void extend(int pos);
    void finalize();                 // calcula suffixIndex y leafCount con un DFS

    // Recorre el patron por el arbol. Si tiene exito, 'matchEnd' es el nodo cuyo
    // subarbol contiene todas las ocurrencias. Rellena metricas opcionales.
    bool matchPattern(const std::string& pattern, int& matchEnd,
                      int* nodesVisited, int* charsCompared,
                      std::vector<int>* path) const;
    void collectLeaves(int node, std::vector<int>& out) const;
};

#endif // SUFFIX_TREE_H
