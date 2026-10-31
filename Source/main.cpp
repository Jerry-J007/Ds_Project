#include "../include/BPlusTree.h"
#include "../include/Ballot.h"
#include "../include/HashTable.h"
#include "../include/Blockchain.h"
#include <iostream>
#include <vector>
#include <filesystem>
#include <sstream>
#include <fstream>
#include <ctime>
using namespace std;

struct userNode{
    int roll;
    string pw;
    userNode* next;
};
HashTable<userNode*> users = new HashTable<userNode*>;
BallotChain* ballots = new BallotChain;

int showVote(int ballotID, int RollNo);
void remove_vote(int ballotID, int ADMIN, int RollNo);
void remove_voter(int ballotID, int ADMIN, int RollNo);
BlockNode* checkTamper(int ballotID, int choice, pair<int,int> interval);
void insert(vector<pair<int,int>>&arr, pair<int,int> element);
vector<pair<int,int>> getTally(int ballotID);
int checkState(int ballotID, int RollNo);
void Vote(int ballotID, int RollNo, int VOTE);
VotingBlockchain* loadBallot(const filesystem::path& entry);
void loadBallots();
void storeballot(BallotNode* ballot);
void storeballots();
void loadUsers(HashTable<userNode*>& users);
bool authorizeUser(int roll, string pw);
void removeBallot(int ballotID);

int main(){
    loadUsers(users);
    loadBallots();

}

int showVote(int ballotID, int RollNo){
    BallotNode* ballot = ballots->hashTable.find(ballotID);
    BlockNode* node = ballot->votes->hashTable.find(RollNo);
    return node->vote;
}

void remove_vote(int ballotID, int ADMIN, int RollNo){
    BallotNode* ballot = ballots->hashTable.find(ballotID);
    BlockNode* node = ballot->votes->hashTable.find(RollNo);
    ballot->votes->hashTable.removeEntry(RollNo);
    ballot->votes->deleteNode(node);
}

void remove_voter(int ballotID, int ADMIN, int RollNo){
    BallotNode* ballot = ballots->hashTable.find(ballotID);
    BlockNode* node = ballot->votes->hashTable.find(RollNo);
    ballot->votes->deleteNode(node);
    ballot->votes->hashTable.remove(RollNo);
}

BlockNode* checkTamper(int ballotID, int choice, pair<int,int> interval){        // choice: 0-entire, 1 - interval 
    BallotNode* ballot = ballots->hashTable.find(ballotID);
    if(!choice){
        BlockNode* tamperedNode = ballot->votes->verifyVotes(ballot->votes->head, ballot->votes->tail);
        return tamperedNode;
    }
    else{
        // B+ tree handles this case (pending)....
    }
}

void insert(vector<pair<int,int>>&arr, pair<int,int> element){
    int i = arr.size()-1;
    if(i <0){
        arr.push_back(element);
        return;
    }
    arr.push_back(element);
    i++;
    while(i > 0){
        if(arr[i-1].second > arr[i].second) break; 
        swap(arr[i], arr[i+1]);
        i--;
    }
}

vector<pair<int,int>> getTally(int ballotID){
    BallotNode* ballot = ballots->hashTable.find(ballotID);
    unordered_map<int,int> tally = ballot->votes->displayElectionTally();
    vector<pair<int,int>> votes;
    for( pair<int,int> vote : tally){
        insert(votes,vote);
    }
    return votes;
}

int checkState(int ballotID, int RollNo){
    BallotNode* ballot = ballots->hashTable.find(ballotID);
    int result = ballot->votes->hashTable.stateCheck(RollNo);
    return result;
}

void Vote(int ballotID, int RollNo, int VOTE){
    BallotNode* ballot = ballots->hashTable.find(ballotID);
    ballot->votes->castVote(RollNo, VOTE);
}

VotingBlockchain* loadBallot(const filesystem::path& entry){
    ifstream in (entry.string());
    string line;
    string piece;

    // first line 
    string title;
    int adminRoll;
    vector<int> candidates;
    getline(in, title, '|');
    getline(in, piece, '|');
    adminRoll = stoi(piece);
    
    if(getline(in, line)){
        stringstream ss(line);
        while(getline(ss, piece, ',')){
            candidates.push_back(stoi(piece));
        }
    }
    vector<int> arr;
    VotingBlockchain* voteChain = new VotingBlockchain(title,adminRoll, candidates,arr);

    //second line
    if(getline(in, line)){
        stringstream ss(line);
        while(getline(ss, piece, ',')){
            voteChain->hashTable.registerEntry(stoi(piece));
            voteChain->resgisteredVoters.push_back(stoi(piece));
        }
    }

    // lines below line 2
    while(getline(in, line)){
        stringstream ss(line);
        // BlockID
        getline(ss, piece, '|');
        int BLOCKID = stoi(piece);
        getline(ss, piece, '|');
        int ROLL = stoi(piece);
        getline(ss, piece, '|');
        int VOTE = stoi(piece);
        getline(ss, piece, '|');
        string TIMESTAMP = piece;
        getline(ss, piece, '|');
        string PREVHASH = piece;
        getline(ss, piece, '|');
        string CURRHASH = piece;
        voteChain->retrieveChain(BLOCKID,ROLL,VOTE,TIMESTAMP,CURRHASH,PREVHASH);
    }
    return voteChain;
}

void loadBallots(){
    for(auto& entry : filesystem::directory_iterator("ballots")){
        if(entry.path().extension() == ".db"){
            VotingBlockchain* voteChain = loadBallot(entry.path());
            ballots->retrieveChain(voteChain);
        }
    }
}

void storeballot(BallotNode* ballot){
    string filename = "./ballots/ballot" + to_string(ballot->id) + ".db";
    ofstream out(filename, ios::trunc);

    //line 1
    out<<ballot->votes->title<<'|'<<ballot->votes->admin<<'|';
    for(int i = 0;i< ballot->votes->candidates.size();i++){
        if(i < i< ballot->votes->candidates.size()-1) out<<ballot->votes->candidates[i]<<',';
        else out<<ballot->votes->candidates[i]<<"\n";
    }
    //line 2
    for(int i = 0;i< ballot->votes->resgisteredVoters.size();i++){
        if(i < i< ballot->votes->resgisteredVoters.size()-1) out<<ballot->votes->resgisteredVoters[i]<<',';
        else out<<ballot->votes->resgisteredVoters[i]<<"\n";
    }
    //line 3 onwards
    BlockNode* node = ballot->votes->head;
    while(node){
        out<<node->blockID<<'|'<<node->roll_no<<"|"<<node->vote<<"|"<<node->timestamp<<"|"<<node->prevHash<<"|"<<node->currHash<<"\n";
        node = node->next;
    }
    out.close();
}

void storeballots(){
    BallotNode* node = ballots->head;
    while(node){
        storeballot(node);
        node = node->next;
    }
}

void loadUsers(HashTable<userNode*>& users){
    ifstream in("users.db");
    string line;
    string piece;
    while(getline(in, line)){
        stringstream ss(line);
        getline(ss, piece, '|');
        int roll = stoi(piece);
        getline(ss, piece);
        users.registerEntry(roll);
        userNode* node = new userNode;
        node->roll = roll;
        node->pw = piece;
        node->next = nullptr;
        users.insert(roll, node);
    }
}

bool authorizeUser(int roll, string pw){            // login check
    int index = roll%(users->TABLE_SIZE);
    userNode* node = users->table[index];
    while(node){
        if(node->roll == roll){
            if(node->pw == pw){
                return true;
            }
            return false;
        }
        node = node->next;
    }
    return false;
}

void removeBallot(int ballotID){
    BallotNode* ballot = ballots->hashTable.find(ballotID);
    ballots->hashTable->remove(ballotID);
    ballots->deleteballot(ballot);
}

/*
use ballots->createBallot(....) to create a ballot directly

*/