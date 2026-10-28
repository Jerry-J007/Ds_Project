#include "../include/BPlusTree.h"
#include "../include/HashModule.h"
#include "../include/Blockchain.h"
#include <iostream>

int main() {
    BPlusTreeDB btreeDB("votes_btree.db");
    HashModule voterAuth;
    VotingBlockchain election(btreeDB, voterAuth);

    // Pre-register NITT sample roll numbers for quick demo
    int sampleRolls[] = {106125043, 106125045, 106125047, 106125001, 106125002, 106125003};
    for (int r : sampleRolls) voterAuth.registerVoter(r);

    int choice;
    while (true) {
        std::cout << "\n================ TRANSPARENT VOTING SYSTEM ================\n"
                  << "1. Cast Vote (Hash Table Check -> Blockchain -> B+ Tree DB)\n"
                  << "2. Search Vote by Block ID (B+ Tree O(log N) Lookup)\n"
                  << "3. Range Audit Votes (B+ Tree Leaf-Chain Traversal)\n"
                  << "4. Display B+ Tree Internal & Leaf Structure\n"
                  << "5. Display Blockchain Ledger & Election Tally\n"
                  << "6. Run Full Integrity Audit (Chain + B+ Tree Cross-Check)\n"
                  << "7. [Demo] Simulate Vote Tampering on a Block\n"
                  << "0. Exit\n"
                  << "Enter choice: ";
        if (!(std::cin >> choice) || choice == 0) break;

        if (choice == 1) {
            int roll, cand;
            std::cout << "Enter Roll No (e.g., 106125047): ";
            std::cin >> roll;
            std::cout << "Enter Candidate ID (1, 2, or 3): ";
            std::cin >> cand;
            election.castVote(roll, cand);
        } else if (choice == 2) {
            int bid;
            std::cout << "Enter Block ID to search in B+ Tree: ";
            std::cin >> bid;
            VoteDBRecord* rec = btreeDB.search(bid);
            if (rec) {
                std::cout << "[FOUND in B+ Tree] Block #" << rec->blockID
                          << " | Candidate: " << rec->candidateID
                          << " | Time: " << rec->timestamp
                          << " | Hash: " << rec->blockHash << "\n";
            } else {
                std::cout << "[NOT FOUND] Block #" << bid << " does not exist.\n";
            }
        } else if (choice == 3) {
            int s, e;
            std::cout << "Enter Start Block ID and End Block ID: ";
            std::cin >> s >> e;
            auto records = btreeDB.rangeQuery(s, e);
            std::cout << "--- B+ Tree Leaf Range Results (" << records.size() << " blocks) ---\n";
            for (const auto& r : records) {
                std::cout << "Block #" << r.blockID << " -> Candidate " << r.candidateID 
                          << " [" << r.blockHash.substr(0, 10) << "...]\n";
            }
        } else if (choice == 4) {
            btreeDB.displayTreeStructure();
        } else if (choice == 5) {
            election.displayLedger();
            election.displayElectionTally();
        } else if (choice == 6) {
            election.verifyChainIntegrity();
        } else if (choice == 7) {
            int bid, fakeCand;
            std::cout << "Enter Block ID to tamper and fake Candidate ID: ";
            std::cin >> bid >> fakeCand;
            if (election.tamperBlockForDemo(bid, fakeCand)) {
                std::cout << "Block #" << bid << " modified in memory. Run Option 6 to test detection!\n";
            }
        }
    }
    return 0;
}