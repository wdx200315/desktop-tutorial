/**
 * @file crypto.cpp
 * @brief AES加密实现
 */

#include "smartsched/server/crypto.h"
#include "../../common/include/smartsched/utils/logger.h"
#include <cstring>
#include <sstream>
#include <iomanip>

namespace smartsched {
namespace server {

// =============================================================================
// 工具函数
// =============================================================================

std::string AesEncryptor::sha256(const unsigned char* data, size_t len) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(data, len, hash);
    return bytesToHex(hash, SHA256_DIGEST_LENGTH);
}

std::vector<unsigned char> AesEncryptor::hexToBytes(const std::string& hex) {
    std::vector<unsigned char> result;
    result.reserve(hex.length() / 2);
    
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byte_str = hex.substr(i, 2);
        unsigned char byte = static_cast<unsigned char>(std::stoi(byte_str, nullptr, 16));
        result.push_back(byte);
    }
    
    return result;
}

std::string AesEncryptor::bytesToHex(const unsigned char* data, size_t len) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    
    for (size_t i = 0; i < len; ++i) {
        oss << std::setw(2) << static_cast<int>(data[i]);
    }
    
    return oss.str();
}

bool AesEncryptor::generateRandomKey(std::vector<unsigned char>& key, int length) {
    key.resize(length);
    return RAND_bytes(key.data(), length) == 1;
}

std::vector<unsigned char> AesEncryptor::addPKCS7Padding(const unsigned char* data, size_t len) {
    size_t padding_len = AES_BLOCK_SIZE - (len % AES_BLOCK_SIZE);
    std::vector<unsigned char> result(data, data + len);
    result.insert(result.end(), padding_len, static_cast<unsigned char>(padding_len));
    return result;
}

bool AesEncryptor::removePKCS7Padding(const unsigned char* data, size_t len, size_t& out_len) {
    if (len == 0 || len % AES_BLOCK_SIZE != 0) {
        return false;
    }
    
    unsigned char padding_len = data[len - 1];
    
    if (padding_len == 0 || padding_len > AES_BLOCK_SIZE) {
        return false;
    }
    
    // 验证padding
    for (size_t i = 0; i < padding_len; ++i) {
        if (data[len - 1 - i] != padding_len) {
            return false;
        }
    }
    
    out_len = len - padding_len;
    return true;
}

CryptoResult AesEncryptor::encryptInternal(const unsigned char* plaintext, size_t plain_len) {
    if (key_.empty()) {
        return CryptoResult(false, {}, "Key not initialized");
    }
    
    clearError();
    
    // 生成IV
    unsigned char iv[AES_IV_SIZE];
    if (RAND_bytes(iv, AES_IV_SIZE) != 1) {
        last_error_ = "Failed to generate IV";
        return CryptoResult(false, {}, last_error_);
    }
    
    // 添加PKCS7填充
    auto padded = addPKCS7Padding(plaintext, plain_len);
    
    // 加密
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        last_error_ = "Failed to create cipher context";
        return CryptoResult(false, {}, last_error_);
    }
    
    int len = 0;
    int cipher_len = 0;
    size_t max_cipher_len = padded.size() + AES_BLOCK_SIZE;
    std::vector<unsigned char> ciphertext(max_cipher_len);
    
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key_.data(), iv) != 1) {
        last_error_ = "Failed to initialize encryption";
        EVP_CIPHER_CTX_free(ctx);
        return CryptoResult(false, {}, last_error_);
    }
    
    if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len, padded.data(), static_cast<int>(padded.size())) != 1) {
        last_error_ = "Encryption update failed";
        EVP_CIPHER_CTX_free(ctx);
        return CryptoResult(false, {}, last_error_);
    }
    cipher_len = len;
    
    if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) != 1) {
        last_error_ = "Encryption final failed";
        EVP_CIPHER_CTX_free(ctx);
        return CryptoResult(false, {}, last_error_);
    }
    cipher_len += len;
    
    EVP_CIPHER_CTX_free(ctx);
    
    // 组合IV + 密文
    std::vector<unsigned char> result;
    result.reserve(AES_IV_SIZE + cipher_len);
    result.insert(result.end(), iv, iv + AES_IV_SIZE);
    result.insert(result.end(), ciphertext.begin(), ciphertext.begin() + cipher_len);
    
    return CryptoResult(true, std::move(result));
}

CryptoResult AesEncryptor::decryptInternal(const unsigned char* ciphertext, size_t cipher_len) {
    if (key_.empty()) {
        return CryptoResult(false, {}, "Key not initialized");
    }
    
    if (cipher_len < AES_IV_SIZE + AES_BLOCK_SIZE) {
        return CryptoResult(false, {}, "Ciphertext too short");
    }
    
    clearError();
    
    // 提取IV
    unsigned char iv[AES_IV_SIZE];
    std::memcpy(iv, ciphertext, AES_IV_SIZE);
    
    // 密文长度
    size_t encrypted_len = cipher_len - AES_IV_SIZE;
    const unsigned char* encrypted_data = ciphertext + AES_IV_SIZE;
    
    // 解密
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        last_error_ = "Failed to create cipher context";
        return CryptoResult(false, {}, last_error_);
    }
    
    int len = 0;
    int plain_len = 0;
    std::vector<unsigned char> plaintext(encrypted_len + AES_BLOCK_SIZE);
    
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key_.data(), iv) != 1) {
        last_error_ = "Failed to initialize decryption";
        EVP_CIPHER_CTX_free(ctx);
        return CryptoResult(false, {}, last_error_);
    }
    
    if (EVP_DecryptUpdate(ctx, plaintext.data(), &len, encrypted_data, static_cast<int>(encrypted_len)) != 1) {
        last_error_ = "Decryption update failed";
        EVP_CIPHER_CTX_free(ctx);
        return CryptoResult(false, {}, last_error_);
    }
    plain_len = len;
    
    if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) != 1) {
        last_error_ = "Decryption final failed";
        EVP_CIPHER_CTX_free(ctx);
        return CryptoResult(false, {}, last_error_);
    }
    plain_len += len;
    
    EVP_CIPHER_CTX_free(ctx);
    
    // 移除PKCS7填充
    size_t unpadded_len = 0;
    if (!removePKCS7Padding(plaintext.data(), plain_len, unpadded_len)) {
        last_error_ = "Invalid padding";
        return CryptoResult(false, {}, last_error_);
    }
    
    std::vector<unsigned char> result(plaintext.begin(), plaintext.begin() + unpadded_len);
    return CryptoResult(true, std::move(result));
}

CryptoResult AesEncryptor::encrypt(const unsigned char* plaintext, size_t plain_len) {
    auto result = encryptInternal(plaintext, plain_len);
    if (result.success && result.data.size() > AES_IV_SIZE) {
        // 验证加密
        auto test_decrypt = decryptInternal(result.data.data(), result.data.size());
        if (!test_decrypt.success || 
            test_decrypt.data.size() != plain_len ||
            std::memcmp(test_decrypt.data.data(), plaintext, plain_len) != 0) {
            LOG_WARN("Encryption verification failed");
        }
    }
    return result;
}

CryptoResult AesEncryptor::decrypt(const unsigned char* ciphertext, size_t cipher_len) {
    return decryptInternal(ciphertext, cipher_len);
}

// =============================================================================
// PSK密钥管理器
// =============================================================================

PskKeyManager::PskKeyManager() {}

PskKeyManager::~PskKeyManager() {
    // 清除敏感数据
    std::fill(psk_.begin(), psk_.end(), 0);
    std::fill(current_key_.begin(), current_key_.end(), 0);
}

bool PskKeyManager::setPsk(const std::string& psk) {
    if (psk.length() < 16) {
        LOG_ERROR("PSK too short, minimum 16 characters required");
        return false;
    }
    
    psk_ = std::vector<unsigned char>(psk.begin(), psk.end());
    
    // 生成会话密钥（简化版，实际应使用TLS PSK）
    return deriveSessionKey("", "");
}

bool PskKeyManager::setPskFromHex(const std::string& psk_hex) {
    psk_ = AesEncryptor::hexToBytes(psk_hex);
    
    if (psk_.size() < AES_KEY_SIZE) {
        LOG_ERROR("PSK too short for AES-256");
        return false;
    }
    
    // 截取或扩展到256位
    psk_.resize(AES_KEY_SIZE);
    return true;
}

bool PskKeyManager::deriveSessionKey(const std::string& client_hello, const std::string& server_hello) {
    if (psk_.empty()) {
        LOG_ERROR("PSK not set");
        return false;
    }
    
    // 简化版密钥派生（实际应使用TLS PSK机制）
    std::vector<unsigned char> seed;
    seed.insert(seed.end(), psk_.begin(), psk_.end());
    seed.insert(seed.end(), client_hello.begin(), client_hello.end());
    seed.insert(seed.end(), server_hello.begin(), server_hello.end());
    
    std::string seed_str(seed.begin(), seed.end());
    std::string hash = AesEncryptor::sha256(seed_str);
    
    current_key_ = AesEncryptor::hexToBytes(hash);
    current_key_.resize(AES_KEY_SIZE);
    
    return true;
}

void PskKeyManager::reset() {
    std::fill(psk_.begin(), psk_.end(), 0);
    std::fill(current_key_.begin(), current_key_.end(), 0);
    psk_.clear();
    current_key_.clear();
}

// =============================================================================
// 加密工具
// =============================================================================

bool CryptoUtils::secureRandom(unsigned char* buffer, size_t length) {
    return RAND_bytes(buffer, length) == 1;
}

std::vector<unsigned char> CryptoUtils::secureRandom(size_t length) {
    std::vector<unsigned char> result(length);
    if (!secureRandom(result.data(), length)) {
        result.clear();
    }
    return result;
}

std::vector<unsigned char> CryptoUtils::generateKey(int key_size) {
    return secureRandom(key_size);
}

std::vector<unsigned char> CryptoUtils::generateIV() {
    return secureRandom(AES_IV_SIZE);
}

bool CryptoUtils::pbkdf2(
    const std::string& password,
    const std::string& salt,
    int iterations,
    int key_length,
    std::vector<unsigned char>& out_key
) {
    out_key.resize(key_length);
    
    int result = PKCS5_PBKDF2_HMAC(
        password.c_str(),
        password.length(),
        reinterpret_cast<const unsigned char*>(salt.c_str()),
        salt.length(),
        iterations,
        EVP_sha256(),
        key_length,
        out_key.data()
    );
    
    return result == 1;
}

std::vector<unsigned char> CryptoUtils::hmacSha256(
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& data
) {
    std::vector<unsigned char> result(SHA256_DIGEST_LENGTH);
    
    unsigned int len = 0;
    HMAC(EVP_sha256(), key.data(), key.size(), data.data(), data.size(), result.data(), &len);
    
    result.resize(len);
    return result;
}

bool CryptoUtils::constantTimeCompare(
    const std::vector<unsigned char>& a,
    const std::vector<unsigned char>& b
) {
    if (a.size() != b.size()) {
        return false;
    }
    
    unsigned char result = 0;
    for (size_t i = 0; i < a.size(); ++i) {
        result |= a[i] ^ b[i];
    }
    
    return result == 0;
}

} // namespace server
} // namespace smartsched
