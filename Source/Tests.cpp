#include "../include/Tests.h"
#include "../include/main.h"
#include "../include/Digest.h"
#include <chrono>
#include <functional>
#include <memory>
#include <random>
#include <stdexcept>

// A failed check throws, so one broken test never stops the others (unlike assert/exit).
#define CHECK(cond) do { if (!(cond)) throw std::runtime_error(string(#cond) + " (line " + to_string(__LINE__) + ")"); } while (0)

namespace {

const int TEST_BALLOT_ID = 2000000000;      // file ballots/ballot2000000000.db, deleted after the test

string readWholeFile(const string& name){
    ifstream in(name, ios::binary);
    stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

void writeWholeFile(const string& name, const string& data){
    ofstream out(name, ios::binary);
    out << data;
}

string stripCR(const string& text){
    string out;
    for(char c : text) if(c != '\r') out += c;
    return out;
}

string toCRLF(const string& text){          // what a Windows editor or Git can do to the files
    string out;
    for(char c : stripCR(text)){
        if(c == '\n') out += '\r';
        out += c;
    }
    return out;
}

// the test voters have to be real users, loadBallot() ignores unknown rolls
struct TempUsers{
    vector<int> rolls;
    vector<userNode*> nodes;
    explicit TempUsers(int count){
        for(int roll = 900000001; (int)rolls.size() < count; roll++){
            if(users->stateCheck(roll) != 0) continue;
            userNode* node = new userNode{roll, "", nullptr};
            users->registerEntry(roll);
            users->insert(roll, node);
            rolls.push_back(roll);
            nodes.push_back(node);
        }
    }
    ~TempUsers(){
        for(size_t i = 0; i < rolls.size(); i++){
            users->remove(rolls[i]);
            delete nodes[i];
        }
    }
};

struct TempBallotFile{
    string path = "./ballots/ballot" + to_string(TEST_BALLOT_ID) + ".db";
    ~TempBallotFile(){ error_code ec; filesystem::remove(path, ec); }
};

// ---------------------------------------------------------------- the tests

void testSha256(){
    CHECK(sha256("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    CHECK(sha256("") == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

void testPasswordHash(){
    string salt = newSalt();
    CHECK(salt.size() == 32);
    string a = hashPassword("secret", salt);
    CHECK(a.size() == 64);
    CHECK(a == hashPassword("secret", salt));                       // same input -> same hash
    CHECK(a != hashPassword("Secret", salt));                       // other password -> other hash
    CHECK(a != hashPassword("secret", newSalt()));                  // other salt -> other hash
}

void testHashTableStates(){
    HashTable<BlockNode*> table;
    CHECK(table.stateCheck(42) == 0);                               // unknown
    table.registerEntry(42);
    CHECK(table.stateCheck(42) == 1);                               // registered, not voted
    BlockNode block{};
    table.insert(42, &block);
    CHECK(table.stateCheck(42) == 2 && table.find(42) == &block);   // voted
    table.insert(43, &block);                                       // unregistered key is ignored
    CHECK(table.stateCheck(43) == 0);
    CHECK(table.removeEntry(42) && table.stateCheck(42) == 1);      // vote removed, still registered
    CHECK(table.remove(42) && table.stateCheck(42) == 0);
    CHECK(!table.remove(42));
}

void testHashTableCollisions(){
    HashTable<BlockNode*> table;
    const int size = HashTable<BlockNode*>::TABLE_SIZE;
    for(int r = 1; r <= 5000; r++) table.registerEntry(r);
    for(int k = 1; k <= 3000; k++) table.registerEntry(k * size);   // all land in bucket 0
    for(int r = 1; r <= 5000; r++) CHECK(table.stateCheck(r) == 1);
    for(int k = 1; k <= 3000; k++) CHECK(table.stateCheck(k * size) == 1);
    CHECK(table.stateCheck(5001) == 0);
    table.registerEntry(77);                                        // duplicate registration must not add a second entry
    CHECK(table.remove(77) && table.stateCheck(77) == 0);
    for(int k = 1; k <= 3000; k += 2) CHECK(table.remove(k * size)); // removing from the middle of a long chain
    for(int k = 2; k <= 3000; k += 2) CHECK(table.stateCheck(k * size) == 1);
}

void testUnregisteredRejected(){
    VotingBlockchain chain("T", 1, {1, 2, 3}, {101});
    chain.castVote(999, 1);                                         // not registered
    CHECK(chain.blockCount == 0 && chain.head == nullptr);
    CHECK(chain.hashTable.stateCheck(999) == 0);
}

void testDuplicateVote(){
    VotingBlockchain chain("T", 1, {1, 2, 3}, {101, 102});
    chain.castVote(101, 2);
    CHECK(chain.blockCount == 1);
    CHECK(chain.hashTable.stateCheck(101) == 2);
    CHECK(chain.hashTable.find(101)->vote == 2);
    chain.castVote(101, 1);                                         // second vote must be refused
    CHECK(chain.blockCount == 1 && chain.hashTable.find(101)->vote == 2);
    CHECK(chain.hashTable.stateCheck(102) == 1);                    // other voter untouched
}

void testChainLinks(){
    VotingBlockchain chain("T", 1, {1, 2}, {1, 2, 3});
    for(int r = 1; r <= 3; r++) chain.castVote(r, (r % 2) + 1);
    CHECK(chain.head->prevHash == "0");
    CHECK(chain.head->next->prevHash == chain.head->currHash);
    CHECK(chain.tail->prevHash == chain.head->next->currHash);
    CHECK(chain.verifyVotes(chain.head, chain.tail) == nullptr);
}

void testBPlusTreeSplit(){
    vector<int> voters;
    for(int r = 1; r <= 10; r++) voters.push_back(r);
    VotingBlockchain chain("T", 1, {1, 2}, voters);
    for(int r = 1; r <= 10; r++) chain.castVote(r, (r % 2) + 1);   // block ids 0,0,0,1,1,1,2,2,2,3
    CHECK(chain.blockCount == 10);
    CHECK(chain.database.search(0) != nullptr && chain.database.search(3) != nullptr);
    CHECK(chain.database.search(4) == nullptr);
    CHECK(chain.database.search(1)->blockID == 1);
    CHECK(chain.database.rangeQuery(1, 2).size() == 6);
    CHECK(chain.database.rangeQuery(0, 3).size() == 10);
    CHECK(chain.database.rangeQuery(3, 1).empty());                 // reversed interval
}

void testBPlusTreeRandomOrder(){
    BPlusTreeDB tree;
    vector<unique_ptr<BlockNode>> nodes;
    vector<int> keys;
    for(int i = 1; i <= 500; i++) keys.push_back(i);
    mt19937 rng(42);
    shuffle(keys.begin(), keys.end(), rng);
    for(int k : keys){
        nodes.emplace_back(new BlockNode{});
        nodes.back()->blockID = k;
        tree.insert(k, nodes.back().get());
    }
    for(int k = 1; k <= 500; k++) CHECK(tree.search(k) != nullptr && tree.search(k)->blockID == k);
    CHECK(tree.search(0) == nullptr && tree.search(501) == nullptr);
    vector<BlockNode*> range = tree.rangeQuery(100, 120);
    CHECK(range.size() == 21);
    for(size_t i = 0; i < range.size(); i++) CHECK(range[i]->blockID == 100 + (int)i);   // leaves are sorted
    tree.clear();
    CHECK(tree.search(1) == nullptr);
}

void testTamperDetection(){
    VotingBlockchain chain("T", 1, {1, 2, 3}, {1, 2, 3, 4, 5, 6, 7});
    for(int r = 1; r <= 7; r++) chain.castVote(r, 1);
    CHECK(chain.verifyVotes(chain.head, chain.tail) == nullptr);
    BlockNode* victim = chain.hashTable.find(4);
    victim->vote = 3;                                               // flip a vote in memory
    CHECK(chain.verifyVotes(chain.head, chain.tail) == victim);     // whole chain finds exactly that block
    BlockNode* first = chain.database.rangeQuery(0, 0).front();     // block ids 0 = voters 1..3: still clean
    BlockNode* last = chain.database.rangeQuery(0, 0).back();
    CHECK(chain.verifyVotes(first, last) == nullptr);
    vector<BlockNode*> range = chain.database.rangeQuery(1, 2);     // voters 4..7 contain the victim
    CHECK(chain.verifyVotes(range.front(), range.back()) == victim);
    victim->vote = 1;                                               // undo -> clean again
    CHECK(chain.verifyVotes(chain.head, chain.tail) == nullptr);
    chain.head->currHash[0] = chain.head->currHash[0] == 'a' ? 'b' : 'a';   // forged hash is caught too
    CHECK(chain.verifyVotes(chain.head, chain.tail) == chain.head);
}

void testRemoveVote(){
    VotingBlockchain chain("T", 1, {1, 2}, {1, 2, 3, 4, 5});
    for(int r = 1; r <= 5; r++) chain.castVote(r, (r % 2) + 1);
    for(int target : {3, 1, 5}){                                    // middle, first, last
        BlockNode* node = chain.hashTable.find(target);
        chain.hashTable.removeEntry(target);
        chain.deleteNode(node);
        CHECK(node == nullptr);
        CHECK(chain.hashTable.stateCheck(target) == 1);             // registered again, may re-vote
        CHECK(chain.verifyVotes(chain.head, chain.tail) == nullptr); // chain was re-hashed and is still valid
    }
    CHECK(chain.blockCount == 2);
    CHECK(chain.database.rangeQuery(0, 10).size() == 2);            // index rebuilt
    chain.castVote(3, 2);                                           // re-vote works and keeps the chain valid
    CHECK(chain.blockCount == 3 && chain.verifyVotes(chain.head, chain.tail) == nullptr);
}

void testEndPoll(){
    VotingBlockchain chain("T", 1, {1, 2, 3}, {1, 2, 3, 4});
    chain.castVote(1, 2);
    chain.castVote(2, 2);
    chain.castVote(3, 3);
    chain.endPoll();
    CHECK(chain.ended && chain.head == nullptr && chain.blockCount == 0);
    CHECK(chain.totalVoters == 4 && chain.totalVotes == 3);
    CHECK(chain.finalVotes == vector<int>({0, 2, 1}));
    CHECK(chain.database.search(0) == nullptr);                     // blocks are gone
    chain.endPoll();                                                // a second call changes nothing
    CHECK(chain.totalVotes == 3);
}

void checkReloaded(VotingBlockchain& chain, const vector<int>& rolls){
    CHECK(chain.blockCount == 3);
    CHECK(chain.hashTable.stateCheck(rolls[0]) == 2 && chain.hashTable.stateCheck(rolls[1]) == 2);
    CHECK(chain.hashTable.stateCheck(rolls[2]) == 2);
    CHECK(chain.hashTable.stateCheck(rolls[3]) == 1);               // untouched voter is still eligible
    chain.castVote(rolls[0], 3);                                    // double vote refused after reload
    CHECK(chain.blockCount == 3);
    CHECK(chain.database.search(0) != nullptr);
}

void testPersistence(){
    TempUsers temp(4);
    TempBallotFile file;
    const vector<int>& rolls = temp.rolls;
    {
        VotingBlockchain chain("Persist test", rolls[0], {1, 2, 3}, rolls, {"A", "B", "C"});
        chain.castVote(rolls[0], 2);
        chain.castVote(rolls[1], 1);
        chain.castVote(rolls[2], 2);
        BallotNode node{TEST_BALLOT_ID, &chain, nullptr};
        storeballot(&node);
    }                                                               // chain destroyed = program closed
    CHECK(filesystem::exists(file.path));
    {
        unique_ptr<VotingBlockchain> loaded(loadBallot(file.path));
        CHECK(loaded->title == "Persist test" && loaded->admin == rolls[0]);
        CHECK(loaded->candidateNames == vector<string>({"A", "B", "C"}));
        CHECK(loaded->verifyVotes(loaded->head, loaded->tail) == nullptr);
        checkReloaded(*loaded, rolls);
    }
    string original = stripCR(readWholeFile(file.path));
    writeWholeFile(file.path, toCRLF(original));                    // Windows line endings must not look like tampering
    {
        unique_ptr<VotingBlockchain> loaded(loadBallot(file.path));
        CHECK(loaded->verifyVotes(loaded->head, loaded->tail) == nullptr);
        checkReloaded(*loaded, rolls);
    }
}

void testEditedFileDetected(){
    TempUsers temp(3);
    TempBallotFile file;
    {
        VotingBlockchain chain("Edit test", temp.rolls[0], {1, 2}, temp.rolls);
        for(int roll : temp.rolls) chain.castVote(roll, 1);
        BallotNode node{TEST_BALLOT_ID, &chain, nullptr};
        storeballot(&node);
    }
    string text = stripCR(readWholeFile(file.path));
    size_t line3 = text.find('\n', text.find('\n') + 1) + 1;        // first block row: id|roll|vote|time|prev|curr
    size_t voteCol = text.find('|', text.find('|', line3) + 1) + 1;
    CHECK(text[voteCol] == '1');
    text[voteCol] = '2';                                            // flip the first vote by hand
    writeWholeFile(file.path, text);
    unique_ptr<VotingBlockchain> loaded(loadBallot(file.path));
    BlockNode* bad = loaded->verifyVotes(loaded->head, loaded->tail);
    CHECK(bad != nullptr && bad == loaded->head && bad->roll_no == temp.rolls[0]);
}

void testEndedFile(){
    TempUsers temp(3);
    TempBallotFile file;
    {
        VotingBlockchain chain("Ended test", temp.rolls[0], {1, 2}, temp.rolls, {"X", "Y"});
        chain.castVote(temp.rolls[0], 2);
        chain.castVote(temp.rolls[1], 2);
        chain.endPoll();
        BallotNode node{TEST_BALLOT_ID, &chain, nullptr};
        storeballot(&node);
    }
    unique_ptr<VotingBlockchain> loaded(loadBallot(file.path));
    CHECK(loaded->ended && loaded->totalVoters == 3 && loaded->totalVotes == 2);
    CHECK(loaded->finalVotes == vector<int>({0, 2}));
    CHECK(loaded->candidateNames == vector<string>({"X", "Y"}));
}

struct TestCase{
    const char* name;
    function<void()> run;
};

} // namespace

vector<TestResult> runAutomatedTests(){
    const vector<TestCase> tests = {
        {"SHA-256 known vectors ('abc' and empty string)", testSha256},
        {"Password hashing: deterministic, salted, password-sensitive", testPasswordHash},
        {"Hash table states: unknown / can vote / voted, remove", testHashTableStates},
        {"Hash table keeps 8000 voters, incl. 3000 colliding rolls", testHashTableCollisions},
        {"Unregistered voter cannot vote", testUnregisteredRejected},
        {"O(1) duplicate vote prevention", testDuplicateVote},
        {"Blocks are linked by hash (genesis prevHash = 0)", testChainLinks},
        {"B+ tree leaf/internal splits and range queries", testBPlusTreeSplit},
        {"B+ tree correct under random insertion order (500 keys)", testBPlusTreeRandomOrder},
        {"Tamper detection: whole chain, interval and forged hash", testTamperDetection},
        {"Removing a vote re-hashes the chain and lets the voter re-vote", testRemoveVote},
        {"Ending a poll keeps only the statistics", testEndPoll},
        {"Ballot survives save/reload, incl. CRLF files, no double voting", testPersistence},
        {"Hand-edited ballot file is detected after reload", testEditedFileDetected},
        {"Ended ballot saves and reloads its statistics", testEndedFile},
    };

    vector<TestResult> results;
    for(const TestCase& test : tests){
        TestResult result{test.name, true, "", 0};
        auto start = chrono::steady_clock::now();
        try{
            test.run();
        }
        catch(const exception& e){
            result.passed = false;
            result.detail = e.what();
        }
        catch(...){
            result.passed = false;
            result.detail = "unknown error";
        }
        result.ms = chrono::duration<double, milli>(chrono::steady_clock::now() - start).count();
        results.push_back(result);
    }
    return results;
}
