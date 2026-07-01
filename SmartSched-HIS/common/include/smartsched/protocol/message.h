/**
 * @file message.h
 * @brief 消息结构定义
 * 
 * 网络视角: 定义完整的消息格式
 * 开发者视角: 提供便捷的消息构建/解析接口
 */

#pragma once

#include "protocol.h"
#include "commands.h"
#include <string>
#include <memory>
#include <chrono>
#include <functional>

namespace smartsched {
namespace protocol {

// =============================================================================
// 时间戳类型
// =============================================================================
using Timestamp = std::chrono::milliseconds;

inline Timestamp currentTimestamp() {
    return std::chrono::duration_cast<Timestamp>(
        std::chrono::system_clock::now().time_since_epoch()
    );
}

// =============================================================================
// 基础消息类
// =============================================================================
struct Message {
    // 消息头
    uint32_t magic = PROTOCOL_MAGIC;
    uint8_t  version = PROTOCOL_VERSION_MAJOR;
    MessageType type = MessageType::Request;
    uint16_t status = 0;
    uint32_t sequence = 0;
    
    // 元数据
    Timestamp timestamp;
    std::string session_id;
    std::string user_id;
    
    // 命令和消息体
    CommandID command = CommandID::PING;
    std::string body;
    
    Message() : timestamp(currentTimestamp()) {}
    
    virtual ~Message() = default;
    
    // 序列化接口
    virtual std::string serialize() const = 0;
    
    // 反序列化接口
    virtual bool deserialize(const std::string& data) = 0;
    
    // 校验消息
    bool validate() const {
        return magic == PROTOCOL_MAGIC && 
               version >= PROTOCOL_VERSION_MAJOR &&
               !body.empty();
    }
    
    // 创建响应消息
    std::unique_ptr<Message> createResponse() const {
        auto resp = std::make_unique<Message>();
        resp->sequence = this->sequence;
        resp->type = MessageType::Response;
        resp->session_id = this->session_id;
        resp->user_id = this->user_id;
        return resp;
    }
    
    // 创建错误响应
    std::unique_ptr<Message> createErrorResponse(StatusCode code, const std::string& error_msg = "") const {
        auto resp = createResponse();
        resp->status = static_cast<uint16_t>(code);
        resp->body = error_msg.empty() ? getStatusDescription(code) : error_msg;
        return resp;
    }
};

// =============================================================================
// JSON消息类
// =============================================================================
struct JsonMessage : public Message {
    // 扩展字段（用于JSON消息）
    std::string method;  // 命令方法名
    
    JsonMessage() : Message() {
        type = MessageType::Request;
    }
    
    std::string serialize() const override {
        // 简化版本：直接返回body作为JSON
        // 完整版本需要包含完整的消息头序列化
        return body;
    }
    
    bool deserialize(const std::string& data) override {
        body = data;
        return !data.empty();
    }
};

// =============================================================================
// 请求消息类
// =============================================================================
struct RequestMessage : public JsonMessage {
    RequestMessage() : JsonMessage() {
        type = MessageType::Request;
    }
    
    RequestMessage(CommandID cmd, const std::string& req_body = "")
        : JsonMessage() {
        command = cmd;
        body = req_body;
    }
    
    // 静态工厂方法
    static std::unique_ptr<RequestMessage> create(
        CommandID cmd, 
        const std::string& body = "",
        uint32_t seq = 0
    ) {
        auto msg = std::make_unique<RequestMessage>(cmd, body);
        msg->sequence = seq;
        return msg;
    }
};

// =============================================================================
// 响应消息类
// =============================================================================
struct ResponseMessage : public JsonMessage {
    ResponseMessage() : JsonMessage() {
        type = MessageType::Response;
        status = static_cast<uint16_t>(StatusCode::Success);
    }
    
    ResponseMessage(uint32_t seq, StatusCode code = StatusCode::Success)
        : JsonMessage() {
        sequence = seq;
        status = static_cast<uint16_t>(code);
        type = MessageType::Response;
    }
    
    // 快速成功响应
    static std::unique_ptr<ResponseMessage> success(
        uint32_t seq, 
        const std::string& body = ""
    ) {
        auto resp = std::make_unique<ResponseMessage>(seq, StatusCode::Success);
        resp->body = body;
        return resp;
    }
    
    // 快速错误响应
    static std::unique_ptr<ResponseMessage> error(
        uint32_t seq, 
        StatusCode code
    ) {
        auto resp = std::make_unique<ResponseMessage>(seq, code);
        resp->body = getStatusDescription(code);
        return resp;
    }
    
    bool isSuccess() const {
        return status == 0;
    }
    
    const char* getErrorMessage() const {
        return getStatusDescription(static_cast<StatusCode>(status));
    }
};

// =============================================================================
// 心跳消息
// =============================================================================
struct HeartbeatMessage : public Message {
    int64_t client_time;
    int64_t server_time;
    
    HeartbeatMessage() : Message() {
        type = MessageType::Heartbeat;
        command = CommandID::PING;
        client_time = currentTimestamp().count();
    }
};

struct HeartbeatAckMessage : public Message {
    int64_t client_time;
    int64_t server_time;
    int64_t round_trip_time;
    
    HeartbeatAckMessage() : Message() {
        type = MessageType::HeartbeatAck;
        command = CommandID::PONG;
        server_time = currentTimestamp().count();
    }
    
    static std::unique_ptr<HeartbeatAckMessage> fromHeartbeat(
        const HeartbeatMessage& hb, 
        int64_t arrival_time
    ) {
        auto ack = std::make_unique<HeartbeatAckMessage>();
        ack->sequence = hb.sequence;
        ack->client_time = hb.client_time;
        ack->server_time = arrival_time;
        ack->round_trip_time = arrival_time - hb.client_time;
        return ack;
    }
};

// =============================================================================
// 消息解析器
// =============================================================================
class MessageParser {
public:
    // 解析JSON格式的消息
    static std::unique_ptr<Message> parseJson(const std::string& json_str) {
        // 简化实现：假设JSON格式为 {"cmd": 1001, "body": {...}}
        auto msg = std::make_unique<JsonMessage>();
        if (msg->deserialize(json_str)) {
            return msg;
        }
        return nullptr;
    }
    
    // 检查是否包含完整消息（用于TCP流式解析）
    static size_t findMessageBoundary(const std::string& buffer) {
        return buffer.find(MESSAGE_DELIMITER);
    }
    
    // 从缓冲区提取一条消息
    static std::pair<std::string, std::string> extractMessage(std::string& buffer) {
        auto pos = buffer.find(MESSAGE_DELIMITER);
        if (pos == std::string::npos) {
            return {"", buffer};
        }
        
        std::string msg = buffer.substr(0, pos);
        buffer.erase(0, pos + 1);  // +1 for delimiter
        return {msg, buffer};
    }
};

// =============================================================================
// 消息构建器
// =============================================================================
class MessageBuilder {
public:
    static std::string buildJsonRequest(
        CommandID cmd, 
        const std::string& body,
        uint32_t sequence = 0
    ) {
        // 简化格式：{"cmd": <cmd_id>, "seq": <seq>, "body": <body>}
        return "{\"cmd\":" + std::to_string(static_cast<uint16_t>(cmd)) +
               ",\"seq\":" + std::to_string(sequence) +
               ",\"body\":" + body + "}";
    }
    
    static std::string buildJsonResponse(
        uint32_t sequence,
        StatusCode status,
        const std::string& body
    ) {
        return "{\"seq\":" + std::to_string(sequence) +
               ",\"status\":" + std::to_string(static_cast<uint16_t>(status)) +
               ",\"body\":" + body + "}";
    }
    
    static std::string buildHeartbeat() {
        return "{\"cmd\":" + std::to_string(static_cast<uint16_t>(CommandID::PING)) +
               ",\"ts\":" + std::to_string(currentTimestamp().count()) + "}";
    }
};

} // namespace protocol
} // namespace smartsched
