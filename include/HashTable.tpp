template <typename V>
HashTable<V>::HashTable(){
    table.resize(TABLE_SIZE);
}

template <typename V>
void HashTable<V>::clear(){
    for(Entry*& head : table){
        Entry* node = head;
        while(node){
            Entry* next = node->next;
            delete node;
            node = next;
        }
        head = nullptr;
    }
}

template <typename V>
HashTable<V>::~HashTable(){
    clear();
}

template <typename V>
void HashTable<V>::registerEntry(int k){
    if(stateCheck(k) != 0) return;      // already registered
    int index = k%TABLE_SIZE;
    Entry* node = new Entry;
    node->key = k;
    node->value = nullptr;
    node->next = nullptr;
    node->next = table[index];      // inserting new entries at head; O(1) insertion
    table[index] = node;
}

template <typename V>
void HashTable<V>::insert(int k, V val){
    if(stateCheck(k) != 1) return;
    int index = k%TABLE_SIZE;
    Entry* node = table[index];
    while(node){
        if(node->key == k){
            node->value = val;
        }
        node = node->next;
    }
}

template <typename V>
int HashTable<V>::stateCheck(int k){        // 0 - not registered, 1 - registered but not voted, 2 - registered and voted
    int index = k%TABLE_SIZE;
    Entry* node = table[index];
    while(node){
        if(node->key == k) return !(node->value == nullptr) ? 2:1;
        node = node->next;
    }
    return 0;
}

template <typename V>
V HashTable<V>::find(int k){
    int index = k%TABLE_SIZE;
    Entry* node = table[index];
    while(node){
        if(node->key == k) return node->value;
        node = node->next;
    }
    return nullptr;
}

template <typename V>
bool HashTable<V>::remove(int k){
    int index = k%TABLE_SIZE;
    Entry* node= table[index];
    while(node){
        if(node->key == k){
            table[index] = node->next;
            delete node;
            return true;
        }
        if(node->next && node->next->key == k){
            Entry* temp = node->next;
            node->next = node->next->next;
            delete temp;
            return true;
        }
        node = node->next;
    }
    return false;
}

template <typename V>
bool HashTable<V>::removeEntry(int k){
    int index = k%TABLE_SIZE;
    Entry* node = table[index];
    while(node){
        if(node->key == k){
            node->value = nullptr;
            return true;
        }
        node = node->next;
    }
    return false;
}
