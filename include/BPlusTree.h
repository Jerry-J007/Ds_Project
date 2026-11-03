// BPlusTree.h
#ifndef BPLUSTREE_H
#define BPLUSTREE_H

#include <vector>
#include <string>

struct BlockNode;

class BPlusNode {
public:
    bool isLeaf;
    std::vector<int> keys;                  // Stores BlockIDs for binary search bounding within the node
    std::vector<BPlusNode*> children;       // Internal node routing pointers for O(log N) traversal
    std::vector<BlockNode*> records;        // Leaf node pointers directly to the in-memory Blockchain linked list ledger
    BPlusNode* nextLeaf;                    // Singly linked list of leaves for O(K) sequential range queries
    BPlusNode* parent; // Eliminates O(N) findParent bottleneck during bottom-up node splits

    BPlusNode(bool leaf);
};

class BPlusTreeDB {
private:
    BPlusNode* root;
    const int MAX_KEYS = 3;                 // Tree order (m=4) constraint to force frequent splits for testing
    std::string dbFilename;                 // Target binary database persistence file (e.g., votes_btree.db)

    // Handles upward propagation of keys when an internal node exceeds MAX_KEYS
    void insertInternal(int key, BPlusNode* cursor, BPlusNode* child);
    
    // Post-order traversal to safely free memory and prevent leaks
    void destroyTree(BPlusNode* node);

public:
    BPlusTreeDB(const std::string& filename = "votes_btree.db");
    ~BPlusTreeDB();

    // Core B+ Tree operations maintaining logarithmic time complexities
    void insert(int blockID, BlockNode* blockPtr);
    BlockNode* search(int blockID) const;
    std::vector<BlockNode*> rangeQuery(int startBlockID, int endBlockID) const;

    void clear();                       // drop the whole index (used when the chain is rebuilt from disk)
    void displayTreeStructure() const;  // Level-order BFS traversal for debugging tree topology
    
    // Ensures ACID-like atomicity during ledger persistence to avoid corrupted state on power/crash failures
    bool saveLinkedListToDiskAtomically(BlockNode* head) const;
    const std::string& getDbFilename() const { return dbFilename; }
};

#endif