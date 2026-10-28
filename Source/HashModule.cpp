#include "../include/HashModule.h"
#include "../include/Blockchain.h"

void HashModule::registerVoter(int rollNumber){
    int index = rollNumber%TABLE_SIZE;
    VoterNode* node = new VoterNode;
    node->roll_no = rollNumber;
    node->hasVoted = false;
    node->next = nullptr;

    node->next = table[index];      // inserting new entries at tail;
    table[index] = node;
}

HashModule::HashModule(){
    table.resize(TABLE_SIZE, nullptr);
}

bool HashModule::canVote(int rollNumber){
    int index = rollNumber%TABLE_SIZE;
    VoterNode* node = table[index];
    while(node){
        if(node->roll_no == rollNumber) return !node->hasVoted;
        node = node->next;
    }
    return false;
}

bool HashModule::markVoted(int rollNumber, BlockNode* nodePointer) {        // returns false if user not found
    int index = rollNumber%TABLE_SIZE;
    VoterNode* node = table[index];
    while(node){
        if(node->roll_no == rollNumber){
            node->hasVoted = true;
            // assign the pointer to LL node
            return true;
        }
        node = node->next;
    }
    return false;
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


