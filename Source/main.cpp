#include "../include/BPlusTree.h"
#include "../include/HashModule.h"
#include "../include/Blockchain.h"
#include <iostream>
#include <vector>
using namespace std;

void insert(vector<pair<int,int>>&arr, pair<int,int> element);

int main() {
    BPlusTreeDB btreeDB("votes_btree.db");
    HashModule hashTable;
    VotingBlockchain blockChain(btreeDB, hashTable);

    // Pre-register NITT sample roll numbers for quick demo
    int sampleRolls[] = {106125043, 106125045, 106125047, 106125001, 106125002, 106125003};     // get it from admin
    for (int r : sampleRolls) hashTable.registerVoter(r);

    int choice;
    while (true) {
        std::cout << "\n================ TRANSPARENT VOTING SYSTEM ================\n"
                  << "1. Cast Vote (Hash Table Check -> Blockchain -> B+ Tree DB)\n"
                  << "2. Search Vote by Roll.No O(1) Lookup)\n"
                  << "3. Range Audit Votes (B+ Tree Leaf-Chain Traversal)\n"
                  << "4. Display B+ Tree Internal & Leaf Structure\n"
                  << "5. Display Election Tally\n"
                  << "6. Run Full Integrity Audit (Chain + B+ Tree Cross-Check)\n"
                  << "7. Remove voter"
                  << "8. Ban user"
                  << "9. Show vote"
                  << "0. Exit\n"
                  << "Enter choice: ";

        if (!(std::cin >> choice)){ 
            cout<<"Enter valid a choice.";
            break;
        }

        switch(choice){
            case 1:{
                int roll, cand;
                std::cout << "Enter Roll No: ";
                std::cin >> roll;
                std::cout << "Enter Candidate ID (1, 2, or 3): ";       // make it felxible for any no.of candidates
                std::cin >> cand;
                blockChain.castVote(roll, cand);
                break;
            }
            case 2:{
                int roll_no;
                std::cout << "Enter Roll.no to search in Hashtable: ";
                std::cin >> roll_no;
                int result = hashTable.canVote(roll_no);
                if(result == 2){
                    cout<<"Has already voted";
                }
                else if(result == 1){
                    cout<<"Hasn't voted yet";
                }
                else{
                    cout<<"Not voted yet.";
                }
                break;
            }
            case 3:{
                int s, e;
                std::cout << "Enter Start Block ID and End Block ID: ";
                std::cin >> s >> e;
                auto records = btreeDB.rangeQuery(s, e);
                std::cout << "--- B+ Tree Leaf Range Results (" << records.size() << " blocks) ---\n";
                for (const auto& r : records) {
                    std::cout << "Block #" << r.blockID << " -> Candidate " << r.candidateID 
                            << " [" << r.blockHash.substr(0, 10) << "...]\n";
                }
                break;
            }
            case 4:{
                btreeDB.displayTreeStructure();
                break;
            }
            case 5:{
                unordered_map<int,int>tally = blockChain.displayElectionTally();
                int candidateCount = 3;      // need to be flexible
                vector<pair<int,int>> votes;
                votes.reserve(candidateCount);
                for( pair<int,int> vote : tally){
                    insert(votes,vote);
                }
                for(int i =0;i < candidateCount;i++){
                    cout<<i+1<<") "<<"Candidate "<<votes[i].first<<" : "<<"Votes: "<<votes[i].second;
                }
                break;
            }
            case 6:{
                int option;
                cout<<"Which section of votes do you want to verify?\n1) All the votes (Press 1)\n2) Range of votes based on timeline (Press 2):";
                cin>>option;
                switch(option){
                    case 1:{
                        BlockNode* tamperedNode = blockChain.verifyVotes(blockChain.head, blockChain.tail);
                        if(!tamperedNode){
                            cout<<"No vote is tampered";
                            break;
                        }
                        cout<<"The vote of Roll.No: "<<tamperedNode->roll_no<<" has been tampered";
                        break;
                    }
                    case 2:{
                        // Jeruson (check the B+ Tree)
                        break;
                    }
                    default:
                        cout<<"Enter a valid choice";
                }
                break;
            }
            case 7:{
                // do admin checks
                int rollNo;
                cout<<"Enter roll.no to be deleted: ";
                cin>>rollNo;
                BlockNode* node = hashTable.searchNode(rollNo);
                hashTable.removeVote(rollNo);
                blockChain.deleteNode(node);
                cout<<"Deleted successfully";
            }
            case 8:{
                // do admin checks
                int rollNo;
                cout<<"Enter roll.no to be deleted: ";
                cin>>rollNo;
                BlockNode* node = hashTable.searchNode(rollNo);
                hashTable.removeVoter(rollNo);
                blockChain.deleteNode(node);
                cout<<"Banned successfully";
            }
            case 9:{
                int rollNo;
                string pw;
                cout<<"Enter roll.no: ";
                cin>>rollNo;
                BlockNode* node = hashTable.searchNode(rollNo);
                if(!node){
                    cout<<"Voter does not exist";
                    break;
                }
                cout<<"Enter reciept: ";
                cin>>pw;
                if(pw == node->reciept){
                    cout<<"Voted for "<<node->vote;
                }
                else{
                    cout<<"Receipt does not match";
                }
                break;
            }
            default:
                cout<<"Invalid choice";
        }
    }
    return 0;
}

void insert(vector<pair<int,int>>&arr, pair<int,int> element){
    int i = arr.size()-1;
    arr.push_back(element);
    while(i >= 0){
        if(arr[i].second < arr[i+1].second) break; 
        swap(arr[i], arr[i+1]);
        i--;
    }
}