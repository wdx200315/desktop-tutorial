#pragma once
#include "common.h"
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <vector>

class CryptoUtils {
public:
    static std::string sha256(const std::string& input);
    static std::string aesEncrypt(const std::string& plain, const std::string& key, const std::string& iv);
    static std::string aesDecrypt(const std::string& cipher, const std::string& key, const std::string& iv);
    static std::string base64Encode(const unsigned char* data, size_t len);
    static std::string base64Decode(const std::string& input);
};
