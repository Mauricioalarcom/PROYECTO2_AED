#include "SuffixTree.h"

#include <stack>
#include <stdexcept>

// ---------------------------------------------------------------------------
// Helpers de nodos
// ---------------------------------------------------------------------------
int SuffixTree::newNode(int start, int end) {
    nodes_.emplace_back(start, end);
    nodes_.back().suffixLink = root_;   // por defecto apunta a la raiz (Ukkonen)
    return static_cast<int>(nodes_.size()) - 1;
}

int SuffixTree::edgeLength(int node) const {
    if (node == root_) return 0;
    const int e = (nodes_[node].end == LEAF) ? leafEnd_ : nodes_[node].end;
    return e - nodes_[node].start + 1;
}

// Si el active length supera la arista hacia 'node', baja a ese nodo y devuelve
// true para reintentar la extension desde alli (skip/count trick de Ukkonen).
bool SuffixTree::walkDown(int node) {
    const int len = edgeLength(node);
    if (activeLength_ >= len) {
        activeEdge_  += len;
        activeLength_ -= len;
        activeNode_   = node;
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Una fase de Ukkonen: agrega el caracter text_[pos] al arbol.
// ---------------------------------------------------------------------------
void SuffixTree::extend(int pos) {
    leafEnd_ = pos;          // regla 1: extension de hojas en O(1) (fin global)
    ++remainder_;            // un sufijo mas pendiente
    lastNewNode_ = -1;       // ningun nodo interno creado en esta fase aun

    while (remainder_ > 0) {
        if (activeLength_ == 0)
            activeEdge_ = pos;          // mirar el caracter actual

        const char ae = text_[activeEdge_];
        const unsigned char aeKey = static_cast<unsigned char>(ae);

        const int existing = nodes_[activeNode_].children.get(aeKey);
        if (existing < 0) {
            // ---- Regla 2: no existe arista; crear una hoja ----
            const int leaf = newNode(pos, LEAF);
            nodes_[activeNode_].children.set(aeKey, leaf);

            if (lastNewNode_ != -1) {           // resolver suffix link pendiente
                nodes_[lastNewNode_].suffixLink = activeNode_;
                lastNewNode_ = -1;
            }
        } else {
            const int next = existing;
            if (walkDown(next)) continue;       // bajar y reintentar

            // ---- Regla 3: el caracter ya esta sobre la arista ----
            if (text_[nodes_[next].start + activeLength_] == text_[pos]) {
                if (lastNewNode_ != -1 && activeNode_ != root_) {
                    nodes_[lastNewNode_].suffixLink = activeNode_;
                    lastNewNode_ = -1;
                }
                ++activeLength_;
                break;                          // fin de fase; queda remainder
            }

            // ---- Regla 2: dividir la arista (split) ----
            const int splitEnd = nodes_[next].start + activeLength_ - 1;
            const int split = newNode(nodes_[next].start, splitEnd);
            nodes_[activeNode_].children.set(aeKey, split);

            const int leaf = newNode(pos, LEAF);
            nodes_[split].children.set(static_cast<unsigned char>(text_[pos]), leaf);

            nodes_[next].start += activeLength_;
            nodes_[split].children.set(static_cast<unsigned char>(text_[nodes_[next].start]), next);

            if (lastNewNode_ != -1)
                nodes_[lastNewNode_].suffixLink = split;
            lastNewNode_ = split;
        }

        // Un sufijo fue agregado en esta iteracion.
        --remainder_;

        if (activeNode_ == root_ && activeLength_ > 0) {
            --activeLength_;
            activeEdge_ = pos - remainder_ + 1;
        } else if (activeNode_ != root_) {
            activeNode_ = nodes_[activeNode_].suffixLink;   // seguir suffix link
        }
    }
}

// ---------------------------------------------------------------------------
// Construccion completa
// ---------------------------------------------------------------------------
void SuffixTree::buildUkkonen(const std::string& text) {
    nodes_.clear();
    leafCount_.clear();
    text_ = text;
    text_.push_back(kTerminal);     // caracter terminal unico

    root_        = 0;
    activeNode_  = 0;
    activeEdge_  = -1;
    activeLength_ = 0;
    remainder_   = 0;
    leafEnd_     = -1;
    lastNewNode_ = -1;

    newNode(-1, -1);                // nodo raiz (sin arista entrante)

    const int n = static_cast<int>(text_.size());
    for (int i = 0; i < n; ++i)
        extend(i);

    finalize();
}

// DFS iterativo (post-orden) que asigna suffixIndex a las hojas y cuenta las
// hojas de cada subarbol. Iterativo para no desbordar la pila con textos largos.
void SuffixTree::finalize() {
    leafCount_.assign(nodes_.size(), 0);
    const int n = static_cast<int>(text_.size());

    // Pila con (nodo, labelHeight, fase): fase 0 = al entrar, 1 = al salir.
    struct Frame { int node; int height; int phase; };
    std::stack<Frame> st;
    st.push({root_, 0, 0});

    while (!st.empty()) {
        Frame f = st.top(); st.pop();
        Node& nd = nodes_[f.node];

        if (nd.children.empty()) {              // hoja
            nd.suffixIndex = n - f.height;
            leafCount_[f.node] = 1;
            continue;
        }

        if (f.phase == 0) {
            nd.suffixIndex = -1;                // nodo interno
            st.push({f.node, f.height, 1});     // volver despues de los hijos
            nd.children.forEach([&](unsigned char, int child) {
                st.push({child, f.height + edgeLength(child), 0});
            });
        } else {
            int total = 0;
            nd.children.forEach([&](unsigned char, int child) {
                total += leafCount_[child];
            });
            leafCount_[f.node] = total;
        }
    }
}

// ---------------------------------------------------------------------------
// Busqueda
// ---------------------------------------------------------------------------
bool SuffixTree::matchPattern(const std::string& pattern, int& matchEnd,
                              int* nodesVisited, int* charsCompared,
                              std::vector<int>* path) const {
    if (pattern.empty()) return false;

    int node = root_;
    if (path) path->push_back(root_);

    const int m = static_cast<int>(pattern.size());
    int i = 0;
    while (i < m) {
        const char c = pattern[i];
        const int child = nodes_[node].children.get(static_cast<unsigned char>(c));
        if (child < 0)
            return false;                       // no hay arista: patron ausente

        if (nodesVisited) ++(*nodesVisited);
        if (path) path->push_back(child);

        const int s = nodes_[child].start;
        const int e = (nodes_[child].end == LEAF) ? leafEnd_ : nodes_[child].end;

        int j = s;
        while (j <= e && i < m) {               // comparar a lo largo de la arista
            if (charsCompared) ++(*charsCompared);
            if (text_[j] != pattern[i])
                return false;                   // mismatch dentro de la arista
            ++i; ++j;
        }
        node = child;
    }

    matchEnd = node;                            // el subarbol de 'node' tiene las ocurrencias
    return true;
}

void SuffixTree::collectLeaves(int node, std::vector<int>& out) const {
    std::stack<int> st;
    st.push(node);
    while (!st.empty()) {
        const int u = st.top(); st.pop();
        const Node& nd = nodes_[u];
        if (nd.children.empty()) {
            if (nd.suffixIndex >= 0)
                out.push_back(nd.suffixIndex);
        } else {
            nd.children.forEach([&](unsigned char, int child) { st.push(child); });
        }
    }
}

bool SuffixTree::contains(const std::string& pattern) const {
    int end;
    return matchPattern(pattern, end, nullptr, nullptr, nullptr);
}

long long SuffixTree::countOccurrences(const std::string& pattern) const {
    int end;
    if (!matchPattern(pattern, end, nullptr, nullptr, nullptr))
        return 0;
    return leafCount_[end];                     // O(1) tras llegar al nodo
}

std::vector<int> SuffixTree::findOccurrences(const std::string& pattern) const {
    std::vector<int> out;
    int end;
    if (!matchPattern(pattern, end, nullptr, nullptr, nullptr))
        return out;
    collectLeaves(end, out);
    return out;
}

SuffixTree::SearchResult SuffixTree::search(const std::string& pattern) const {
    SearchResult r;
    int end;
    r.found = matchPattern(pattern, end, &r.nodesVisited, &r.charsCompared, &r.path);
    if (r.found)
        r.count = leafCount_[end];
    return r;
}

// ---------------------------------------------------------------------------
// Exportacion para la visualizacion
// ---------------------------------------------------------------------------
std::vector<SuffixTree::NodeView> SuffixTree::exportNodes() const {
    std::vector<NodeView> out(nodes_.size());
    for (int i = 0; i < static_cast<int>(nodes_.size()); ++i) {
        const Node& nd = nodes_[i];
        NodeView& v = out[i];
        v.id = i;
        v.parent = -1;
        v.start = nd.start;
        v.end = (nd.end == LEAF) ? leafEnd_ : nd.end;
        v.suffixLink = nd.suffixLink;
        v.suffixIndex = nd.suffixIndex;
        v.isLeaf = nd.children.empty();
        nd.children.forEach([&](unsigned char, int child) { v.children.push_back(child); });
    }
    for (int i = 0; i < static_cast<int>(out.size()); ++i)
        for (int c : out[i].children)
            out[c].parent = i;
    return out;
}

std::string SuffixTree::edgeLabelById(int id) const {
    if (id < 0 || id >= static_cast<int>(nodes_.size()) || id == root_) return "";
    const Node& nd = nodes_[id];
    if (nd.start < 0) return "";
    int e = (nd.end == LEAF) ? leafEnd_ : nd.end;
    if (e >= static_cast<int>(text_.size())) e = static_cast<int>(text_.size()) - 1;
    std::string s = text_.substr(nd.start, e - nd.start + 1);
    for (char& ch : s) if (ch == kTerminal) ch = '$';
    return s;
}

int SuffixTree::nodeSuffixIndex(int id) const {
    if (id < 0 || id >= static_cast<int>(nodes_.size())) return -1;
    return nodes_[id].suffixIndex;
}

std::string SuffixTree::edgeLabel(const NodeView& n) const {
    if (n.id == root_ || n.start < 0) return "";
    int e = n.end;
    if (e >= static_cast<int>(text_.size())) e = static_cast<int>(text_.size()) - 1;
    std::string s = text_.substr(n.start, e - n.start + 1);
    // Mostrar el terminal de forma legible
    for (char& ch : s) if (ch == kTerminal) ch = '$';
    return s;
}
