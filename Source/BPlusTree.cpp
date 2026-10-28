#include "../include/BPlusTree.h"
#include <iostream>
#include <queue>
#include <sstream>
#include <algorithm>

BPlusNode::BPlusNode(bool leaf) : isLeaf(leaf), nextLeaf(nullptr) {}

BPlusTreeDB::BPlusTreeDB(const std::string& filename) 
    : root(nullptr), dbFilename(filename) {}

BPlusTreeDB::~BPlusTreeDB() {
    destroyTree(root);
}

void BPlusTreeDB::destroyTree(BPlusNode* node) {
    if (!node) return;
    if (!node->isLeaf) {
        for (BPlusNode* child : node->children) {
            destroyTree(child);
        }
    }
    delete node;
}

// Insert a vote record into the B+ Tree and persist to disk
void BPlusTreeDB::insert(const VoteDBRecord& record) {
    int key = record.blockID;

    // Case 1: Empty tree
    if (!root) {
        root = new BPlusNode(true);
        root->keys.push_back(key);
        root->records.push_back(record);
        persistToDisk();
        return;
    }

    // Traverse to appropriate leaf node
    BPlusNode* cursor = root;
    BPlusNode* parent = nullptr;

    while (!cursor->isLeaf) {
        parent = cursor;
        int idx = 0;
        while (idx < (int)cursor->keys.size() && key >= cursor->keys[idx]) {
            idx++;
        }
        cursor = cursor->children[idx];
    }

    // Insert into leaf in sorted order
    auto it = std::lower_bound(cursor->keys.begin(), cursor->keys.end(), key);
    int insertIdx = std::distance(cursor->keys.begin(), it);
    cursor->keys.insert(it, key);
    cursor->records.insert(cursor->records.begin() + insertIdx, record);

    // If leaf overflows, split the leaf node
    if ((int)cursor->keys.size() > MAX_KEYS) {
        BPlusNode* newLeaf = new BPlusNode(true);
        int splitIdx = (MAX_KEYS + 1) / 2;

        newLeaf->keys.assign(cursor->keys.begin() + splitIdx, cursor->keys.end());
        newLeaf->records.assign(cursor->records.begin() + splitIdx, cursor->records.end());

        cursor->keys.resize(splitIdx);
        cursor->records.resize(splitIdx);

        // Maintain leaf linked list for fast range queries
        newLeaf->nextLeaf = cursor->nextLeaf;
        cursor->nextLeaf = newLeaf;

        int promotedKey = newLeaf->keys[0];

        if (cursor == root) {
            BPlusNode* newRoot = new BPlusNode(false);
            newRoot->keys.push_back(promotedKey);
            newRoot->children.push_back(cursor);
            newRoot->children.push_back(newLeaf);
            root = newRoot;
        } else {
            insertInternal(promotedKey, parent, newLeaf);
        }
    }
    persistToDisk();
}

// Recursive internal node split handler
void BPlusTreeDB::insertInternal(int key, BPlusNode* cursor, BPlusNode* child) {
    auto it = std::lower_bound(cursor->keys.begin(), cursor->keys.end(), key);
    int idx = std::distance(cursor->keys.begin(), it);

    cursor->keys.insert(it, key);
    cursor->children.insert(cursor->children.begin() + idx + 1, child);

    if ((int)cursor->keys.size() > MAX_KEYS) {
        BPlusNode* newInternal = new BPlusNode(false);
        int splitIdx = (MAX_KEYS + 1) / 2;
        int promotedKey = cursor->keys[splitIdx];

        newInternal->keys.assign(cursor->keys.begin() + splitIdx + 1, cursor->keys.end());
        newInternal->children.assign(cursor->children.begin() + splitIdx + 1, cursor->children.end());

        cursor->keys.resize(splitIdx);
        cursor->children.resize(splitIdx + 1);

        if (cursor == root) {
            BPlusNode* newRoot = new BPlusNode(false);
            newRoot->keys.push_back(promotedKey);
            newRoot->children.push_back(cursor);
            newRoot->children.push_back(newInternal);
            root = newRoot;
        } else {
            BPlusNode* parent = findParent(root, cursor);
            insertInternal(promotedKey, parent, newInternal);
        }
    }
}

BPlusNode* BPlusTreeDB::findParent(BPlusNode* cursor, BPlusNode* child) {
    if (!cursor || cursor->isLeaf) return nullptr;
    for (BPlusNode* c : cursor->children) {
        if (c == child) return cursor;
        BPlusNode* res = findParent(c, child);
        if (res) return res;
    }
    return nullptr;
}

// O(log N) Point Lookup by Block ID
VoteDBRecord* BPlusTreeDB::search(int blockID) const {
    if (!root) return nullptr;
    BPlusNode* cursor = root;
    while (!cursor->isLeaf) {
        int idx = 0;
        while (idx < (int)cursor->keys.size() && blockID >= cursor->keys[idx]) {
            idx++;
        }
        cursor = cursor->children[idx];
    }
    for (size_t i = 0; i < cursor->keys.size(); ++i) {
        if (cursor->keys[i] == blockID) {
            return &(cursor->records[i]);
        }
    }
    return nullptr;
}

// O(log N + K) Range Query using Leaf Linked List
std::vector<VoteDBRecord> BPlusTreeDB::rangeQuery(int startBlockID, int endBlockID) const {
    std::vector<VoteDBRecord> result;
    if (!root || startBlockID > endBlockID) return result;

    BPlusNode* cursor = root;
    while (!cursor->isLeaf) {
        int idx = 0;
        while (idx < (int)cursor->keys.size() && startBlockID >= cursor->keys[idx]) {
            idx++;
        }
        cursor = cursor->children[idx];
    }

    // Traverse leaf chain horizontally
    while (cursor) {
        for (size_t i = 0; i < cursor->keys.size(); ++i) {
            if (cursor->keys[i] >= startBlockID && cursor->keys[i] <= endBlockID) {
                result.push_back(cursor->records[i]);
            } else if (cursor->keys[i] > endBlockID) {
                return result;
            }
        }
        cursor = cursor->nextLeaf;
    }
    return result;
}

// Visualizes internal routing keys vs leaf data buckets
void BPlusTreeDB::displayTreeStructure() const {
    if (!root) {
        std::cout << "[B+ Tree Database is currently empty]\n";
        return;
    }
    std::cout << "\n=== B+ TREE INDEX STRUCTURE (Max Keys = " << MAX_KEYS << ") ===\n";
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
    std::cout << "===============================================\n";
}

// Writes all leaf records sequentially to a persistent database file
bool BPlusTreeDB::persistToDisk() const {
    if (!root) return false;
    std::ofstream out(dbFilename);
    if (!out.is_open()) return false;

    BPlusNode* cursor = root;
    while (!cursor->isLeaf) cursor = cursor->children[0];

    out << "BlockID,CandidateID,Timestamp,PrevHash,BlockHash\n";
    while (cursor) {
        for (const auto& rec : cursor->records) {
            out << rec.blockID << ","
                << rec.candidateID << ","
                << rec.timestamp << ","
                << rec.prevHash << ","
                << rec.blockHash << "\n";
        }
        cursor = cursor->nextLeaf;
    }
    out.close();
    return true;
}

void BPlusTreeDB::loadFromDisk() {
    std::ifstream in(dbFilename);
    if (!in.is_open()) return;

    std::string line;
    std::getline(in, line); // Skip CSV header
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string token;
        VoteDBRecord rec;

        std::getline(ss, token, ','); rec.blockID = std::stoi(token);
        std::getline(ss, token, ','); rec.candidateID = std::stoi(token);
        std::getline(ss, rec.timestamp, ',');
        std::getline(ss, rec.prevHash, ',');
        std::getline(ss, rec.blockHash, ',');

        insert(rec);
    }
    in.close();
}