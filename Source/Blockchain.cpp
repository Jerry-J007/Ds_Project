#include "../include/Blockchain.h"
#include "../include/Digest.h"
#include <iostream>
#include <random>  // to generate random no. for the receipt
#include <sstream>   // to build a string using stream operations
#include <iomanip>   // for formating operations like adding padding, filling ith 0s ,etc
#include <unordered_map>
#include <ctime>

VotingBlockchain::VotingBlockchain(string TITLE,int adminRoll, vector<int> candidatesArr, vector<int> REGISTEREDVOTERS){
    title = TITLE;
    head = nullptr;
    tail = nullptr;
    blockCount = 0;
    admin = adminRoll;
    candidates = candidatesArr;
    resgisteredVoters = REGISTEREDVOTERS;
    for(int voters: REGISTEREDVOTERS){
        hashTable.registerEntry(voters);
    }
}

void VotingBlockchain::retrieveChain(int BLOCKid, int ROLL, int VOTE, string TIMESTAMP, string CURRHASH, string PREVHASH){
    BlockNode* newNode = new BlockNode;

    newNode->blockID = BLOCKid;
    newNode->roll_no = ROLL;
    newNode->vote = VOTE;
    newNode->timestamp = TIMESTAMP;
    newNode->currHash = CURRHASH;
    newNode->prevHash = PREVHASH;
    newNode->next = nullptr;
    if(!head){
        head = newNode;
        tail = newNode;
    }
    else{
        tail->next = newNode;
        tail = newNode;
    }
    blockCount++;
    hashTable.insert(ROLL, newNode);
    // inser in B+ Tree as well    (pending)......
}

string VotingBlockchain::getCurrentTimestamp(){
    time_t now = time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d_%H:%M:%S", localtime(&now));
    return buf;
}

string VotingBlockchain::calculateSHA256(int rollNumber, int vote,string timestamp, string prevHash){
    string fields = to_string(rollNumber)+"|"+to_string(vote)+"|"+timestamp+"|"+ prevHash; 
    return sha256(fields);
}

void VotingBlockchain::castVote(int rollNumber, int candidateID){
    // Step 1: O(1) Hash Table Eligibility Check
    if(hashTable.stateCheck(rollNumber) == 0){
        return;
    }
    if(hashTable.stateCheck(rollNumber) == 2){
        return;
    }
    BlockNode* node = new BlockNode;
    node->blockID = blockCount/15;   //  15 is hardcoded, but it is the no.of pointers in a block in B+ trees
    blockCount++;
    node->roll_no = rollNumber;
    node->vote = candidateID;
    node->timestamp = getCurrentTimestamp();
    node->next = nullptr;  
    if(!head){
        head = node;
        tail = node;
        node->prevHash = "0";
        node->currHash = calculateSHA256(node->roll_no, node->vote, node->timestamp, "0");
    }
    else{
        node->prevHash = tail->currHash;
        node->currHash = calculateSHA256(node->roll_no, node->vote, node->timestamp,tail->currHash);
        tail->next = node;
        tail = node;
    }
    hashTable.insert(rollNumber, node);
    // make entry in B+ Tree as well ( pending)...
    return;
}

BlockNode* VotingBlockchain::verifyVotes(BlockNode* start, BlockNode* end){
    if(!start || !end) return nullptr;
    BlockNode* ptr = start;
    string hash = start->prevHash;
    while(ptr && ptr != end->next){
        hash = calculateSHA256(ptr->roll_no, ptr->vote, ptr->timestamp,hash);
        if(ptr->currHash != hash) return ptr;
        ptr = ptr->next;
    }
    return nullptr;
}

void VotingBlockchain::deleteNode(BlockNode* &node){
    if(!node) return;
    BlockNode* ptr = node->next;
    string previousHash = node->prevHash;
    while(ptr){
        ptr->prevHash = previousHash;
        ptr->currHash = calculateSHA256(ptr->roll_no, ptr->vote,ptr->timestamp, previousHash);
        previousHash = ptr->currHash;
        ptr = ptr->next;
    }
    BlockNode** pointer = &node;
    *pointer = node->next;
    blockCount--;
    delete node;
}

unordered_map<int,int> VotingBlockchain::displayElectionTally(){
    BlockNode* ptr = head;
    unordered_map<int,int> tally;
    while(ptr){
        tally[ptr->vote]++;
        ptr = ptr->next;
    }
    return tally;
}
