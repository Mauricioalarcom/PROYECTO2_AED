#ifndef SUFFIX_TREE_H
#define SUFFIX_TREE_H

#include <string>
#include <vector>

// ============================================================
// Nodo del Suffix Tree (algoritmo de Ukkonen).
//
// El puntero 'end' apunta al fin de la arista entrante:
//   - Hojas:          apuntan a leafEnd_ del arbol (se actualiza
//                     automaticamente en cada fase -> truco O(1))
//   - Nodos internos: tienen su propio int en el heap.
// suffixIndex guarda el indice de inicio del sufijo (solo hojas;
// -1 para nodos internos).
// ============================================================
struct Node {
    int   start;
    int*  end;
    Node* suffixLink;
    Node* children[128];
    int   suffixIndex;

    Node(int s, int* e);
};

// ============================================================
// Suffix Tree construido con el algoritmo de Ukkonen (O(n)).
// Implementado desde cero; no usa std::map / std::set / std::unordered_map.
// ============================================================
class SuffixTree {
public:
    // Metricas instrumentadas de una busqueda (para comparacion experimental).
    struct SearchResult {
        bool               found         = false;
        long long          count         = 0;
        int                nodesVisited  = 0;
        int                charsCompared = 0;
        std::vector<Node*> path;          // nodos recorridos (raiz -> nodo match)
    };

    SuffixTree();
    ~SuffixTree();

    // Sin copia (maneja punteros crudos con new/delete).
    SuffixTree(const SuffixTree&)            = delete;
    SuffixTree& operator=(const SuffixTree&) = delete;

    // ---- API exigida por el enunciado ----
    void             buildUkkonen(std::string text);
    bool             contains(std::string pattern) const;
    int              countOccurrences(std::string pattern) const;
    std::vector<int> findOccurrences(std::string pattern) const;

    // ---- Version instrumentada para la visualizacion ----
    SearchResult search(std::string pattern) const;

    // ---- Soporte para visualizar la ruta del patron en el arbol ----
    std::string        edgeLabel(Node* node) const;
    int                nodeCount() const { return nodeCount_; }
    const std::string& getText()   const { return text_; }

    // Caracter terminal unico (nunca aparece en texto normalizado).
    static constexpr char kTerminal = '\x01';

private:
    std::string text_;      // texto indexado (con kTerminal al final)
    Node*       root_;
    int         leafEnd_;   // fin global de las hojas (truco de Ukkonen)
    int         nodeCount_;

    // Active point (estado del algoritmo de Ukkonen)
    Node* activeNode_;
    int   activeEdge_;      // indice en text_ del primer char de la arista activa
    int   activeLength_;
    int   remainder_;
    Node* lastNewNode_;     // ultimo nodo interno sin suffix link resuelto

    Node* newNode(int start, int* end);
    void  freeTree(Node* node);
    int   edgeLen(Node* node) const;
    bool  walkDown(Node* node);
    void  extend(int pos);
    void  finalize();

    bool matchPattern(const std::string& pat, Node*& matchEnd,
                      int* nodesVisited, int* charsCompared,
                      std::vector<Node*>* path) const;
    void collectLeaves(Node* node, std::vector<int>& out) const;
    int  countLeaves(Node* node) const;
};

#endif // SUFFIX_TREE_H
