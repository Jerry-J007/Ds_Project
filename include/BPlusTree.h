#ifndef BPLUSTREE_H
#define BPLUSTREE_H

#include <string>
#include <vector>
#include <fstream>

// Record stored inside the B+ Tree Leaf Database
struct VoteDBRecord {
    int blockID;             // Primary Key (Indexed in B+ Tree)
    int candidateID;         // Vote choice
    std::string timestamp;   // Time of vote
    std::string blockHash;   // Current block hash for cross-verification
    std::string prevHash;    // Previous block hash
};

class BPlusNode {
public:
    bool isLeaf;
    std::vector<int> keys;
    std::vector<BPlusNode*> children;       // Used only by internal nodes
    std::vector<VoteDBRecord> records;      // Used only by leaf nodes
    BPlusNode* nextLeaf;                    // Leaf-level linked list pointer

    BPlusNode(bool leaf);
};

class BPlusTreeDB {
private:
    BPlusNode* root;
    const int MAX_KEYS = 3; // Small degree so splits are easy to demo in viva
    std::string dbFilename;

    void insertInternal(int key, BPlusNode* cursor, BPlusNode* child);
    BPlusNode* findParent(BPlusNode* cursor, BPlusNode* child);
    void destroyTree(BPlusNode* node);
    void printLevelOrder(BPlusNode* node) const;

public:
    BPlusTreeDB(const std::string& filename = "votes_btree.db");
    ~BPlusTreeDB();

    // Core B+ Tree Operations
    void insert(const VoteDBRecord& record);
    VoteDBRecord* search(int blockID) const;
    std::vector<VoteDBRecord> rangeQuery(int startBlockID, int endBlockID) const;

    // Database Persistence & Visualization
    void displayTreeStructure() const;
    bool persistToDisk() const;
    void loadFromDisk();
};

#endif