#include "../include/main.h"
#include "../include/Digest.h"

BallotChain* ballots = new BallotChain;
HashTable<userNode*>* users = new HashTable<userNode*>;
int superAdminRoll = 0;

static void chomp(string& s){               // files written on Windows end lines with \r\n
    while(!s.empty() && (s.back() == '\r' || s.back() == '\n')) s.pop_back();
}

int showVote(int ballotID, int RollNo){
    BallotNode* ballot = ballots->hashTable.find(ballotID);
    if(!ballot) return -1;
    BlockNode* node = ballot->votes->hashTable.find(RollNo);
    if(!node) return -1;
    return node->vote;
}

void remove_vote(int ballotID, int ADMIN, int RollNo){
    BallotNode* ballot = ballots->hashTable.find(ballotID);
    if(!ballot || ballot->votes->admin != ADMIN) return;
    BlockNode* node = ballot->votes->hashTable.find(RollNo);
    if(!node) return;
    ballot->votes->hashTable.removeEntry(RollNo);
    ballot->votes->deleteNode(node);
}

void remove_voter(int ballotID, int ADMIN, int RollNo){
    BallotNode* ballot = ballots->hashTable.find(ballotID);
    if(!ballot || ballot->votes->admin != ADMIN) return;
    BlockNode* node = ballot->votes->hashTable.find(RollNo);
    ballot->votes->hashTable.removeEntry(RollNo);
    ballot->votes->deleteNode(node);        // no-op when the voter never voted
    ballot->votes->hashTable.remove(RollNo);
    vector<int>& reg = ballot->votes->resgisteredVoters;
    reg.erase(remove(reg.begin(), reg.end(), RollNo), reg.end());
}

BlockNode* checkTamper(int ballotID, int choice, pair<int,int> interval){        // choice: 0-entire, 1 - interval
    BallotNode* ballot = ballots->hashTable.find(ballotID);
    if(!ballot) return nullptr;
    if(!choice){
        return ballot->votes->verifyVotes(ballot->votes->head, ballot->votes->tail);
    }
    vector<BlockNode*> blocks = ballot->votes->database.rangeQuery(interval.first, interval.second);
    if(blocks.empty()) return nullptr;
    return ballot->votes->verifyVotes(blocks.front(), blocks.back());
}

void insert(vector<pair<int,int>>&arr, pair<int,int> element){      // keeps arr sorted by count, highest first
    arr.push_back(element);
    int i = arr.size()-1;
    while(i > 0 && arr[i-1].second < arr[i].second){
        swap(arr[i-1], arr[i]);
        i--;
    }
}

vector<pair<int,int>> getTally(int ballotID){
    vector<pair<int,int>> votes;
    BallotNode* ballot = ballots->hashTable.find(ballotID);
    if(!ballot) return votes;
    if(ballot->votes->ended){               // the chain is gone, use the saved statistics
        for(size_t i = 0; i < ballot->votes->candidates.size(); i++){
            insert(votes, {ballot->votes->candidates[i], ballot->votes->finalVotes[i]});
        }
        return votes;
    }
    unordered_map<int,int> tally = ballot->votes->displayElectionTally();
    for(int candidate : ballot->votes->candidates){         // candidates nobody voted for count 0
        insert(votes, {candidate, tally.count(candidate) ? tally[candidate] : 0});
    }
    return votes;
}

int checkState(int ballotID, int RollNo){       // 0 - not registered, 1 - can vote, 2 - voted
    BallotNode* ballot = ballots->hashTable.find(ballotID);
    if(!ballot) return 0;
    return ballot->votes->hashTable.stateCheck(RollNo);
}

void Vote(int ballotID, int RollNo, int VOTE){
    BallotNode* ballot = ballots->hashTable.find(ballotID);
    if(!ballot) return;
    ballot->votes->castVote(RollNo, VOTE);
}

// ended ballot file:           (format of ballot.db after admin ends the poll)
//   #ENDED
//   title|adminRoll|endedAt|registeredVoters|votesCast
//   candidateNumber|name|votes        (one line per candidate)


static VotingBlockchain* loadEndedBallot(ifstream& in){
    string line, piece, title, endedAt;
    int admin, voters, votes;
    getline(in, line);
    chomp(line);
    stringstream head(line);
    getline(head, title, '|');
    getline(head, piece, '|');
    admin = stoi(piece);
    getline(head, endedAt, '|');
    getline(head, piece, '|');
    voters = stoi(piece);
    getline(head, piece, '|');
    votes = stoi(piece);

    vector<int> candidates, counts;
    vector<string> names;
    while(getline(in, line)){
        chomp(line);
        if(line.empty()) continue;
        stringstream ss(line);
        getline(ss, piece, '|');
        candidates.push_back(stoi(piece));
        getline(ss, piece, '|');
        names.push_back(piece);
        getline(ss, piece, '|');
        counts.push_back(stoi(piece));
    }
    VotingBlockchain* chain = new VotingBlockchain(title, admin, candidates, {}, names);
    chain->ended = true;
    chain->endedAt = endedAt;
    chain->totalVoters = voters;
    chain->totalVotes = votes;
    chain->finalVotes = counts;
    return chain;
}

VotingBlockchain* loadBallot(const filesystem::path& entry){
    ifstream in (entry.string());
    string line;
    string piece;

    getline(in, line);                  // a running ballot's first line always contains '|', so "#ENDED" is unambiguous
    chomp(line);
    if(line == "#ENDED") return loadEndedBallot(in);
    in.clear();
    in.seekg(0);

    // first line
    string title;
    int adminRoll;
    vector<int> candidates;
    vector<string> names;
    getline(in, title, '|');
    getline(in, piece, '|');
    adminRoll = stoi(piece);

    if(getline(in, line)){
        chomp(line);
        stringstream ss(line);
        while(getline(ss, piece, ',')){
            if(piece.empty()) continue;
            size_t bar = piece.find('|');                       // "number|name"; 
            candidates.push_back(stoi(piece.substr(0, bar)));
            names.push_back(bar == string::npos ? piece : piece.substr(bar + 1));
        }
    }
    vector<int> arr;
    VotingBlockchain* voteChain = new VotingBlockchain(title,adminRoll, candidates,arr, names);
    try{

    //second line
    if(getline(in, line)){
        chomp(line);
        stringstream ss(line);
        while(getline(ss, piece, ',')){
            if(piece.empty()) continue;
            int roll = stoi(piece);
            if(roll < 0) continue;
            if(users->stateCheck(roll) ==0) continue;
            if(voteChain->hashTable.stateCheck(roll) != 0) continue;     // duplicate
            voteChain->hashTable.registerEntry(roll);
            voteChain->resgisteredVoters.push_back(roll);
        }
    }

    // lines below line 2
    while(getline(in, line)){
        chomp(line);
        if(line.empty()) continue;
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
    }
    catch(...){
        delete voteChain;               // half read file: don't leak the chain, let the caller skip it
        throw;
    }
    return voteChain;
}

void loadBallots(BallotChain*& ballots){
    filesystem::create_directories("ballots");
    vector<pair<int, filesystem::path>> files;          // ballotN.db -> (N, path)
    for(auto& entry : filesystem::directory_iterator("ballots")){
        string name = entry.path().filename().string();
        if(entry.path().extension() != ".db" || name.rfind("ballot", 0) != 0) continue;
        string number = entry.path().stem().string().substr(6);
        if(number.empty() || number.find_first_not_of("0123456789") != string::npos) continue;
        files.push_back({stoi(number), entry.path()});
    }
    sort(files.begin(), files.end());
    for(auto& file : files){
        try{
            ballots->retrieveChain(loadBallot(file.second), file.first);
        }
        catch(const exception&){
            cerr << "skipping unreadable ballot file " << file.second << endl;
        }
    }
}

void storeballot(BallotNode* ballot){
    filesystem::create_directories("ballots");
    string filename = "./ballots/ballot" + to_string(ballot->id) + ".db";
    ofstream out(filename, ios::trunc);
    VotingBlockchain* chain = ballot->votes;

    if(chain->ended){                   // only the statistics are stored
        out<<"#ENDED\n";
        out<<chain->title<<'|'<<chain->admin<<'|'<<chain->endedAt<<'|'<<chain->totalVoters<<'|'<<chain->totalVotes<<"\n";
        for(size_t i = 0;i< chain->candidates.size();i++){
            out<<chain->candidates[i]<<'|'<<chain->candidateNames[i]<<'|'<<chain->finalVotes[i]<<"\n";
        }
        return;
    }

    //line 1
    out<<chain->title<<'|'<<chain->admin<<'|';
    for(size_t i = 0;i< chain->candidates.size();i++){
        if(i) out<<',';
        out<<chain->candidates[i]<<'|'<<chain->candidateNames[i];
    }
    out<<"\n";
    //line 2
    for(size_t i = 0;i< chain->resgisteredVoters.size();i++){
        if(i) out<<',';
        out<<chain->resgisteredVoters[i];
    }
    out<<"\n";
    //line 3 onwards
    BlockNode* node = chain->head;
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

// stored password format: "<32 hex salt>$<64 hex PBKDF2-SHA256 hash>"
static bool isHashed(const string& stored){
    if(stored.size() != 32 + 1 + 64 || stored[32] != '$') return false;
    return stored.find_first_not_of("0123456789abcdef$") == string::npos;
}

static string makeStored(const string& password){
    string salt = newSalt();
    return salt + "$" + hashPassword(password, salt);
}

void loadUsers(HashTable<userNode*>& users){
    ifstream in("users.db");
    string line;
    string piece;
    vector<string> lines;               // file content, rewritten if plain passwords had to be hashed
    bool converted = false;
    while(getline(in, line)){
        chomp(line);
        lines.push_back(line);
        if(line.empty()) continue;
        stringstream ss(line);
        getline(ss, piece, '|');
        int roll;
        try{ roll = stoi(piece); }
        catch(const exception&){ continue; }
        if(roll <= 0) continue;
        getline(ss, piece);
        if(!isHashed(piece)){           // old plain text entry: hash it now
            piece = makeStored(piece);
            lines.back() = to_string(roll) + "|" + piece;
            converted = true;
        }
        users.registerEntry(roll);
        userNode* node = new userNode;
        node->roll = roll;
        node->pw = piece;
        node->next = nullptr;
        users.insert(roll, node);
        if(superAdminRoll == 0) superAdminRoll = roll;      // first line is the admin of all ballots
    }
    in.close();
    if(converted){
        ofstream out("users.db.tmp", ios::trunc);
        for(const string& entry : lines) out<<entry<<"\n";
        out.close();
        if(!out) return;                                    // keep the old file if writing failed
        error_code ec;
        filesystem::rename("users.db.tmp", "users.db", ec);
    }
}

// 0 - added, 1 - roll already exists, 2 - bad password format, 3 - could not write users.db
// `stored` is already "<salt>$<hash>": the browser hashes the password, the server never sees it in plain text
int addUser(int roll, const string& stored){
    if(roll <= 0 || !isHashed(stored)) return 2;
    if(users->stateCheck(roll) != 0) return 1;

    bool needsNewline = false;                          // the last line of the file may not end with '\n'
    {
        ifstream in("users.db", ios::binary | ios::ate);
        if(in && in.tellg() > 0){
            in.seekg(-1, ios::end);
            char last;
            in.get(last);
            needsNewline = (last != '\n');
        }
    }
    ofstream out("users.db", ios::app | ios::binary);
    if(!out) return 3;
    if(needsNewline) out<<"\n";
    out<<roll<<"|"<<stored<<"\n";
    out.close();
    if(!out) return 3;

    userNode* node = new userNode;
    node->roll = roll;
    node->pw = stored;
    node->next = nullptr;
    users->registerEntry(roll);
    users->insert(roll, node);
    if(superAdminRoll == 0) superAdminRoll = roll;
    return 0;
}

bool authorizeUser(int roll, string pw){            // login check
    if(roll <= 0) return false;
    userNode* node = users->find(roll);
    if(!node || !isHashed(node->pw)) return false;
    string expected = node->pw.substr(33);
    string given = hashPassword(pw, node->pw.substr(0, 32));
    unsigned char diff = 0;             // compare every character, no early exit
    for(size_t i = 0; i < expected.size(); i++) diff |= expected[i] ^ given[i];
    return diff == 0;
}

void clearVotes(int ballotID){
    BallotNode* ballot = ballots->hashTable.find(ballotID);
    if(!ballot) return;
    vector<int> voters = ballot->votes->resgisteredVoters;
    for(int roll : voters){
        if(checkState(ballotID, roll) == 2) remove_vote(ballotID, ballot->votes->admin, roll);
    }
}

void endBallot(int ballotID){
    BallotNode* ballot = ballots->hashTable.find(ballotID);
    if(ballot) ballot->votes->endPoll();
}

void removeBallot(int ballotID){
    BallotNode* ballot = ballots->hashTable.find(ballotID);
    if(!ballot) return;
    ballots->hashTable.remove(ballotID);
    ballots->deleteballot(ballot);
    filesystem::remove("./ballots/ballot" + to_string(ballotID) + ".db");
}
