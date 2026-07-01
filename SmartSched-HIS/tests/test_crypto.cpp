/**
 * @file test_crypto.cpp
 * @brief 加密模块单元测试
 */
#include <gtest/gtest.h>
#include "smartsched/crypto/crypto.h"

using namespace smartsched;

// ==================== AES加密测试 ====================

class CryptoTest : public ::testing::Test {
protected:
    void SetUp() override {
        Crypto::instance().setKey("1234567890123456", "1234567890123456");
    }
};

TEST_F(CryptoTest, EncryptDecrypt)
{
    std::string plaintext = "Hello, SmartSched!";
    
    std::string encrypted = Crypto::instance().encrypt(plaintext);
    std::string decrypted = Crypto::instance().decrypt(encrypted);
    
    EXPECT_EQ(decrypted, plaintext);
    EXPECT_NE(encrypted, plaintext);
}

TEST_F(CryptoTest, EncryptEmptyString)
{
    std::string plaintext = "";
    
    std::string encrypted = Crypto::instance().encrypt(plaintext);
    std::string decrypted = Crypto::instance().decrypt(encrypted);
    
    EXPECT_EQ(decrypted, plaintext);
}

TEST_F(CryptoTest, EncryptLongString)
{
    std::string plaintext(10000, 'A');
    
    std::string encrypted = Crypto::instance().encrypt(plaintext);
    std::string decrypted = Crypto::instance().decrypt(encrypted);
    
    EXPECT_EQ(decrypted, plaintext);
}

TEST_F(CryptoTest, EncryptWithSpecialChars)
{
    std::string plaintext = "中文测试!@#$%^&*()_+-=[]{}|;':\",./<>?`~";
    
    std::string encrypted = Crypto::instance().encrypt(plaintext);
    std::string decrypted = Crypto::instance().decrypt(encrypted);
    
    EXPECT_EQ(decrypted, plaintext);
}

TEST_F(CryptoTest, EncryptDifferentOutputs)
{
    std::string plaintext = "Test message";
    
    // 使用不同的IV应该产生不同的密文
    std::string enc1 = Crypto::instance().encrypt(plaintext);
    
    Crypto::instance().setIV("1111111111111111");
    std::string enc2 = Crypto::instance().encrypt(plaintext);
    
    EXPECT_NE(enc1, enc2);
    
    // 但解密后应该相同
    EXPECT_EQ(plaintext, Crypto::instance().decrypt(enc1));
}

TEST_F(CryptoTest, HashSHA256)
{
    std::string input = "password123";
    std::string hash = Crypto::instance().sha256(input);
    
    EXPECT_EQ(hash.length(), 64);  // SHA256产生64个十六进制字符
    
    // 相同输入应产生相同哈希
    std::string hash2 = Crypto::instance().sha256(input);
    EXPECT_EQ(hash, hash2);
}

TEST_F(CryptoTest, HashDifferentInputs)
{
    std::string hash1 = Crypto::instance().sha256("input1");
    std::string hash2 = Crypto::instance().sha256("input2");
    
    EXPECT_NE(hash1, hash2);
}

TEST_F(CryptoTest, Base64Encode)
{
    std::string input = "Hello, World!";
    std::string encoded = Crypto::instance().base64Encode(input);
    
    EXPECT_TRUE(encoded.find('=') != std::string::npos || 
                encoded.length() >= input.length());
}

TEST_F(CryptoTest, Base64Decode)
{
    std::string input = "Hello, World!";
    std::string encoded = Crypto::instance().base64Encode(input);
    std::string decoded = Crypto::instance().base64Decode(encoded);
    
    EXPECT_EQ(decoded, input);
}

TEST_F(CryptoTest, GenerateRandomKey)
{
    std::string key1 = Crypto::instance().generateRandomKey(16);
    std::string key2 = Crypto::instance().generateRandomKey(16);
    
    EXPECT_EQ(key1.length(), 16);
    EXPECT_EQ(key2.length(), 16);
    EXPECT_NE(key1, key2);
}

// ==================== 边界条件测试 ====================

class CryptoBoundaryTest : public ::testing::Test {
protected:
    void SetUp() override {
        Crypto::instance().setKey("1234567890123456", "1234567890123456");
    }
};

TEST_F(CryptoBoundaryTest, VeryLongInput)
{
    std::string plaintext(1 << 20, 'X');  // 1MB
    std::string encrypted = Crypto::instance().encrypt(plaintext);
    std::string decrypted = Crypto::instance().decrypt(encrypted);
    
    EXPECT_EQ(decrypted, plaintext);
}

TEST_F(CryptoBoundaryTest, UnicodeInput)
{
    std::string plaintext = "中文测试 😀 éèê";
    std::string encrypted = Crypto::instance().encrypt(plaintext);
    std::string decrypted = Crypto::instance().decrypt(encrypted);
    
    EXPECT_EQ(decrypted, plaintext);
}

TEST_F(CryptoBoundaryTest, CorruptedCiphertext)
{
    std::string plaintext = "Test message";
    std::string encrypted = Crypto::instance().encrypt(plaintext);
    
    // 篡改密文
    if (encrypted.length() > 10) {
        encrypted[10] = 'X';
    }
    
    EXPECT_THROW({
        Crypto::instance().decrypt(encrypted);
    }, std::runtime_error);
}
