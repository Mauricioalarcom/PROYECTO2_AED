#ifndef SUFFIX_TREE_H
#define SUFFIX_TREE_H

#include <string>
#include <vector>
using namespace std;

struct Node {
    int start;
    int* end;
    Node* suffixLink;
    Node* children[128];
    int suffixIndex;

    Node(int s, int* e);
};

class SuffixTree {
public:
    struct SearchResult {
        bool found = false;
        long long count = 0;
        int nodesVisited = 0;
        int charsCompared = 0;
        vector<Node*> path;
    };

    SuffixTree();
    ~SuffixTree();

    SuffixTree(const SuffixTree&) = delete;
    SuffixTree& operator=(const SuffixTree&) = delete;

    void buildUkkonen(std::string text);
    bool contains(std::string pattern) const;
    int countOccurrences(std::string pattern) const;
    std::vector<int> findOccurrences(std::string pattern) const;

    SearchResult search(std::string pattern) const;

    string edgeLabel(Node* node) const;
    int nodeCount() const { return nNodes; }
    const string& getText() const { return text; }  

    static constexpr char kTerminal = '\x01';

private:
    string text;
    Node* root;
    int leafEnd;
    int nNodes;

    Node* activeNode;
    int activeEdge;
    int activeLength;
    int remainder;
    Node* lastNewNode;

    Node* newNode(int start, int* end);
    void freeTree(Node* node);
    int edgeLen(Node* node) const;
    bool walkDown(Node* node);
    void extend(int pos);
    void finalize();

    bool matchPattern(const string& pat, Node*& matchEnd,
                      int* nodesVisited, int* charsCompared,
                      vector<Node*>* path) const;
    void collectLeaves(Node* node, vector<int>& out) const;
    int countLeaves(Node* node) const;
};

#endif // SUFFIX_TREE_H
