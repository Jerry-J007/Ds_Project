#ifndef DIGEST_H
#define DIGEST_H

#include <string>

// Returns the SHA-256 digest of `input` as 64 lowercase hex characters.
std::string sha256(const std::string& input);

#endif // DIGEST_H