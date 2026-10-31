#include "../include/Ballot.h"

BallotChain::BallotChain(){
    ballotCount = 0;
    head = nullptr;
    tail = nullptr;
};

void BallotChain::retrieveChain(VotingBlockchain* VOTES){
    BallotNode* node = new BallotNode;
    node->id = ballotCount;
    ballotCount++;
    node->votes = VOTES;
    if(!head){
        head = node;
        tail = node;
    }
    else{
        tail->next = node;
        tail = node;
    }
    node->next = nullptr;
    hashTable.insert(node->id, node);
}
void BallotChain::createBallot(string TITLE, int ADMIN, vector<int> CAND, vector<int> RGU){
    BallotNode* ballot = new BallotNode;
    ballot->id = ballotCount++;
    VotingBlockchain* voteChain = new VotingBlockchain(TITLE, ADMIN, CAND,RGU);          //// fix DB parameter here (pending)....
    ballot->votes = voteChain;
    ballot->next = nullptr;
    if(!head){
        head = ballot;
        tail = ballot;
    }
    else{
        tail->next = ballot;
        tail = ballot;
    }
    hashTable.registerEntry(ballot->id);
    hashTable.insert(ballot->id, ballot);
}
void BallotChain::deleteballot(BallotNode* node){
    BallotNode** pointer = &node;
    *pointer = node->next;
    delete node;
}
