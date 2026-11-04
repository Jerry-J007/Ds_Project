#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include "BPlusTree.h"
#include "HashTable.h"
#include <string>
#include <vector>
#include <unordered_map>
using namespace std;

struct BlockNode{
    int blockID;    // The block.no in B+ Trees under which the node is listed
    int roll_no;
    int vote;       // the candidate no.
    string timestamp;
    string currHash;        // hash of the contents
    string prevHash;        // hash of previous node
    BlockNode* next;
};

class VotingBlockchain{
    public:
        string title;       
        BlockNode* head;
        BlockNode* tail;
        int blockCount;             // no.of ppl voted
        BPlusTreeDB database;     
        HashTable<BlockNode*> hashTable;   
        int admin;
        vector<int> resgisteredVoters;          // All these users are allowed to vote
        vector<int> candidates;             
        vector<string> candidateNames;     // candidateNames[i] is the name of candidates[i]

        // when admin ends the poll: the chain is thrown away and only these statistics are kept
        bool ended;
        string endedAt;
        int totalVoters;                   // registered voters when the poll ended
        int totalVotes;                    // votes cast
        vector<int> finalVotes;            // finalVotes[i] = votes of candidates[i]
        string getCurrentTimestamp();

        VotingBlockchain(string TITLE, int adminRoll, vector<int> candidatesArr,vector<int> REGISTEREDVOTERS, vector<string> namesArr = {});
        ~VotingBlockchain();                // frees every block of the chain
        VotingBlockchain(const VotingBlockchain&) = delete;
        VotingBlockchain& operator=(const VotingBlockchain&) = delete;

        void retrieveChain(int BLOCKid, int ROLL, int VOTE, string TIMESTAMP, string CURRHASH, string PREVHASH);            // To read and load the votechain of a ballot from disk

        void castVote(int rollNumber, int candidateID);
        void endPoll();                     // freeze the tally, free the blocks
        BlockNode* verifyVotes(BlockNode* start, BlockNode* end);       // tamper detection function
        unordered_map<int,int> displayElectionTally();                  // To see the current status of the election

        string calculateSHA256(int blockID, int rollNumber, int vote,string timestamp, string prevHash);        // For hashing node content
        void deleteNode(BlockNode* &node);
        
};

#endif