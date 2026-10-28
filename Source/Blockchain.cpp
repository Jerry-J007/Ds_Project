#include "../include/Blockchain.h"
#include "../include/Digest.h"
#include <iostream>
#include <random>  // to generate random no. for the receipt
#include <sstream>   // to build a string using stream operations
#include <iomanip>   // for formating operations like adding padding, filling ith 0s ,etc
#include <unordered_map>
#include <ctime>

VotingBlockchain::VotingBlockchain(BPlusTreeDB& DB, HashModule& HT){
    head = nullptr;
    tail = nullptr;
    blockCount = 0;
    database = DB;
    hashTable = HT;
    candidateCount = 0;
}

string VotingBlockchain::getCurrentTimestamp(){
    time_t now = time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d_%H:%M:%S", localtime(&now));
    return buf;
}

string VotingBlockchain::generateReceipt(){
    random_device rd;
    ostringstream out;
    for(int i =0;i<4;i++){          // four rounds of random nop. generated so the probability of getting same receipt is much lesser
        out<<hex<<setw(8)<<setfill('0')<<rd();
    }
    return out.str();
}

string VotingBlockchain::calculateSHA256(int rollNumber, int vote,string timestamp, string receipt, string prevHash){
    string fields = to_string(rollNumber) + to_string(vote) + timestamp + receipt + prevHash; 
    return sha256(fields);
}

bool VotingBlockchain::castVote(int rollNumber, int candidateID){
    // Step 1: O(1) Hash Table Eligibility Check
    if(!hashTable.canVote(rollNumber)){
        return false;
    }
    BlockNode* node = new BlockNode;
    node->blockID = blockCount%15;   //  15 is hardcoded, but it is the no.of pointers in a block in B+ trees
    node->roll_no = rollNumber;
    node->vote = candidateID;
    node->timestamp = getCurrentTimestamp();
    node->next = nullptr;
    node->reciept = generateReceipt();// add the reciept
    
    if(!head){
        head = node;
        tail = node;
        node->currHash = calculateSHA256(node->roll_no, node->vote, node->timestamp, node->reciept, "0");
    }
    else{
        node->currHash = calculateSHA256(node->roll_no, node->vote, node->timestamp, node->reciept, tail->currHash);
        tail->next = node;
        tail = node;
    }
    blockCount++;
    return true;
}

BlockNode* VotingBlockchain::verifyVotes(BlockNode* start, BlockNode* end){
    BlockNode* ptr = start;
    if(end){        // the end is mentioned
        string hash = "0";
        while(ptr != end){
            hash = calculateSHA256(ptr->roll_no, ptr->vote, ptr->timestamp, ptr->reciept,hash);
            if(ptr->currHash != hash) return ptr;
            ptr = ptr->next;
        }
        return nullptr;
    }
    else{
        string hash = "0";
        while(ptr){
            hash = calculateSHA256(ptr->roll_no, ptr->vote, ptr->timestamp, ptr->reciept,hash);
            if(ptr->currHash != hash) return ptr;
            ptr = ptr->next;
        }
        return nullptr;
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
