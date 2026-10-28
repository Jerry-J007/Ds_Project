#ifndef HASH_MODULE_H
#define HASH_MODULE_H

#include "../include/Blockchain.h"
#include <vector>

using namespace std;

class HashModule {
    private:
        static const int TABLE_SIZE = 1009; // static is used so that the same table is shared for all class instance and doesn't need extra space for each instance

        struct VoterNode {
            int roll_no;
            bool hasVoted;
            // add the pointer to LL field ( will be done after I finish Blcokchain part)
            VoterNode* next;
        };

        vector<VoterNode*> table;

    public:
        HashModule();
        void registerVoter(int roll_no);
        bool canVote(int roll_no);
        bool markVoted(int roll_no, BlockNode* nodePointer);
        bool removeVoter(int roll_no);
};


#endif