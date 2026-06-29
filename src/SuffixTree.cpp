#include "SuffixTree.h"

#include <stack>

// ============================================================
// Node
// ============================================================
Node::Node(int s, int* e)
    : start(s), end(e), suffixLink(nullptr), suffixIndex(-1) {
    for (int i = 0; i < 128; i++)
        children[i] = nullptr;
}

// ============================================================
// Constructor / destructor
// ============================================================
SuffixTree::SuffixTree()
    : root_(nullptr), leafEnd_(-1), nodeCount_(0),
      activeNode_(nullptr), activeEdge_(-1),
      activeLength_(0), remainder_(0), lastNewNode_(nullptr) {}

SuffixTree::~SuffixTree() {
    freeTree(root_);
}

// Libera recursivamente el subarbol de 'node'.
// Las hojas tienen end == &leafEnd_ (miembro del arbol, no del heap);
// los nodos internos y la raiz tienen su propio int* en el heap.
void SuffixTree::freeTree(Node* node) {
    if (!node) return;
    for (int i = 0; i < 128; i++)
        if (node->children[i])
            freeTree(node->children[i]);
    if (node->end != &leafEnd_)
        delete node->end;
    delete node;
}

// ============================================================
// Helpers
// ============================================================

// Crea un nodo nuevo y lo registra.
Node* SuffixTree::newNode(int start, int* end) {
    Node* n = new Node(start, end);
    n->suffixLink = root_;    // fallback por defecto: la raiz
    nodeCount_++;
    return n;
}

// Longitud de la arista entrante al nodo (numero de caracteres).
int SuffixTree::edgeLen(Node* node) const {
    return *node->end - node->start + 1;
}

// Skip/count trick: si el active length supera la arista, baja al nodo.
// Devuelve true para reintentar la extension desde el nuevo active point.
bool SuffixTree::walkDown(Node* node) {
    int len = edgeLen(node);
    if (activeLength_ >= len) {
        activeEdge_  += len;
        activeLength_ -= len;
        activeNode_   = node;
        return true;
    }
    return false;
}

// ============================================================
// Fase de Ukkonen: inserta text_[pos] en el arbol.
// ============================================================
void SuffixTree::extend(int pos) {
    leafEnd_ = pos;       // Regla 1: extiende todas las hojas en O(1)
    remainder_++;
    lastNewNode_ = nullptr;

    while (remainder_ > 0) {
        if (activeLength_ == 0)
            activeEdge_ = pos;

        int aeIdx = (unsigned char)text_[activeEdge_];
        Node* child = activeNode_->children[aeIdx];

        if (!child) {
            // ---- Regla 2: no existe arista; crear hoja nueva ----
            activeNode_->children[aeIdx] = newNode(pos, &leafEnd_);
            if (lastNewNode_) {
                lastNewNode_->suffixLink = activeNode_;
                lastNewNode_ = nullptr;
            }
        } else {
            if (walkDown(child)) continue;  // bajar y reintentar

            // ---- Regla 3: el caracter ya existe en la arista ----
            if (text_[child->start + activeLength_] == text_[pos]) {
                if (lastNewNode_ && activeNode_ != root_) {
                    lastNewNode_->suffixLink = activeNode_;
                    lastNewNode_ = nullptr;
                }
                activeLength_++;
                break;      // fin de fase; remainder queda para la siguiente
            }

            // ---- Regla 2 (split): dividir la arista ----
            int* splitEnd = new int(child->start + activeLength_ - 1);
            Node* split = newNode(child->start, splitEnd);
            activeNode_->children[aeIdx] = split;

            // Hoja nueva desde el punto de split hacia text_[pos]
            split->children[(unsigned char)text_[pos]] = newNode(pos, &leafEnd_);

            // El nodo hijo existente arranca ahora despues del split
            child->start += activeLength_;
            split->children[(unsigned char)text_[child->start]] = child;

            if (lastNewNode_)
                lastNewNode_->suffixLink = split;
            lastNewNode_ = split;
        }

        remainder_--;

        if (activeNode_ == root_ && activeLength_ > 0) {
            // Desde la raiz: acortar la arista activa un caracter
            activeLength_--;
            activeEdge_ = pos - remainder_ + 1;
        } else if (activeNode_ != root_) {
            // Saltar por el suffix link al siguiente sufijo pendiente
            activeNode_ = activeNode_->suffixLink;
        }
    }
}

// ============================================================
// Construccion completa del arbol
// ============================================================
void SuffixTree::buildUkkonen(std::string text) {
    // Liberar arbol anterior si existe
    freeTree(root_);
    root_ = nullptr;
    nodeCount_ = 0;

    text_    = std::move(text);
    text_   += kTerminal;   // caracter terminal unico
    leafEnd_ = -1;

    // Crear la raiz (sin arista entrante; su end es propio en el heap)
    root_ = new Node(-1, new int(-1));
    root_->suffixLink = root_;
    nodeCount_ = 1;

    activeNode_  = root_;
    activeEdge_  = -1;
    activeLength_ = 0;
    remainder_   = 0;
    lastNewNode_ = nullptr;

    for (int i = 0; i < (int)text_.size(); i++)
        extend(i);

    finalize();
}

// DFS iterativo (pre-orden): asigna suffixIndex a cada hoja.
// suffixIndex = n - altura_en_caracteres (posicion de inicio del sufijo).
void SuffixTree::finalize() {
    struct Frame { Node* node; int height; };
    std::stack<Frame> st;
    st.push({root_, 0});

    while (!st.empty()) {
        auto [node, height] = st.top();
        st.pop();

        bool isLeaf = true;
        for (int c = 0; c < 128; c++) {
            if (node->children[c]) {
                isLeaf = false;
                int childHeight = height + edgeLen(node->children[c]);
                st.push({node->children[c], childHeight});
            }
        }
        if (isLeaf && node != root_)
            node->suffixIndex = (int)text_.size() - height;
    }
}

// ============================================================
// Busqueda
// ============================================================

// Recorre el patron sobre el arbol. Si tiene exito, matchEnd apunta al nodo
// cuyo subarbol contiene todas las ocurrencias. Rellena metricas opcionales.
bool SuffixTree::matchPattern(const std::string& pat, Node*& matchEnd,
                              int* nodesVisited, int* charsCompared,
                              std::vector<Node*>* path) const {
    if (pat.empty() || !root_) return false;

    Node* node = root_;
    if (path) path->push_back(root_);

    int m = (int)pat.size();
    int i = 0;
    while (i < m) {
        int c = (unsigned char)pat[i];
        if (c >= 128) return false;
        Node* child = node->children[c];
        if (!child) return false;

        if (nodesVisited) (*nodesVisited)++;
        if (path) path->push_back(child);

        int j = child->start;
        int e = *child->end;
        while (j <= e && i < m) {
            if (charsCompared) (*charsCompared)++;
            if (text_[j] != pat[i]) return false;
            j++; i++;
        }
        node = child;
    }

    matchEnd = node;
    return true;
}

// DFS iterativo: recolecta suffixIndex de todas las hojas del subarbol.
void SuffixTree::collectLeaves(Node* node, std::vector<int>& out) const {
    std::stack<Node*> st;
    st.push(node);
    while (!st.empty()) {
        Node* cur = st.top(); st.pop();
        bool isLeaf = true;
        for (int c = 0; c < 128; c++) {
            if (cur->children[c]) {
                isLeaf = false;
                st.push(cur->children[c]);
            }
        }
        if (isLeaf && cur->suffixIndex >= 0)
            out.push_back(cur->suffixIndex);
    }
}

// DFS iterativo: cuenta las hojas del subarbol.
int SuffixTree::countLeaves(Node* node) const {
    int count = 0;
    std::stack<Node*> st;
    st.push(node);
    while (!st.empty()) {
        Node* cur = st.top(); st.pop();
        bool isLeaf = true;
        for (int c = 0; c < 128; c++) {
            if (cur->children[c]) {
                isLeaf = false;
                st.push(cur->children[c]);
            }
        }
        if (isLeaf) count++;
    }
    return count;
}

bool SuffixTree::contains(std::string pattern) const {
    Node* end = nullptr;
    return matchPattern(pattern, end, nullptr, nullptr, nullptr);
}

int SuffixTree::countOccurrences(std::string pattern) const {
    Node* end = nullptr;
    if (!matchPattern(pattern, end, nullptr, nullptr, nullptr))
        return 0;
    return countLeaves(end);
}

std::vector<int> SuffixTree::findOccurrences(std::string pattern) const {
    std::vector<int> out;
    Node* end = nullptr;
    if (!matchPattern(pattern, end, nullptr, nullptr, nullptr))
        return out;
    collectLeaves(end, out);
    return out;
}

SuffixTree::SearchResult SuffixTree::search(std::string pattern) const {
    SearchResult r;
    Node* end = nullptr;
    r.found = matchPattern(pattern, end, &r.nodesVisited, &r.charsCompared, &r.path);
    if (r.found)
        r.count = countLeaves(end);
    return r;
}

// ============================================================
// Soporte para visualizacion
// ============================================================

// Retorna la etiqueta de la arista entrante a 'node' (subcadena de text_).
std::string SuffixTree::edgeLabel(Node* node) const {
    if (!node || node == root_ || node->start < 0) return "";
    int s = node->start;
    int e = *node->end;
    if (e >= (int)text_.size()) e = (int)text_.size() - 1;
    std::string lbl = text_.substr(s, e - s + 1);
    for (char& ch : lbl)
        if (ch == kTerminal) ch = '$';
    return lbl;
}
