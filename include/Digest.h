#ifndef DIGEST_H
#define DIGEST_H

#include <string>

// Returns the SHA-256 digest of `input` as 64 lowercase hex characters.
std::string sha256(const std::string& input);

// 16 random bytes as 32 hex characters, used as a per-user password salt.
std::string newSalt();

// PBKDF2-HMAC-SHA256 (100000 rounds) of `password` with the hex `saltHex`, as 64 lowercase hex characters.
std::string hashPassword(const std::string& password, const std::string& saltHex);

#endif // DIGEST_H