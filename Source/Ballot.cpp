#include "../include/Ballot.h"

BallotChain::BallotChain(){
    ballotCount = 0;
    head = nullptr;
    tail = nullptr;
};

BallotChain::~BallotChain(){
    BallotNode* node = head;
    while(node){
        BallotNode* next = node->next;
        delete node->votes;
        delete node;
        node = next;
    }
}

void BallotChain::retrieveChain(VotingBlockchain* VOTES, int id){
    BallotNode* node = new BallotNode;
    node->id = (id < 0) ? ballotCount : id;
    if(node->id >= ballotCount) ballotCount = node->id + 1;
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
    hashTable.registerEntry(node->id);
    hashTable.insert(node->id, node);
}
void BallotChain::createBallot(string TITLE, int ADMIN, vector<int> CAND, vector<int> RGU, vector<string> NAMES){
    BallotNode* ballot = new BallotNode;
    ballot->id = ballotCount++;
    VotingBlockchain* voteChain = new VotingBlockchain(TITLE, ADMIN, CAND, RGU, NAMES);
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
    if(!node) return;
    BallotNode* prev = nullptr;
    BallotNode* cur = head;
    while(cur && cur != node){
        prev = cur;
        cur = cur->next;
    }
    if(!cur) return;
    if(prev) prev->next = node->next;
    else head = node->next;
    if(tail == node) tail = prev;
    delete node->votes;
    delete node;
}
