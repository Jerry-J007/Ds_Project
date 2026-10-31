#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include "BPlusTree.h"
#include "HashTable.h"
#include <string>
#include <map>
#include <unordered_map>
using namespace std;

struct BlockNode{
    int blockID;    // in B+ Trees
    int roll_no;
    int vote;       // the candidate no.
    string timestamp;
    string currHash;
    string prevHash;
    BlockNode* next;
};

class VotingBlockchain{
    public:
        string title;
        BlockNode* head;
        BlockNode* tail;
        int blockCount;     // no.of ppl voted
        BPlusTreeDB database;     
        HashTable<BlockNode*> hashTable;   
        int admin;
        vector<int> resgisteredVoters;
        vector<int> candidates;
        string getCurrentTimestamp();

        VotingBlockchain(string TITLE, int adminRoll, vector<int> candidatesArr,vector<int> REGISTEREDVOTERS);

        void retrieveChain(int BLOCKid, int ROLL, int VOTE, string TIMESTAMP, string CURRHASH, string PREVHASH);

        void castVote(int rollNumber, int candidateID);
        BlockNode* verifyVotes(BlockNode* start, BlockNode* end);
        unordered_map<int,int> displayElectionTally();

        string calculateSHA256(int rollNumber, int vote,string timestamp, string prevHash);
        void deleteNode(BlockNode* &node);
        
};

#endif