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
        ~HashTable();                       // frees the entries (the stored values belong to someone else)
        HashTable(const HashTable&) = delete;               // a copy would free the same entries twice
        HashTable& operator=(const HashTable&) = delete;
        void registerEntry(int k);                          // Just registers the entry, the pointer is still null
        void insert(int k, V val);                      // can be called only on registered entries, adds value (pointer) to the respective key
        int stateCheck(int k);              // To check if a user is allowed to vote and if he has voted
        V find(int k);
        bool remove(int k);                     // deletes the register entry
        bool removeEntry(int k);                // only deletes the node entry, register entry is still there
        void clear();                       // drops every entry (values are not freed)
};

#include "HashTable.tpp"
#endif