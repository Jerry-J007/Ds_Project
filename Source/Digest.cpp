#include "../include/Digest.h"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>

std::string sha256(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];          // 32 raw bytes

    SHA256(reinterpret_cast<const unsigned char*>(input.data()),
           input.size(),
           hash);

    std::ostringstream out;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        out << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(hash[i]);
    }
    return out.str();                                  // 64 hex chars
}