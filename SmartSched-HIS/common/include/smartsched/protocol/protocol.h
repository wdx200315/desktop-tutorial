/**
 * @file protocol.h
 * @brief 通信协议基础定义
 * 
 * 网络视角: 定义服务端与客户端的通信协议
 * 安全视角: 支持AES-256加密传输
 */

#pragma once

#include "../common/compiler.h"
#include "../common/version.h"
#include <string>
#include <cstdint>

namespace smartsched {
namespace protocol {

// =============================================================================
// 协议常量
// =============================================================================

// 协议版本（与服务端协商使用）
static constexpr int PROTOCOL_VERSION_MAJOR = 1;
static constexpr int PROTOCOL_VERSION_MINOR = 0;

// 消息分隔符（用于TCP流式传输中的消息边界）
static constexpr char MESSAGE_DELIMITER = '\n';

// 最大消息长度（防止恶意大消息攻击）
static constexpr size_t MAX_MESSAGE_SIZE = 1024 * 1024;  // 1MB

// 心跳间隔（秒）
static constexpr int HEARTBEAT_INTERVAL = 30;

// 连接超时（秒）
static constexpr int CONNECTION_TIMEOUT = 300;

// =============================================================================
// 消息类型
// =============================================================================
enum class MessageType : uint8_t {
    // 业务请求/响应
    Request = 0x01,
    Response = 0x02,
    
    // 控制消息
    Heartbeat = 0x10,
    HeartbeatAck = 0x11,
    
    // 错误消息
    Error = 0xFF
};

// =============================================================================
// 消息状态码
// =============================================================================
enum class StatusCode : uint16_t {
    Success = 0,
    
    // 客户端错误（1xxx）
    InvalidRequest = 1001,
    InvalidParams = 1002,
    Unauthorized = 1003,
    SessionExpired = 1004,
    RateLimited = 1005,
    
    // 业务错误（2xxx）
    PatientNotFound = 2001,
    DepartmentNotFound = 2002,
    DoctorNotFound = 2003,
    QueueFull = 2004,
    AlreadyInQueue = 2005,
    NotInQueue = 2006,
    AlreadyConsulting = 2007,
    UltrasoundUnavailable = 2008,
    
    // 服务器错误（5xxx）
    InternalError = 5001,
    DatabaseError = 5002,
    CryptoError = 5003,
    NetworkError = 5004,
    ServerBusy = 5005
};

// =============================================================================
// 消息头结构
// =============================================================================
#pragma pack(push, 1)
struct MessageHeader {
    uint32_t magic;        // 协议魔数 0x534D5254 ("SMRT")
    uint8_t  version;      // 协议版本
    MessageType type;      // 消息类型
    uint16_t status;       // 状态码
    uint32_t sequence;     // 序列号（用于请求-响应匹配）
    uint32_t body_length;  // 消息体长度
    uint32_t checksum;     // 校验和（可选）
};
#pragma pack(pop)

// =============================================================================
// 协议魔数
// =============================================================================
static constexpr uint32_t PROTOCOL_MAGIC = 0x534D5254;  // "SMRT" - SmartSched

// =============================================================================
// 协议工具函数
// =============================================================================

// 验证消息头有效性
SMARTSCHED_NODISCARD
inline bool isValidHeader(const MessageHeader& header) {
    return header.magic == PROTOCOL_MAGIC &&
           header.version >= PROTOCOL_VERSION_MAJOR &&
           header.body_length <= MAX_MESSAGE_SIZE;
}

// 获取状态码描述
SMARTSCHED_NODISCARD
inline const char* getStatusDescription(StatusCode code) {
    switch (code) {
        case StatusCode::Success: return "Success";
        
        // 客户端错误
        case StatusCode::InvalidRequest: return "Invalid request format";
        case StatusCode::InvalidParams: return "Invalid parameters";
        case StatusCode::Unauthorized: return "Unauthorized access";
        case StatusCode::SessionExpired: return "Session expired";
        case StatusCode::RateLimited: return "Rate limit exceeded";
        
        // 业务错误
        case StatusCode::PatientNotFound: return "Patient not found";
        case StatusCode::DepartmentNotFound: return "Department not found";
        case StatusCode::DoctorNotFound: return "Doctor not found";
        case StatusCode::QueueFull: return "Queue is full";
        case StatusCode::AlreadyInQueue: return "Already in queue";
        case StatusCode::NotInQueue: return "Not in queue";
        case StatusCode::AlreadyConsulting: return "Already in consultation";
        case StatusCode::UltrasoundUnavailable: return "Ultrasound machine unavailable";
        
        // 服务器错误
        case StatusCode::InternalError: return "Internal server error";
        case StatusCode::DatabaseError: return "Database operation failed";
        case StatusCode::CryptoError: return "Encryption/decryption failed";
        case StatusCode::NetworkError: return "Network error";
        case StatusCode::ServerBusy: return "Server is busy";
        
        default: return "Unknown error";
    }
}

} // namespace protocol
} // namespace smartsched
