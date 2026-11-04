#include "../include/httplib.h"     // Single-header HTTP server (before main.h: windows headers clash with using namespace std)
#include "../include/main.h"
#include "../include/Tests.h"
#include <mutex>
#include <string>

httplib::Server server;
std::mutex mtx;                     // only 1 request touches the data at a time

static string jsonEscape(const string& s){
    string out;
    for(unsigned char c : s){
        if(c == '"') out += "\\\"";
        else if(c == '\\') out += "\\\\";
        else if(c < 0x20) out += ' ';
        else out += c;
    }
    return out;
}

static string fail(const string& message){
    return "{\"ok\":false,\"message\":\"" + jsonEscape(message) + "\"}";
}

static bool toInt(const string& s, int& value){         // positive numbers only
    if(s.empty() || s.size() > 9 || s.find_first_not_of("0123456789") != string::npos) return false;
    value = stoi(s);
    return value > 0;
}

// "1, 2,3" -> {1,2,3}; false if anything is not a positive number. Duplicates are dropped.
static bool parseList(const string& text, vector<int>& out){
    stringstream ss(text);
    string piece;
    while(getline(ss, piece, ',')){
        piece.erase(0, piece.find_first_not_of(" \t\r\n"));
        piece.erase(piece.find_last_not_of(" \t\r\n") + 1);
        if(piece.empty()) continue;
        int value;
        if(!toInt(piece, value)) return false;
        if(find(out.begin(), out.end(), value) == out.end()) out.push_back(value);
    }
    return true;
}

// "1|Alice, 2|Bob" -> numbers {1,2} and names {"Alice","Bob"}; false on a bad number, empty/duplicate name or too few entries
static bool parseCandidates(const string& text, vector<int>& numbers, vector<string>& names){
    stringstream ss(text);
    string piece;
    while(getline(ss, piece, ',')){
        size_t bar = piece.find('|');
        if(bar == string::npos) return false;
        string number = piece.substr(0, bar), name = piece.substr(bar + 1);
        for(string* part : {&number, &name}){
            part->erase(0, part->find_first_not_of(" \t\r\n"));
            part->erase(part->find_last_not_of(" \t\r\n") + 1);
        }
        int value;
        if(!toInt(number, value) || name.empty() || name.size() > 40 || name.find('|') != string::npos) return false;
        if(find(numbers.begin(), numbers.end(), value) != numbers.end()) return false;
        numbers.push_back(value);
        names.push_back(name);
    }
    return numbers.size() >= 2;
}

static string candidateName(const VotingBlockchain* chain, int number){
    for(size_t i = 0; i < chain->candidates.size(); i++){
        if(chain->candidates[i] == number) return chain->candidateNames[i];
    }
    return std::to_string(number);
}

// winner(s) and statistics of an ended poll
static string resultsJson(BallotNode* ballot){
    VotingBlockchain* chain = ballot->votes;
    vector<pair<int,int>> tally = getTally(ballot->id);         // highest first
    int top = tally.empty() ? 0 : tally[0].second;
    string json = "{\"ok\":true,\"state\":\"ended\",\"title\":\"" + jsonEscape(chain->title) + "\""
        + ",\"endedAt\":\"" + jsonEscape(chain->endedAt) + "\""
        + ",\"totalVoters\":" + std::to_string(chain->totalVoters)
        + ",\"totalVotes\":" + std::to_string(chain->totalVotes) + ",\"winners\":[";
    bool first = true;
    if(top > 0){
        for(auto& entry : tally){
            if(entry.second != top) continue;
            if(!first) json += ",";
            first = false;
            json += "\"" + jsonEscape(candidateName(chain, entry.first)) + "\"";
        }
    }
    json += "],\"results\":[";
    for(size_t i = 0; i < tally.size(); i++){
        if(i) json += ",";
        json += "{\"number\":" + std::to_string(tally[i].first) + ",\"name\":\"" + jsonEscape(candidateName(chain, tally[i].first))
            + "\",\"votes\":" + std::to_string(tally[i].second) + "}";
    }
    json += "]}";
    return json;
}

static void reply(httplib::Response& res, const string& json){
    res.set_content(json, "application/json");
}

int main() {
    if(!filesystem::exists("frontend") && filesystem::exists("../frontend")){
        filesystem::current_path("..");                 // started from Source/
    }
    loadUsers(*users);
    loadBallots(ballots);

    server.Get("/api/ballots", [&](const httplib::Request&, httplib::Response& res){
        std::lock_guard<std::mutex> lock(mtx);
        string json = "[";
        for(BallotNode* node = ballots->head; node; node = node->next){
            if(node != ballots->head) json += ",";
            json += "{\"id\":" + std::to_string(node->id)
                + ",\"title\":\"" + jsonEscape(node->votes->title) + "\""
                + ",\"admin\":" + std::to_string(node->votes->admin)
                + ",\"ended\":" + (node->votes->ended ? "true" : "false") + "}";
        }
        json += "]";
        reply(res, json);
    });

    // plain login (used before creating a ballot)
    server.Post("/api/login", [&](const httplib::Request& req, httplib::Response& res){
        std::lock_guard<std::mutex> lock(mtx);
        int roll;
        if(!toInt(req.get_param_value("roll"), roll) || !authorizeUser(roll, req.get_param_value("password"))){
            return reply(res, fail("Invalid roll number or password."));
        }
        reply(res, "{\"ok\":true}");
    });

    // login for one ballot: tells the page whether this voter can vote or already did
    server.Post(R"(/api/ballots/(\d+)/access)", [&](const httplib::Request& req, httplib::Response& res){
        std::lock_guard<std::mutex> lock(mtx);
        int roll;
        int id = stoi(req.matches[1]);
        BallotNode* ballot = ballots->hashTable.find(id);
        if(!ballot) return reply(res, fail("Ballot not found."));
        if(!toInt(req.get_param_value("roll"), roll) || !authorizeUser(roll, req.get_param_value("password"))){
            return reply(res, fail("Invalid roll number or password."));
        }
        if(ballot->votes->ended) return reply(res, resultsJson(ballot));       // ended: everybody sees the results
        if(ballot->votes->admin == roll) return reply(res, "{\"ok\":true,\"state\":\"admin\"}");
        int state = checkState(id, roll);
        if(state == 0) return reply(res, fail("You are not registered for this ballot."));
        if(state == 2){
            return reply(res, "{\"ok\":true,\"state\":\"voted\",\"candidate\":\"" + std::to_string(showVote(id, roll))
                + "\",\"name\":\"" + jsonEscape(candidateName(ballot->votes, showVote(id, roll))) + "\"}");
        }
        string json = "{\"ok\":true,\"state\":\"eligible\",\"candidates\":[";
        const vector<int>& cands = ballot->votes->candidates;
        for(size_t i = 0; i < cands.size(); i++){
            if(i) json += ",";
            json += "{\"number\":" + std::to_string(cands[i]) + ",\"name\":\"" + jsonEscape(ballot->votes->candidateNames[i]) + "\"}";
        }
        json += "]}";
        reply(res, json);
    });

    server.Post(R"(/api/ballots/(\d+)/vote)", [&](const httplib::Request& req, httplib::Response& res){
        std::lock_guard<std::mutex> lock(mtx);
        int roll, candidate;
        int id = stoi(req.matches[1]);
        BallotNode* ballot = ballots->hashTable.find(id);
        if(!ballot) return reply(res, fail("Ballot not found."));
        if(!toInt(req.get_param_value("roll"), roll) || !authorizeUser(roll, req.get_param_value("password"))){
            return reply(res, fail("Invalid roll number or password."));
        }
        if(ballot->votes->ended) return reply(res, fail("This poll has ended."));
        int state = checkState(id, roll);
        if(state == 0) return reply(res, fail("You are not registered for this ballot."));
        if(state == 2) return reply(res, fail("You have already voted."));
        const vector<int>& cands = ballot->votes->candidates;
        if(!toInt(req.get_param_value("candidate"), candidate) || find(cands.begin(), cands.end(), candidate) == cands.end()){
            return reply(res, fail("Invalid candidate."));
        }
        Vote(id, roll, candidate);
        storeballot(ballot);
        reply(res, "{\"ok\":true}");
    });

    // create a ballot; the logged in user becomes its admin
    server.Post("/api/ballots", [&](const httplib::Request& req, httplib::Response& res){
        std::lock_guard<std::mutex> lock(mtx);
        int roll;
        if(!toInt(req.get_param_value("roll"), roll) || !authorizeUser(roll, req.get_param_value("password"))){
            return reply(res, fail("Invalid roll number or password."));
        }
        string title = req.get_param_value("title");
        title.erase(0, title.find_first_not_of(" \t\r\n"));
        title.erase(title.find_last_not_of(" \t\r\n") + 1);
        if(title.empty() || title.size() > 80 || title.find_first_of("|\r\n") != string::npos){
            return reply(res, fail("Enter a title (max 80 characters, no '|')."));
        }
        vector<int> cands, voters;
        vector<string> names;
        if(!parseCandidates(req.get_param_value("candidates"), cands, names)){
            return reply(res, fail("Add at least 2 candidates, each with a unique number and a name (max 40 characters, no ',' or '|')."));
        }
        if(!parseList(req.get_param_value("voters"), voters) || voters.empty()){
            return reply(res, fail("Enter voter roll numbers, comma separated."));
        }
        ballots->createBallot(title, roll, cands, voters, names);
        storeballot(ballots->tail);
        reply(res, "{\"ok\":true,\"id\":" + std::to_string(ballots->tail->id) + "}");
    });

    // ---------- admin endpoints: every call re-checks the password and that the user is this ballot's admin ----------
    auto adminBallot = [](const httplib::Request& req, httplib::Response& res) -> BallotNode* {
        int roll;
        BallotNode* ballot = ballots->hashTable.find(stoi(req.matches[1]));
        if(!ballot){ reply(res, fail("Ballot not found.")); return nullptr; }
        if(!toInt(req.get_param_value("roll"), roll) || !authorizeUser(roll, req.get_param_value("password"))){
            reply(res, fail("Invalid roll number or password.")); return nullptr;
        }
        if(ballot->votes->admin != roll){ reply(res, fail("Only the ballot admin can do this.")); return nullptr; }
        if(ballot->votes->ended){ reply(res, fail("This poll has ended.")); return nullptr; }
        return ballot;
    };

    // everything the admin page shows: title, voters (voted or not), tally, block id range
    server.Post(R"(/api/ballots/(\d+)/admin/info)", [&](const httplib::Request& req, httplib::Response& res){
        std::lock_guard<std::mutex> lock(mtx);
        BallotNode* ballot = adminBallot(req, res);
        if(!ballot) return;
        VotingBlockchain* chain = ballot->votes;
        string json = "{\"ok\":true,\"title\":\"" + jsonEscape(chain->title) + "\",\"blocks\":" + std::to_string(chain->blockCount)
            + ",\"maxBlockId\":" + std::to_string(chain->blockCount > 0 ? (chain->blockCount - 1) / 3 : 0) + ",\"voters\":[";
        for(size_t i = 0; i < chain->resgisteredVoters.size(); i++){
            int voter = chain->resgisteredVoters[i];
            if(i) json += ",";
            json += "{\"roll\":" + std::to_string(voter) + ",\"voted\":" + (checkState(ballot->id, voter) == 2 ? "true" : "false") + "}";
        }
        json += "],\"tally\":[";
        vector<pair<int,int>> tally = getTally(ballot->id);
        for(size_t i = 0; i < tally.size(); i++){
            if(i) json += ",";
            json += "{\"candidate\":" + std::to_string(tally[i].first) + ",\"name\":\"" + jsonEscape(candidateName(chain, tally[i].first)) + "\",\"votes\":" + std::to_string(tally[i].second) + "}";
        }
        json += "]}";
        reply(res, json);
    });

    // mode=all checks the whole chain; mode=interval checks only blocks whose B+ tree id is in [start, end]
    server.Post(R"(/api/ballots/(\d+)/admin/tamper)", [&](const httplib::Request& req, httplib::Response& res){
        std::lock_guard<std::mutex> lock(mtx);
        BallotNode* ballot = adminBallot(req, res);
        if(!ballot) return;
        bool interval = req.get_param_value("mode") == "interval";
        pair<int,int> range = {0, 0};
        if(interval){
            string a = req.get_param_value("start"), b = req.get_param_value("end");
            auto number = [](const string& s, int& v){
                if(s.empty() || s.size() > 9 || s.find_first_not_of("0123456789") != string::npos) return false;
                v = stoi(s); return true;
            };
            if(!number(a, range.first) || !number(b, range.second) || range.first > range.second){
                return reply(res, fail("Enter a valid interval: start and end block ids (start <= end)."));
            }
            if(ballot->votes->database.rangeQuery(range.first, range.second).empty()){
                return reply(res, fail("No blocks in that interval."));
            }
        }
        BlockNode* bad = checkTamper(ballot->id, interval ? 1 : 0, range);
        if(!bad) return reply(res, "{\"ok\":true,\"tampered\":false}");
        reply(res, "{\"ok\":true,\"tampered\":true,\"blockId\":" + std::to_string(bad->blockID)
            + ",\"roll\":" + std::to_string(bad->roll_no) + ",\"timestamp\":\"" + jsonEscape(bad->timestamp) + "\"}");
    });

    // end the poll: only the statistics are kept (the chain is checked first, it cannot be checked afterwards)
    server.Post(R"(/api/ballots/(\d+)/admin/end)", [&](const httplib::Request& req, httplib::Response& res){
        std::lock_guard<std::mutex> lock(mtx);
        BallotNode* ballot = adminBallot(req, res);
        if(!ballot) return;
        BlockNode* bad = checkTamper(ballot->id, 0, {0, 0});
        if(bad){
            return reply(res, fail("The chain is tampered at block id " + std::to_string(bad->blockID) + " (voter "
                + std::to_string(bad->roll_no) + "). Fix this before ending the poll."));
        }
        endBallot(ballot->id);
        storeballot(ballot);
        reply(res, "{\"ok\":true}");
    });

// remove a voter completely (their vote too)
    server.Post(R"(/api/ballots/(\d+)/admin/remove_voter)", [&](const httplib::Request& req, httplib::Response& res){
        std::lock_guard<std::mutex> lock(mtx);
        BallotNode* ballot = adminBallot(req, res);
        if(!ballot) return;
        int target;
        if(!toInt(req.get_param_value("target"), target) || checkState(ballot->id, target) == 0){
            return reply(res, fail("That roll number is not registered for this ballot."));
        }
        remove_voter(ballot->id, ballot->votes->admin, target);
        storeballot(ballot);
        reply(res, "{\"ok\":true}");
    });

    // remove only the vote; the voter stays registered and can vote again
    server.Post(R"(/api/ballots/(\d+)/admin/remove_vote)", [&](const httplib::Request& req, httplib::Response& res){
        std::lock_guard<std::mutex> lock(mtx);
        BallotNode* ballot = adminBallot(req, res);
        if(!ballot) return;
        int target;
        if(!toInt(req.get_param_value("target"), target) || checkState(ballot->id, target) != 2){
            return reply(res, fail("That voter has not voted."));
        }
        remove_vote(ballot->id, ballot->votes->admin, target);
        storeballot(ballot);
        reply(res, "{\"ok\":true}");
    });

    //super admin (first user in users.db): manages every ballot
    auto superAuth = [](const httplib::Request& req, httplib::Response& res) -> bool {
        int roll;
        if(!toInt(req.get_param_value("roll"), roll) || !authorizeUser(roll, req.get_param_value("password"))
           || roll != superAdminRoll){
            reply(res, fail("Invalid admin roll number or password."));
            return false;
        }
        return true;
    };

    server.Post("/api/super/login", [&](const httplib::Request& req, httplib::Response& res){
        std::lock_guard<std::mutex> lock(mtx);
        if(!superAuth(req, res)) return;
        reply(res, "{\"ok\":true}");
    });

    server.Post("/api/super/ballots", [&](const httplib::Request& req, httplib::Response& res){
        std::lock_guard<std::mutex> lock(mtx);
        if(!superAuth(req, res)) return;
        string json = "{\"ok\":true,\"ballots\":[";
        for(BallotNode* node = ballots->head; node; node = node->next){
            if(node != ballots->head) json += ",";
            json += "{\"id\":" + std::to_string(node->id)
                + ",\"title\":\"" + jsonEscape(node->votes->title) + "\""
                + ",\"admin\":" + std::to_string(node->votes->admin)
                + ",\"voters\":" + std::to_string(node->votes->ended ? node->votes->totalVoters : (int)node->votes->resgisteredVoters.size())
                + ",\"votes\":" + std::to_string(node->votes->ended ? node->votes->totalVotes : node->votes->blockCount)
                + ",\"ended\":" + (node->votes->ended ? "true" : "false") + "}";
        }
        json += "]}";
        reply(res, json);
    });

    // add a user; "stored" is salt$hash, already computed by the browser
    server.Post("/api/super/users/add", [&](const httplib::Request& req, httplib::Response& res){
        std::lock_guard<std::mutex> lock(mtx);
        if(!superAuth(req, res)) return;
        int newRoll;
        if(!toInt(req.get_param_value("newRoll"), newRoll)) return reply(res, fail("Enter a valid roll number (positive number)."));
        switch(addUser(newRoll, req.get_param_value("stored"))){
            case 0: return reply(res, "{\"ok\":true}");
            case 1: return reply(res, fail("A user with this roll number already exists."));
            case 2: return reply(res, fail("Invalid password data."));
            default: return reply(res, fail("Could not write users.db."));
        }
    });

    // run the automated test suite and report every result (super admin only)
    server.Post("/api/super/tests", [&](const httplib::Request& req, httplib::Response& res){
        std::lock_guard<std::mutex> lock(mtx);          // tests use a temp ballot file and temp users: nobody else may interfere
        if(!superAuth(req, res)) return;
        vector<TestResult> results = runAutomatedTests();
        int passed = 0;
        string json = "{\"ok\":true,\"tests\":[";
        for(size_t i = 0; i < results.size(); i++){
            if(results[i].passed) passed++;
            if(i) json += ",";
            char ms[32];
            snprintf(ms, sizeof(ms), "%.1f", results[i].ms);
            json += "{\"name\":\"" + jsonEscape(results[i].name) + "\",\"passed\":" + (results[i].passed ? "true" : "false")
                + ",\"detail\":\"" + jsonEscape(results[i].detail) + "\",\"ms\":" + ms + "}";
        }
        json += "],\"passed\":" + std::to_string(passed) + ",\"total\":" + std::to_string(results.size()) + "}";
        reply(res, json);
    });

    // delete a ballot completely (memory and its file)
    server.Post(R"(/api/super/ballots/(\d+)/delete)", [&](const httplib::Request& req, httplib::Response& res){
        std::lock_guard<std::mutex> lock(mtx);
        if(!superAuth(req, res)) return;
        int id = stoi(req.matches[1]);
        if(!ballots->hashTable.find(id)) return reply(res, fail("Ballot not found."));
        removeBallot(id);
        reply(res, "{\"ok\":true}");
    });

    // remove every vote of a ballot; the ballot and its voters stay
    server.Post(R"(/api/super/ballots/(\d+)/clear)", [&](const httplib::Request& req, httplib::Response& res){
        std::lock_guard<std::mutex> lock(mtx);
        if(!superAuth(req, res)) return;
        BallotNode* ballot = ballots->hashTable.find(stoi(req.matches[1]));
        if(!ballot) return reply(res, fail("Ballot not found."));
        clearVotes(ballot->id);
        storeballot(ballot);
        reply(res, "{\"ok\":true}");
    });

    server.set_mount_point("/", "./frontend");          // serves index.html first
    std::cout << "Open http://localhost:8080" << std::endl;
    if(!server.listen("0.0.0.0", 8080)){
        std::cerr << "could not start the server (is port 8080 busy?)" << std::endl;
        return 1;
    }
    return 0;
}
