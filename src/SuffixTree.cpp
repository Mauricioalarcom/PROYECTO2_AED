#include "SuffixTree.h"

#include <stack>
using namespace std;

Node::Node(int s, int* e)
    : start(s), end(e), suffixLink(nullptr), suffixIndex(-1) {
    for (int i = 0; i < 128; i++)
        children[i] = nullptr;
}


// constructor y destructor owo

SuffixTree::SuffixTree()
    : root(nullptr), leafEnd(-1), nNodes(0),
      activeNode(nullptr), activeEdge(-1),
      activeLength(0), remainder(0), lastNewNode(nullptr) {}

SuffixTree::~SuffixTree() {
    freeTree(root);
}

void SuffixTree::freeTree(Node* node) {
    if (!node) return;
    for (int i = 0; i < 128; i++)
        freeTree(node->children[i]);
    if (node->end != &leafEnd)
        delete node->end;
    delete node;
}


// helperss

Node* SuffixTree::newNode(int start, int* end) {
    Node* n = new Node(start, end);
    n->suffixLink = root;
    nNodes++;
    return n;
}

int SuffixTree::edgeLen(Node* node) const {
    return *node->end - node->start + 1;
}

bool SuffixTree::walkDown(Node* node) {
    int len = edgeLen(node);
    if (activeLength >= len) {
        activeEdge += len;
        activeLength -= len;
        activeNode = node;
        return true;
    }
    return false;
}

// ============================================================
// Fase de Ukkonen
// ============================================================
void SuffixTree::extend(int pos) {
    leafEnd = pos;
    remainder++;
    lastNewNode = nullptr;

    while (remainder > 0) {
        if (activeLength == 0)
            activeEdge = pos;

        int aeIdx = text[activeEdge];
        Node* child = activeNode->children[aeIdx];

        if (!child) {
            // Regla 2: crear hoja
            activeNode->children[aeIdx] = newNode(pos, &leafEnd);
            if (lastNewNode) {
                lastNewNode->suffixLink = activeNode;
                lastNewNode = nullptr;
            }
        } else {
            if (walkDown(child)) continue;

            // Regla 3: el caracter ya existe
            if (text[child->start + activeLength] == text[pos]) {
                if (lastNewNode && activeNode != root) {
                    lastNewNode->suffixLink = activeNode;
                    lastNewNode = nullptr;
                }
                activeLength++;
                break;
            }

            // Regla 2: split
            int* splitEnd = new int(child->start + activeLength - 1);
            Node* split = newNode(child->start, splitEnd);
            activeNode->children[aeIdx] = split;

            split->children[text[pos]] = newNode(pos, &leafEnd);
            child->start += activeLength;
            split->children[text[child->start]] = child;

            if (lastNewNode)
                lastNewNode->suffixLink = split;
            lastNewNode = split;
        }

        remainder--;

        if (activeNode == root && activeLength > 0) {
            activeLength--;
            activeEdge = pos - remainder + 1;
        } else if (activeNode != root) {
            activeNode = activeNode->suffixLink;
        }
    }
}

// buildUkkonen construye el Suffix Tree para el texto dado usando el algoritmo de Ukkonen.
void SuffixTree::buildUkkonen(string text) {
    freeTree(root);
    root = nullptr;
    nNodes = 0;

    this->text = std::move(text);
    this->text += kTerminal;
    leafEnd = -1;

    root = new Node(-1, new int(-1));
    root->suffixLink = root;
    nNodes = 1;

    activeNode = root;
    activeEdge = -1;
    activeLength = 0;
    remainder = 0;
    lastNewNode = nullptr;

    for (int i = 0; i < (int)this->text.size(); i++)
        extend(i);

    finalize();
}

// finalize: asigna suffixIndex a cada hoja

void SuffixTree::finalize() {
    struct Frame { Node* node; int height; };
    stack<Frame> st;
    st.push({root, 0});

    while (!st.empty()) {
        auto [node, height] = st.top();
        st.pop();

        bool isLeaf = true;
        for (int c = 0; c < 128; c++) {
            if (node->children[c]) {
                isLeaf = false;
                st.push({node->children[c], height + edgeLen(node->children[c])});
            }
        }
        if (isLeaf && node != root)
            node->suffixIndex = (int)text.size() - height;
    }
}


// Busqueda

bool SuffixTree::matchPattern(const string& pat, Node*& matchEnd,
                              int* nodesVisited, int* charsCompared,
                              vector<Node*>* path) const {
    if (pat.empty() || !root) return false;

    Node* node = root;
    if (path) path->push_back(root);

    int m = (int)pat.size();
    int i = 0;
    while (i < m) {
        int c = pat[i];
        Node* child = node->children[c];
        if (!child) return false;

        if (nodesVisited) (*nodesVisited)++;
        if (path) path->push_back(child);

        int j = child->start;
        int e = *child->end;
        while (j <= e && i < m) {
            if (charsCompared) (*charsCompared)++;
            if (text[j] != pat[i]) return false;
            j++; i++;
        }
        node = child;
    }

    matchEnd = node;
    return true;
}

void SuffixTree::collectLeaves(Node* node, vector<int>& out) const {
    stack<Node*> st;
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

int SuffixTree::countLeaves(Node* node) const {
    int count = 0;
    stack<Node*> st;
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


bool SuffixTree::contains(string pattern) const {
    Node* end = nullptr;
    return matchPattern(pattern, end, nullptr, nullptr, nullptr);
}

int SuffixTree::countOccurrences(string pattern) const {
    Node* end = nullptr;
    if (!matchPattern(pattern, end, nullptr, nullptr, nullptr))
        return 0;
    return countLeaves(end);
}

vector<int> SuffixTree::findOccurrences(string pattern) const {
    vector<int> out;
    Node* end = nullptr;
    if (!matchPattern(pattern, end, nullptr, nullptr, nullptr))
        return out;
    collectLeaves(end, out);
    return out;
}

SuffixTree::SearchResult SuffixTree::search(string pattern) const {
    SearchResult r;
    Node* end = nullptr;
    r.found = matchPattern(pattern, end, &r.nodesVisited, &r.charsCompared, &r.path);
    if (r.found)
        r.count = countLeaves(end);
    return r;
}

string SuffixTree::edgeLabel(Node* node) const {
    if (!node || node == root || node->start < 0) return "";
    int s = node->start;
    int e = *node->end;
    if (e >= (int)text.size()) e = (int)text.size() - 1;
    string lbl = text.substr(s, e - s + 1);
    for (char& ch : lbl)
        if (ch == kTerminal) ch = '$';
    return lbl;
}
