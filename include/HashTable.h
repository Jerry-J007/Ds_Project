#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <vector>
using namespace std;

template <typename V>
class HashTable{
    public:
        static const int TABLE_SIZE = 1009; // static is used so that the same table is shared for all class instance and doesn't need extra space for each instance

        struct Entry{
            int key;
            V value;
            Entry* next;
        };

        vector<Entry*> table;

        HashTable();
        void registerEntry(int k);
        void insert(int k, V val);
        int stateCheck(int k);
        V find(int k);
        bool remove(int k);
        bool removeEntry(int k);
};

#include "HashTable.tpp"
#endif