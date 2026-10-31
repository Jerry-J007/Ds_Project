#ifndef BALLOT_H
#define BALLOT_H

#include "HashTable.h"
#include "Blockchain.h"
#include <string>

using namespace std;

struct BallotNode{
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

        BallotChain();
        void retrieveChain(VotingBlockchain* VOTES);
        void createBallot(string TITLE, int ADMIN, vector<int> CAND, vector<int> RGU);
        void deleteballot(BallotNode* node);


};




#endif