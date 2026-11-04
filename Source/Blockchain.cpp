#include "../include/Blockchain.h"
#include "../include/Digest.h"
#include <iostream>
#include <sstream>   // to build a string using stream operations
#include <iomanip>   // for formating operations like adding padding, filling ith 0s ,etc
#include <unordered_map>
#include <ctime>

VotingBlockchain::VotingBlockchain(string TITLE,int adminRoll, vector<int> candidatesArr, vector<int> REGISTEREDVOTERS, vector<string> namesArr){
    title = TITLE;
    head = nullptr;
    tail = nullptr;
    blockCount = 0;
    ended = false;
    totalVoters = 0;
    totalVotes = 0;
    admin = adminRoll;
    candidates = candidatesArr;
    candidateNames = namesArr;
    while(candidateNames.size() < candidates.size()) candidateNames.push_back(to_string(candidates[candidateNames.size()]));   // no name given: show the number
    resgisteredVoters = REGISTEREDVOTERS;
    for(int voters: REGISTEREDVOTERS){
        hashTable.registerEntry(voters);
    }
}

VotingBlockchain::~VotingBlockchain(){
    // the B+ tree and the hash table only point at the blocks, so the blocks are freed here, once
    BlockNode* node = head;
    while(node){
        BlockNode* next = node->next;
        delete node;
        node = next;
    }
}

void VotingBlockchain::endPoll(){
    if(ended) return;
    unordered_map<int,int> tally = displayElectionTally();
    finalVotes.assign(candidates.size(), 0);
    for(size_t i = 0; i < candidates.size(); i++){
        auto found = tally.find(candidates[i]);
        if(found != tally.end()) finalVotes[i] = found->second;
    }
    totalVoters = resgisteredVoters.size();
    totalVotes = blockCount;
    endedAt = getCurrentTimestamp();

    BlockNode* node = head;                 // only the statistics are kept from here on
    while(node){
        BlockNode* next = node->next;
        delete node;
        node = next;
    }
    head = nullptr;
    tail = nullptr;
    blockCount = 0;
    database.clear();
    hashTable.clear();
    resgisteredVoters.clear();
    ended = true;
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
    database.insert(BLOCKid, newNode);
}

string VotingBlockchain::getCurrentTimestamp(){
    time_t now = time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d_%H:%M:%S", localtime(&now));
    return buf;
}

string VotingBlockchain::calculateSHA256(int blockID, int rollNumber, int vote,string timestamp, string prevHash){
    string fields = to_string(blockID)+"|"+to_string(rollNumber)+"|"+to_string(vote)+"|"+timestamp+"|"+ prevHash; 
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
    node->blockID = blockCount/3;
    blockCount++;
    node->roll_no = rollNumber;
    node->vote = candidateID;
    node->timestamp = getCurrentTimestamp();
    node->next = nullptr;  
    if(!head){
        head = node;
        tail = node;
        node->prevHash = "0";
        node->currHash = calculateSHA256(node->blockID, node->roll_no, node->vote, node->timestamp, "0");
    }
    else{
        node->prevHash = tail->currHash;
        node->currHash = calculateSHA256(node->blockID, node->roll_no, node->vote, node->timestamp, tail->currHash);
        tail->next = node;
        tail = node;
    }
    hashTable.insert(rollNumber, node);
    database.insert(node->blockID, node);
    return;
}

BlockNode* VotingBlockchain::verifyVotes(BlockNode* start, BlockNode* end){
    if(!start || !end) return nullptr;
    BlockNode* ptr = start;
    string hash = start->prevHash;
    while(ptr && ptr != end->next){
        hash = calculateSHA256(ptr->blockID, ptr->roll_no, ptr->vote, ptr->timestamp, hash);
        if(ptr->currHash != hash) return ptr;
        ptr = ptr->next;
    }
    return nullptr;
}

void VotingBlockchain::deleteNode(BlockNode* &node){
    if(!node) return;
    BlockNode* prev = nullptr;
    BlockNode* cur = head;
    while(cur && cur != node){
        prev = cur;
        cur = cur->next;
    }
    if(!cur) return;                   // not part of this chain
    BlockNode* after = node->next;
    if(prev) prev->next = after;
    else head = after;
    if(tail == node) tail = prev;
    delete node;
    node = nullptr;
    blockCount--;

    // re-chain: every block after the removed one gets a new hash; block ids and the B+ tree index are rebuilt
    database.clear();
    string previousHash = "0";
    int index = 0;
    bool rehash = (prev == nullptr);
    for(BlockNode* ptr = head; ptr; ptr = ptr->next, index++){
        ptr->blockID = index/3;                         // set first: the block id is part of the hash
        if(rehash){
            ptr->prevHash = previousHash;
            ptr->currHash = calculateSHA256(ptr->blockID, ptr->roll_no, ptr->vote, ptr->timestamp, previousHash);
        }
        previousHash = ptr->currHash;
        database.insert(ptr->blockID, ptr);
        if(ptr == prev) rehash = true;      // blocks after the predecessor are re-hashed
    }
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
