/**
 * @file crypto.h
 * @brief AES-256加密模块
 * 
 * 安全视角: 医疗数据加密传输
 * 认证视角: 支持PSK密钥交换
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>

namespace smartsched {
namespace server {

// =============================================================================
// 加密算法配置
// =============================================================================
constexpr int AES_KEY_SIZE = 32;  // 256位
constexpr int AES_IV_SIZE = 16;   // 128位
constexpr int AES_BLOCK_SIZE = 16;
constexpr int SHA256_DIGEST_LENGTH = 32;

// =============================================================================
// 加密配置
// =============================================================================
struct CryptoConfig {
    // 密钥（32字节用于AES-256）
    std::vector<unsigned char> key;
    
    // 是否启用加密
    bool enabled = true;
    
    // 日志级别
    bool log_crypto_operations = false;
};

// =============================================================================
// 加密结果
// =============================================================================
struct CryptoResult {
    bool success;
    std::vector<unsigned char> data;
    std::string error_message;
    
    CryptoResult() : success(false) {}
    CryptoResult(bool ok, std::vector<unsigned char> d = {}, std::string err = "")
        : success(ok), data(std::move(d)), error_message(std::move(err)) {}
};

// =============================================================================
// AES加密器类
// =============================================================================
class AesEncryptor {
public:
    AesEncryptor();
    ~AesEncryptor();
    
    // 禁用拷贝
    SMARTSCHED_DISALLOW_COPY_AND_MOVE(AesEncryptor);
    
    // 初始化
    bool initialize(const std::vector<unsigned char>& key);
    bool initialize(const std::string& key_hex);
    
    // 加密数据
    CryptoResult encrypt(const unsigned char* plaintext, size_t plain_len);
    CryptoResult encrypt(const std::vector<unsigned char>& plaintext);
    CryptoResult encrypt(const std::string& plaintext);
    
    // 解密数据
    CryptoResult decrypt(const unsigned char* ciphertext, size_t cipher_len);
    CryptoResult decrypt(const std::vector<unsigned char>& ciphertext);
    CryptoResult decrypt(const std::string& ciphertext_hex);
    
    // 工具方法
    static std::string sha256(const std::string& input);
    static std::string sha256(const unsigned char* data, size_t len);
    static std::vector<unsigned char> hexToBytes(const std::string& hex);
    static std::string bytesToHex(const unsigned char* data, size_t len);
    static bool generateRandomKey(std::vector<unsigned char>& key, int length);
    
    // 错误处理
    std::string getLastError() const { return last_error_; }
    void clearError() { last_error_.clear(); }

private:
    std::vector<unsigned char> key_;
    std::string last_error_;
    
    // 加密实现
    CryptoResult encryptInternal(const unsigned char* plaintext, size_t plain_len);
    CryptoResult decryptInternal(const unsigned char* ciphertext, size_t cipher_len);
    
    // 填充
    std::vector<unsigned char> addPKCS7Padding(const unsigned char* data, size_t len);
    bool removePKCS7Padding(const unsigned char* data, size_t len, size_t& out_len);
};

// =============================================================================
// PSK密钥管理器
// =============================================================================
class PskKeyManager {
public:
    PskKeyManager();
    ~PskKeyManager();
    
    // 设置预共享密钥
    bool setPsk(const std::string& psk);
    bool setPskFromHex(const std::string& psk_hex);
    
    // 派生会话密钥
    bool deriveSessionKey(const std::string& client_hello, const std::string& server_hello);
    
    // 获取当前密钥
    const std::vector<unsigned char>& getCurrentKey() const { return current_key_; }
    
    // 重置
    void reset();

private:
    std::vector<unsigned char> psk_;
    std::vector<unsigned char> current_key_;
};

// =============================================================================
// 加密工具
// =============================================================================
class CryptoUtils {
public:
    // 密码学安全随机数生成
    static bool secureRandom(unsigned char* buffer, size_t length);
    static std::vector<unsigned char> secureRandom(size_t length);
    
    // 密钥生成
    static std::vector<unsigned char> generateKey(int key_size = AES_KEY_SIZE);
    static std::vector<unsigned char> generateIV();
    
    // 密钥派生（PBKDF2）
    static bool pbkdf2(
        const std::string& password,
        const std::string& salt,
        int iterations,
        int key_length,
        std::vector<unsigned char>& out_key
    );
    
    // HMAC-SHA256
    static std::vector<unsigned char> hmacSha256(
        const std::vector<unsigned char>& key,
        const std::vector<unsigned char>& data
    );
    
    // 时间常量比较（防止时序攻击）
    static bool constantTimeCompare(
        const std::vector<unsigned char>& a,
        const std::vector<unsigned char>& b
    );
};

// =============================================================================
// 内联实现
// =============================================================================

inline AesEncryptor::AesEncryptor() {}

inline AesEncryptor::~AesEncryptor() {}

inline bool AesEncryptor::initialize(const std::vector<unsigned char>& key) {
    if (key.size() != AES_KEY_SIZE) {
        last_error_ = "Invalid key size: expected " + 
                      std::to_string(AES_KEY_SIZE) + ", got " + 
                      std::to_string(key.size());
        return false;
    }
    
    key_ = key;
    return true;
}

inline bool AesEncryptor::initialize(const std::string& key_hex) {
    auto key_bytes = hexToBytes(key_hex);
    return initialize(key_bytes);
}

inline CryptoResult AesEncryptor::encrypt(const std::string& plaintext) {
    return encrypt(reinterpret_cast<const unsigned char*>(plaintext.data()), plaintext.size());
}

inline CryptoResult AesEncryptor::encrypt(const std::vector<unsigned char>& plaintext) {
    return encrypt(plaintext.data(), plaintext.size());
}

inline CryptoResult AesEncryptor::decrypt(const std::string& ciphertext_hex) {
    auto ciphertext = hexToBytes(ciphertext_hex);
    return decrypt(ciphertext);
}

inline CryptoResult AesEncryptor::decrypt(const std::vector<unsigned char>& ciphertext) {
    return decrypt(ciphertext.data(), ciphertext.size());
}

inline std::string AesEncryptor::sha256(const std::string& input) {
    return sha256(reinterpret_cast<const unsigned char*>(input.data()), input.size());
}

} // namespace server
} // namespace smartsched
