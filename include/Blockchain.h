#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include "BPlusTree.h"
#include "HashModule.h"
#include <string>
#include <map>
#include <unordered_map>
using namespace std;

struct BlockNode {
    int blockID;    // in B+ Trees
    int roll_no;
    int vote;       // the candidate no.
    string reciept;
    string timestamp;
    string currHash;
    BlockNode* next;
};

class VotingBlockchain{
    public:
        BlockNode* head;
        BlockNode* tail;
        int blockCount;
        BPlusTreeDB& database;     // Reference to your B+ Tree Database
        HashModule& hashTable;   // Reference to your friend's Hash Table
        int candidateCount;    // no.of candidates;

        string getCurrentTimestamp();

        VotingBlockchain(BPlusTreeDB& DB, HashModule& HT);

        bool castVote(int rollNumber, int candidateID);
        BlockNode* verifyVotes(BlockNode* start, BlockNode* end);
        unordered_map<int,int> displayElectionTally();

        // Demo helper: Simulates tampering to prove detection during review
        bool tamperBlockForDemo(int blockID, int newCandidateID);
        string generateReceipt();
        string calculateSHA256(int rollNumber, int vote,string timestamp, string receipt, string prevHash);
        void deleteNode(BlockNode* node);
        
};

#endif