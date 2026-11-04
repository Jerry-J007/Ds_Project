// BPlusTree.cpp
#include "../include/BPlusTree.h"
#include "../include/Blockchain.h"
#include <iostream>
#include <fstream>
#include <queue>
#include <algorithm>
#include <filesystem>

BPlusNode::BPlusNode(bool leaf) : isLeaf(leaf), nextLeaf(nullptr), parent(nullptr) {}

BPlusTreeDB::BPlusTreeDB(const std::string& filename) 
    : root(nullptr), dbFilename(filename) {}

BPlusTreeDB::~BPlusTreeDB() {
    destroyTree(root);
}

// O(N) recursive post-order teardown to ensure child pointers are freed before their parents
void BPlusTreeDB::destroyTree(BPlusNode* node) {
    if (!node) return;
    if (!node->isLeaf) {
        for (BPlusNode* child : node->children) destroyTree(child);
    }
    delete node; // Returns memory to the heap, critical for continuous server uptime
}

void BPlusTreeDB::insert(int key, BlockNode* blockPtr) {
    if (!root) {
        // Base case: Initialize the tree with a single leaf root
        root = new BPlusNode(true);
        root->keys.push_back(key);
        root->records.push_back(blockPtr);
        return;
    }

    // O(log N) traversal to find the appropriate leaf node based on BlockID
    BPlusNode* cursor = root;
    while (!cursor->isLeaf) {
        int idx = 0;
        while (idx < (int)cursor->keys.size() && key >= cursor->keys[idx]) idx++;
        cursor = cursor->children[idx];
    }

    // upper_bound: equal keys (votes of the same block) keep insertion order
    // Maintains stable sorting for blocks mined at the exact same logical time
    auto it = std::upper_bound(cursor->keys.begin(), cursor->keys.end(), key);
    int insertIdx = std::distance(cursor->keys.begin(), it);
    cursor->keys.insert(it, key);
    cursor->records.insert(cursor->records.begin() + insertIdx, blockPtr);

    // Check against memory constraints: trigger a split if the node exceeds maximum capacity
    if ((int)cursor->keys.size() > MAX_KEYS) {
        BPlusNode* newLeaf = new BPlusNode(true);
        int splitIdx = (MAX_KEYS + 1) / 2; // Floor division for balanced splitting

        // Transfer the right half of the data to the newly allocated leaf
        newLeaf->keys.assign(cursor->keys.begin() + splitIdx, cursor->keys.end());
        newLeaf->records.assign(cursor->records.begin() + splitIdx, cursor->records.end());

        // Truncate the original leaf to keep the left half
        cursor->keys.resize(splitIdx);
        cursor->records.resize(splitIdx);

        // Re-stitch the leaf-level linked list for O(K) sequential access
        newLeaf->nextLeaf = cursor->nextLeaf;
        cursor->nextLeaf = newLeaf;
        newLeaf->parent = cursor->parent;

        // The smallest key in the new right leaf gets promoted to the parent index
        int promotedKey = newLeaf->keys[0];

        if (cursor == root) {
            // Tree height increases by 1: allocate a new root
            BPlusNode* newRoot = new BPlusNode(false);
            newRoot->keys.push_back(promotedKey);
            newRoot->children.push_back(cursor);
            newRoot->children.push_back(newLeaf);
            cursor->parent = newRoot;
            newLeaf->parent = newRoot;
            root = newRoot;
        } else {
            // Propagate the split recursively upwards in O(1) steps via the parent pointer
            insertInternal(promotedKey, cursor->parent, newLeaf);
        }
    }
}

void BPlusTreeDB::insertInternal(int key, BPlusNode* cursor, BPlusNode* child) {
    // Binary search via lower_bound for O(log M) key insertion index within the node
    auto it = std::lower_bound(cursor->keys.begin(), cursor->keys.end(), key);
    int idx = std::distance(cursor->keys.begin(), it);

    cursor->keys.insert(it, key);
    cursor->children.insert(cursor->children.begin() + idx + 1, child);
    child->parent = cursor;

    // Standard B+ Tree internal node split logic
    if ((int)cursor->keys.size() > MAX_KEYS) {
        BPlusNode* newInternal = new BPlusNode(false);
        int splitIdx = (MAX_KEYS + 1) / 2;
        int promotedKey = cursor->keys[splitIdx]; // Internal nodes push the key UP, unlike leaves which copy it

        newInternal->keys.assign(cursor->keys.begin() + splitIdx + 1, cursor->keys.end());
        newInternal->children.assign(cursor->children.begin() + splitIdx + 1, cursor->children.end());
        
        // Reparent the migrated children to point to the new internal node
        for (BPlusNode* c : newInternal->children) c->parent = newInternal;

        cursor->keys.resize(splitIdx);
        cursor->children.resize(splitIdx + 1);
        newInternal->parent = cursor->parent;

        if (cursor == root) {
            BPlusNode* newRoot = new BPlusNode(false);
            newRoot->keys.push_back(promotedKey);
            newRoot->children.push_back(cursor);
            newRoot->children.push_back(newInternal);
            cursor->parent = newRoot;
            newInternal->parent = newRoot;
            root = newRoot;
        } else {
            insertInternal(promotedKey, cursor->parent, newInternal);
        }
    }
}

BlockNode* BPlusTreeDB::search(int blockID) const {
    // Reuses the robust range query logic to execute an O(log N) point lookup
    std::vector<BlockNode*> found = rangeQuery(blockID, blockID);   // first block stored under this key
    return found.empty() ? nullptr : found.front();
}

void BPlusTreeDB::clear() {
    destroyTree(root);
    root = nullptr;
}

std::vector<BlockNode*> BPlusTreeDB::rangeQuery(int startBlockID, int endBlockID) const {
    std::vector<BlockNode*> result;
    if (!root || startBlockID > endBlockID) return result;

    // Step 1: O(log N) vertical traversal to locate the lower bound leaf
    BPlusNode* cursor = root;
    while (!cursor->isLeaf) {
        int idx = 0;
        while (idx < (int)cursor->keys.size() && startBlockID > cursor->keys[idx]) idx++;
        cursor = cursor->children[idx];
    }

    // Step 2: O(K) horizontal traversal across the leaf linked list to harvest matching records
    while (cursor) {
        for (size_t i = 0; i < cursor->keys.size(); ++i) {
            if (cursor->keys[i] >= startBlockID && cursor->keys[i] <= endBlockID) {
                result.push_back(cursor->records[i]);
            } else if (cursor->keys[i] > endBlockID) {
                return result; // Early exit: exploited sorted nature to avoid unnecessary scanning
            }
        }
        cursor = cursor->nextLeaf;
    }
    return result;
}

void BPlusTreeDB::displayTreeStructure() const {
    if (!root) {
        std::cout << "[B+ Tree Index is empty]\n";
        return;
    }
    std::cout << "\n=== B+ TREE RAM INDEX (Leaf Pointers -> Linked List) ===\n";
    
    // Standard Level-Order Traversal (BFS) using a queue
    std::queue<BPlusNode*> q;
    q.push(root);
    int level = 0;

    while (!q.empty()) {
        int sz = q.size();
        std::cout << "Level " << level++ << ": ";
        while (sz--) {
            BPlusNode* curr = q.front();
            q.pop();
            std::cout << (curr->isLeaf ? "[Leaf: " : "[Internal: ");
            for (size_t i = 0; i < curr->keys.size(); ++i) {
                std::cout << curr->keys[i] << (i + 1 < curr->keys.size() ? "," : "");
            }
            std::cout << "]  ";
            if (!curr->isLeaf) {
                for (BPlusNode* c : curr->children) q.push(c);
            }
        }
        std::cout << "\n";
    }
    std::cout << "========================================================\n";
}

bool BPlusTreeDB::saveLinkedListToDiskAtomically(BlockNode* head) const {
    // Writes to a temporary file first to guarantee write safety (no partial writes on crash)
    std::string tempFile = dbFilename + ".tmp";
    std::ofstream out(tempFile);
    if (!out.is_open()) return false;

    // Persist standard blockchain block attributes to plain CSV format
    out << "BlockID,CandidateID,Timestamp,CurrHash\n";
    BlockNode* curr = head;
    while (curr) {
        out << curr->blockID << ","
            << curr->vote << ","
            << curr->timestamp << ","
            << curr->currHash << "\n";
        curr = curr->next;
    }
    out.flush();
    if (!out) return false;                                // write error during system I/O
    out.close();
    
    std::error_code ec;
    // Atomic rename at the OS filesystem level
    std::filesystem::rename(tempFile, dbFilename, ec);     // replaces the old file in one step
    return !ec;
}