#include "../include/Digest.h"
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <sstream>
#include <iomanip>
#include <stdexcept>

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

static std::string toHex(const unsigned char* bytes, size_t length) {
    std::ostringstream out;
    for (size_t i = 0; i < length; ++i) {
        out << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[i]);
    }
    return out.str();
}

std::string newSalt() {
    unsigned char salt[16];
    if (RAND_bytes(salt, sizeof(salt)) != 1) throw std::runtime_error("could not generate a salt");
    return toHex(salt, sizeof(salt));
}

std::string hashPassword(const std::string& password, const std::string& saltHex) {
    unsigned char hash[32];
    if (PKCS5_PBKDF2_HMAC(password.data(), static_cast<int>(password.size()),
                          reinterpret_cast<const unsigned char*>(saltHex.data()), static_cast<int>(saltHex.size()),
                          100000, EVP_sha256(), sizeof(hash), hash) != 1) {
        throw std::runtime_error("password hashing failed");
    }
    return toHex(hash, sizeof(hash));
}
