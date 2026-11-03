#ifndef MAIN_H
#define MAIN_H

#include "../include/Ballot.h"
#include "../include/Blockchain.h"
#include <iostream>
#include <vector>
#include <filesystem>
#include <sstream>
#include <fstream>
#include <ctime>
#include <algorithm>
using namespace std;

struct userNode{        // A linked list to store all the users with their encrypted passwords
    int roll;
    string pw;
    userNode* next;
};

int showVote(int ballotID, int RollNo);                     
void remove_vote(int ballotID, int ADMIN, int RollNo);
void remove_voter(int ballotID, int ADMIN, int RollNo);
BlockNode* checkTamper(int ballotID, int choice, pair<int,int> interval);
void insert(vector<pair<int,int>>&arr, pair<int,int> element);
vector<pair<int,int>> getTally(int ballotID);
int checkState(int ballotID, int RollNo);
void Vote(int ballotID, int RollNo, int VOTE);
VotingBlockchain* loadBallot(const filesystem::path& entry);
void loadBallots(BallotChain*& ballots);
void storeballot(BallotNode* ballot);
void storeballots();
void loadUsers(HashTable<userNode*>& users);
bool authorizeUser(int roll, string pw);
int addUser(int roll, const string& stored);    // appends "roll|salt$hash" to users.db; 0 ok, 1 exists, 2 bad format, 3 write error
void removeBallot(int ballotID);
void endBallot(int ballotID);       // ends the poll: only the statistics stay

// globals shared by main.cpp and web_main.cpp (defined in main.cpp)
extern BallotChain* ballots;
extern HashTable<userNode*>* users;
extern int superAdminRoll;          // first line of users.db: admin of all ballots
void clearVotes(int ballotID);      // removes every vote of a ballot, voters stay registered

#endif
