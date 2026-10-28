#include "../include/BPlusTree.h"
#include "../include/HashModule.h"
#include "../include/Blockchain.h"
#include"../include/httplib.h"// Single-header HTTP server
#include <iostream>
#include <string>

int main() {
    BPlusTreeDB btreeDB("votes_btree.db");
    HashModule voterAuth;
    VotingBlockchain election(btreeDB, voterAuth);

    // Pre-register sample rolls
    int sampleRolls[] = {106125043, 106125045, 106125047, 106125001};
    for (int r : sampleRolls) voterAuth.registerVoter(r);

    httplib::Server svr;

    // 1. Serve the HTML/CSS/JS frontend files
    svr.set_mount_point("/", "./frontend");

    // 2. API Endpoint: Cast a Vote
    svr.Post("/api/vote", [&](const httplib::Request& req, httplib::Response& res) {
        // Read form data from JS fetch()
        int roll = std::stoi(req.get_param_value("roll"));
        int cand = std::stoi(req.get_param_value("candidate"));

        if (!voterAuth.canVote(roll)) {
            res.set_content(R"({"status":"error", "message":"Unregistered or already voted"})", "application/json");
            return;
        }

        election.castVote(roll, cand);
        res.set_content(R"({"status":"success", "message":"Vote recorded securely!"})", "application/json");
    });

    // 3. API Endpoint: Search B+ Tree
    svr.Get("/api/search", [&](const httplib::Request& req, httplib::Response& res) {
        int blockID = std::stoi(req.get_param_value("blockID"));
        VoteDBRecord* rec = btreeDB.search(blockID);
        
        if (rec) {
            std::string json = R"({"status":"found", "candidate":)" + std::to_string(rec->candidateID) + 
                               R"(, "hash":")" + rec->blockHash + R"("})";
            res.set_content(json, "application/json");
        } else {
            res.set_content(R"({"status":"not_found"})", "application/json");
        }
    });

   std::cout << "Starting Transparent Voting System Server at http://localhost:8080\n";
    
    // Use "0.0.0.0" instead of "localhost" for better Windows compatibility
    if (!svr.listen("0.0.0.0", 8080)) {
        std::cerr << "\n[ERROR] Failed to start server! Port 8080 is likely in use.\n";
        std::cerr << "Try changing the port to 8081 in web_main.cpp\n";
        return 1;
    }

    return 0;
}