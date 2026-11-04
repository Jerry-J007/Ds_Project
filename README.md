# Blockchain Voting System

A web-based voting system written in C++17. Every vote is stored as a block in a hash-linked chain (SHA-256), so any later change to a vote can be detected. The project is built around core data structures: linked lists, a hash table and a B+ tree.

## Features

- **Voters:** log in with a roll number and password, then vote once in a ballot they are registered for. A voter who has already voted sees who they voted for.
- **Ballot creation:** any user can create a ballot (title, numbered candidates with names, list of voter roll numbers) and becomes its admin.
- **Ballot admin:**
  - see the voters (voted or not) and the live tally;
  - check the chain for tampering, either the whole chain or only blocks in a block-id interval;
  - remove a voter completely, or remove only their vote so they can vote again;
  - end the poll. The chain is verified first; once ended, only the statistics (winner(s), totals, per-candidate votes) are kept and everybody can see the results.
- **Super admin** (the first user in `users.db`):
  - list all ballots, clear the votes of a ballot, or delete a ballot;
  - add new users (the password is hashed in the browser);
  - run the automated test suite and see every result.

## Data structures

| Structure | Where | Used for |
|---|---|---|
| Linked list of blocks | `VotingBlockchain` (`Blockchain.cpp`) | The chain of votes. Each block holds `blockID`, roll number, vote, timestamp, previous hash and its own hash |
| Linked list of ballots | `BallotChain` (`Ballot.cpp`) | All ballots |
| Chained hash table (template) | `HashTable.tpp` | O(1) lookup of ballots, users, and voter state per ballot (0 = not registered, 1 = can vote, 2 = voted) |
| B+ tree | `BPlusTree.cpp` | Index of blocks by block id, for range queries in the interval tamper check |

Block ids are `index / 3`, so three consecutive votes share a block id. The hash of a block is `SHA256(blockID|roll|vote|timestamp|prevHash)`. Removing a vote re-hashes the rest of the chain.

## Project layout

```
Source/web_main.cpp    HTTP server and all API endpoints
Source/main.cpp        Globals, voting helpers, loading/saving ballots and users
Source/Blockchain.cpp  Vote chain, hashing, tamper verification
Source/Ballot.cpp      List of ballots
Source/BPlusTree.cpp   B+ tree index
Source/Digest.cpp      SHA-256 and PBKDF2 password hashing (OpenSSL)
Source/Tests.cpp       Automated test suite
include/               Headers (httplib.h is the single-header HTTP library)
frontend/              HTML/JS pages
ballots/               One file per ballot: ballotN.db
users.db               One line per user: roll|salt$hash
```

## Storage

`users.db`: one line per user, `roll|<32 hex salt>$<64 hex PBKDF2-SHA256 hash>`. The first line is the super admin. Plain text passwords found in this file are hashed automatically on startup.

`ballots/ballotN.db` (running poll):

```
title|adminRoll|cand1|name1,cand2|name2,...
roll,roll,roll                                  (registered voters)
blockID|roll|vote|timestamp|prevHash|currHash   (one line per vote)
```

`ballots/ballotN.db` (ended poll):

```
#ENDED
title|adminRoll|endedAt|registeredVoters|votesCast
candidateNumber|name|votes                      (one line per candidate)
```

## Security

- Passwords: random salt + PBKDF2-SHA256 (100,000 rounds), compared in constant time.
- Every API call sends the roll number and password and is re-checked on the server; admin calls also check that the user is the admin of that ballot.
- One mutex means only one request touches the data at a time.
- User text is inserted into pages with `textContent` and escaped in JSON.

## Build and run

Requires g++ (C++17) and OpenSSL. Windows, from the project folder:

```powershell
g++ -std=c++17 -Wall -D_WIN32_WINNT=0x0A00 Source/web_main.cpp Source/main.cpp Source/Ballot.cpp Source/Blockchain.cpp Source/BPlusTree.cpp Source/Digest.cpp Source/Tests.cpp -o web_server -lcrypto -lws2_32 -lcrypt32
.\web_server.exe
```

(or `make run` if `make` is available). Then open http://localhost:8080. Start the server from the project folder, because it uses `frontend`, `ballots` and `users.db` relative to it. Use `localhost` rather than a LAN address, as the browser only allows the Web Crypto API (used to hash passwords when adding a user) on `localhost` or HTTPS.

## API

| Endpoint | Who | Purpose |
|---|---|---|
| `GET /api/ballots` | anyone | List ballots |
| `POST /api/login` | user | Check credentials |
| `POST /api/ballots` | user | Create a ballot |
| `POST /api/ballots/:id/access` | user | Voter state: eligible, voted, admin or ended |
| `POST /api/ballots/:id/vote` | voter | Cast a vote |
| `POST /api/ballots/:id/admin/info` | ballot admin | Voters, tally, block id range |
| `POST /api/ballots/:id/admin/tamper` | ballot admin | Tamper check (whole chain or interval) |
| `POST /api/ballots/:id/admin/end` | ballot admin | End the poll |
| `POST /api/ballots/:id/admin/remove_voter` | ballot admin | Remove a voter and their vote |
| `POST /api/ballots/:id/admin/remove_vote` | ballot admin | Remove only the vote |
| `POST /api/super/login` | super admin | Check super admin credentials |
| `POST /api/super/ballots` | super admin | List ballots with statistics |
| `POST /api/super/ballots/:id/clear` | super admin | Remove all votes of a ballot |
| `POST /api/super/ballots/:id/delete` | super admin | Delete a ballot |
| `POST /api/super/users/add` | super admin | Add a user (`newRoll`, `stored` = `salt$hash`) |
| `POST /api/super/tests` | super admin | Run the automated tests |
