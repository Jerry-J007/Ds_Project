#ifndef BALLOT_H
#define BALLOT_H

#include "HashTable.h"
#include "Blockchain.h"
#include <string>
#include <vector>

using namespace std;

struct BallotNode{          // The Node acts like container for votingchain
    int id;
    VotingBlockchain* votes;        // The blockchain 
    BallotNode* next;    
};


class BallotChain{
    public:
        BallotNode* head;
        BallotNode* tail;
        HashTable<BallotNode*> hashTable;
        int ballotCount;

        BallotChain();                      // constructor
        ~BallotChain();                     // frees every ballot and its chain (destructor)
        BallotChain(const BallotChain&) = delete;
        BallotChain& operator=(const BallotChain&) = delete;
        void retrieveChain(VotingBlockchain* VOTES, int id = -1);       // Used to read and load the ballots into RAM from the disk. (called initially) 
        void createBallot(string TITLE, int ADMIN, vector<int> CAND, vector<int> RGU, vector<string> NAMES = {});           // Any user can create a ballot
        void deleteballot(BallotNode* node);            // Admin of the voting system can choose to delete ballots


};




#endif