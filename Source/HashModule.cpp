#include "../include/HashModule.h"
#include "../include/Blockchain.h"

void HashModule::registerVoter(int rollNumber){
    int index = rollNumber%TABLE_SIZE;
    VoterNode* node = new VoterNode;
    node->roll_no = rollNumber;
    node->pointer = nullptr;
    node->next = nullptr;

    node->next = table[index];      // inserting new entries at tail;
    table[index] = node;
}

HashModule::HashModule(){
    table.resize(TABLE_SIZE, nullptr);
}

int HashModule::canVote(int rollNumber){        // 0 - not registered, 1 - registered but not voted, 2 - registered and voted
    int index = rollNumber%TABLE_SIZE;
    VoterNode* node = table[index];
    while(node){
        if(node->roll_no == rollNumber) return !(node->pointer == nullptr) ? 2:1;
        node = node->next;
    }
    return 0;
}

BlockNode* HashModule::searchNode(int rollNo){
    int index = rollNo%TABLE_SIZE;
    VoterNode* node = table[index];
    while(node){
        if(node->roll_no == rollNo) return node->pointer;
        node = node->next;
    } 
    return nullptr;
}

bool HashModule::removeVoter(int rollNumber){
    int index = rollNumber%TABLE_SIZE;
    VoterNode* node = table[index];
    while(node){
        if(node->roll_no == rollNumber){
            table[index] = node->next;
            delete node;
            return true;
        }
        if(node->next && node->next->roll_no == rollNumber){
            VoterNode* temp = node->next;
            node->next = node->next->next;
            delete temp;
            return true;
        }
        node = node->next;
    }
    return false;
}

bool HashModule::removeVote(int rollNumber){
    int index = rollNumber%TABLE_SIZE;
    VoterNode* node = table[index];
    while(node){
        if(node->roll_no == rollNumber){
            node->hasVoted = false;
            node->pointer = nullptr;
            return true;
        }
        node = node->next;
    }
    return false;
}


