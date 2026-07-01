#include "CryptoUtils.h"

std::string CryptoUtils::sha256(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), input.size(), hash);
    std::stringstream ss;
    for(int i=0;i<SHA256_DIGEST_LENGTH;++i)
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    return ss.str();
}

std::string CryptoUtils::aesEncrypt(const std::string& plain, const std::string& key, const std::string& iv) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, (unsigned char*)key.c_str(), (unsigned char*)iv.c_str());
    int len = 0, cipherlen = 0;
    std::vector<unsigned char> cipher(plain.size() + AES_BLOCK_SIZE);
    EVP_EncryptUpdate(ctx, cipher.data(), &len, (unsigned char*)plain.c_str(), plain.size());
    cipherlen = len;
    EVP_EncryptFinal_ex(ctx, cipher.data() + len, &len);
    cipherlen += len;
    EVP_CIPHER_CTX_free(ctx);
    cipher.resize(cipherlen);
    return base64Encode(cipher.data(), cipherlen);
}

std::string CryptoUtils::aesDecrypt(const std::string& b64, const std::string& key, const std::string& iv) {
    std::string raw = base64Decode(b64);
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, (unsigned char*)key.c_str(), (unsigned char*)iv.c_str());
    int len = 0, plainlen = 0;
    std::vector<unsigned char> plain(raw.size());
    EVP_DecryptUpdate(ctx, plain.data(), &len, (unsigned char*)raw.data(), raw.size());
    plainlen = len;
    EVP_DecryptFinal_ex(ctx, plain.data() + len, &len);
    plainlen += len;
    EVP_CIPHER_CTX_free(ctx);
    plain.resize(plainlen);
    return std::string(plain.begin(), plain.end());
}

std::string CryptoUtils::base64Encode(const unsigned char* data, size_t len) {
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* bio = BIO_new(BIO_s_mem());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_push(b64, bio);
    BIO_write(bio, data, len);
    BIO_flush(bio);
    BUF_MEM* buf;
    BIO_get_mem_ptr(bio, &buf);
    std::string result(buf->data, buf->length);
    BIO_free_all(bio);
    return result;
}

std::string CryptoUtils::base64Decode(const std::string& input) {
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* bio = BIO_new_mem_buf(input.c_str(), input.length());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_push(b64, bio);
    std::vector<char> buf(input.length());
    int len = BIO_read(bio, buf.data(), input.length());
    BIO_free_all(bio);
    return std::string(buf.data(), len);
}
