#ifndef HASH_MODULE_H
#define HASH_MODULE_H

#include <vector>

using namespace std;

struct BlockNode;  // forward declaration, so that the complier will know that a struct of this type exists.

class HashModule {
    private:
        static const int TABLE_SIZE = 1009; // static is used so that the same table is shared for all class instance and doesn't need extra space for each instance

        struct VoterNode {
            int roll_no;
            bool hasVoted;
            BlockNode* pointer;
            VoterNode* next;
        };

        vector<VoterNode*> table;

    public:
        HashModule();
        void registerVoter(int roll_no);
        int canVote(int roll_no);
        bool removeVoter(int roll_no);
        bool removeVote(int roll_no);
        BlockNode* searchNode(int rollNo);
};


#endif