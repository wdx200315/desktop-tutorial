#pragma once
#include "common.h"
#include <string>
#include "json.hpp"

using json = nlohmann::json;

class ProtocolParser {
public:
    // 构建协议消息 {"cmd":"...", "data":{...}, "token":"..."}
    static std::string buildMessage(const std::string& cmd, const json& data = {}, 
                                    const std::string& token = "");
    // 解析从客户端收到的字符串
    static bool parse(const std::string& raw, std::string& cmd, json& data, std::string& token);
};
