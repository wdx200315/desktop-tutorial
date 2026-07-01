/**
 * @file bytebuffer.h
 * @brief 字节缓冲区工具
 * 
 * 网络视角: 处理TCP流式数据的读写
 * 加密视角: 支持加密前的数据准备
 */

#pragma once

#include "../common/compiler.h"
#include <vector>
#include <string>
#include <cstring>
#include <cstdint>

namespace smartsched {
namespace utils {

// =============================================================================
// 字节缓冲区
// =============================================================================
class ByteBuffer {
public:
    // 构造函数
    ByteBuffer() : read_pos_(0) {}
    
    explicit ByteBuffer(size_t capacity) : buffer_(capacity), read_pos_(0) {}
    
    ByteBuffer(const void* data, size_t size) {
        if (data && size > 0) {
            buffer_.assign(static_cast<const uint8_t*>(data),
                          static_cast<const uint8_t*>(data) + size);
        }
        read_pos_ = 0;
    }
    
    // 复制构造函数
    ByteBuffer(const ByteBuffer& other)
        : buffer_(other.buffer_), read_pos_(other.read_pos_) {}
    
    // 移动构造函数
    ByteBuffer(ByteBuffer&& other) noexcept
        : buffer_(std::move(other.buffer_)), read_pos_(other.read_pos_) {
        other.read_pos_ = 0;
    }
    
    // 赋值运算符
    ByteBuffer& operator=(const ByteBuffer& other) {
        if (this != &other) {
            buffer_ = other.buffer_;
            read_pos_ = other.read_pos_;
        }
        return *this;
    }
    
    ByteBuffer& operator=(ByteBuffer&& other) noexcept {
        if (this != &other) {
            buffer_ = std::move(other.buffer_);
            read_pos_ = other.read_pos_;
            other.read_pos_ = 0;
        }
        return *this;
    }
    
    // =============================================================================
    // 写入操作
    // =============================================================================
    
    void writeUInt8(uint8_t value) {
        buffer_.push_back(value);
    }
    
    void writeUInt16(uint16_t value) {
        buffer_.push_back(static_cast<uint8_t>(value & 0xFF));
        buffer_.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    }
    
    void writeUInt32(uint32_t value) {
        for (int i = 0; i < 4; ++i) {
            buffer_.push_back(static_cast<uint8_t>(value & 0xFF));
            value >>= 8;
        }
    }
    
    void writeUInt64(uint64_t value) {
        for (int i = 0; i < 8; ++i) {
            buffer_.push_back(static_cast<uint8_t>(value & 0xFF));
            value >>= 8;
        }
    }
    
    void writeBytes(const void* data, size_t size) {
        if (data && size > 0) {
            const uint8_t* bytes = static_cast<const uint8_t*>(data);
            buffer_.insert(buffer_.end(), bytes, bytes + size);
        }
    }
    
    void writeString(const std::string& str) {
        // 先写入长度（4字节），再写入内容
        writeUInt32(static_cast<uint32_t>(str.size()));
        writeBytes(str.data(), str.size());
    }
    
    void append(const ByteBuffer& other) {
        buffer_.insert(buffer_.end(), other.buffer_.begin(), other.buffer_.end());
    }
    
    // =============================================================================
    // 读取操作
    // =============================================================================
    
    bool readUInt8(uint8_t& value) {
        if (read_pos_ >= buffer_.size()) return false;
        value = buffer_[read_pos_++];
        return true;
    }
    
    bool readUInt16(uint16_t& value) {
        if (read_pos_ + 2 > buffer_.size()) return false;
        value = static_cast<uint16_t>(buffer_[read_pos_]) |
               (static_cast<uint16_t>(buffer_[read_pos_ + 1]) << 8);
        read_pos_ += 2;
        return true;
    }
    
    bool readUInt32(uint32_t& value) {
        if (read_pos_ + 4 > buffer_.size()) return false;
        value = 0;
        for (int i = 0; i < 4; ++i) {
            value |= (static_cast<uint32_t>(buffer_[read_pos_ + i]) << (i * 8));
        }
        read_pos_ += 4;
        return true;
    }
    
    bool readUInt64(uint64_t& value) {
        if (read_pos_ + 8 > buffer_.size()) return false;
        value = 0;
        for (int i = 0; i < 8; ++i) {
            value |= (static_cast<uint64_t>(buffer_[read_pos_ + i]) << (i * 8));
        }
        read_pos_ += 8;
        return true;
    }
    
    bool readBytes(void* data, size_t size) {
        if (read_pos_ + size > buffer_.size()) return false;
        std::memcpy(data, buffer_.data() + read_pos_, size);
        read_pos_ += size;
        return true;
    }
    
    bool readString(std::string& str) {
        uint32_t len = 0;
        if (!readUInt32(len)) return false;
        
        if (read_pos_ + len > buffer_.size()) return false;
        str.assign(buffer_.begin() + read_pos_,
                  buffer_.begin() + read_pos_ + len);
        read_pos_ += len;
        return true;
    }
    
    // =============================================================================
    // 状态查询
    // =============================================================================
    
    size_t size() const { return buffer_.size(); }
    size_t readableBytes() const { return buffer_.size() - read_pos_; }
    bool hasMore() const { return read_pos_ < buffer_.size(); }
    bool isEmpty() const { return buffer_.empty(); }
    
    const uint8_t* data() const { return buffer_.data(); }
    uint8_t* data() { return buffer_.data(); }
    
    void clear() {
        buffer_.clear();
        read_pos_ = 0;
    }
    
    void resetReadPos() {
        read_pos_ = 0;
    }
    
    // =============================================================================
    // 转换
    // =============================================================================
    
    std::string toString() const {
        return std::string(buffer_.begin(), buffer_.end());
    }
    
    std::string toHexString() const {
        static const char hex_chars[] = "0123456789ABCDEF";
        std::string result;
        result.reserve(buffer_.size() * 2);
        for (uint8_t byte : buffer_) {
            result += hex_chars[(byte >> 4) & 0x0F];
            result += hex_chars[byte & 0x0F];
        }
        return result;
    }
    
    // =============================================================================
    // 静态工具方法
    // =============================================================================
    
    static ByteBuffer fromString(const std::string& str) {
        return ByteBuffer(str.data(), str.size());
    }
    
    static ByteBuffer fromHex(const std::string& hex) {
        ByteBuffer buf;
        if (hex.size() % 2 != 0) return buf;
        
        for (size_t i = 0; i < hex.size(); i += 2) {
            uint8_t byte = (hexCharToNibble(hex[i]) << 4) | hexCharToNibble(hex[i + 1]);
            buf.writeUInt8(byte);
        }
        return buf;
    }

private:
    static uint8_t hexCharToNibble(char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return 0;
    }
    
    std::vector<uint8_t> buffer_;
    size_t read_pos_;
};

} // namespace utils
} // namespace smartsched
