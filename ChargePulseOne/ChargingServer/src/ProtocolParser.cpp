#include "ProtocolParser.h"
#include "Logger.h"

std::string ProtocolParser::buildMessage(const std::string& cmd, const json& data, const std::string& token) {
    json msg;
    msg["cmd"] = cmd;
    msg["data"] = data;
    if(!token.empty()) msg["token"] = token;
    return msg.dump();
}

bool ProtocolParser::parse(const std::string& raw, std::string& cmd, json& data, std::string& token) {
    try {
        json j = json::parse(raw);
        cmd = j.value("cmd", "");
        data = j.value("data", json::object());
        token = j.value("token", "");
        return true;
    } catch(std::exception& e) {
        Logger::instance().log(ERROR, "JSON parse error: " + std::string(e.what()));
        return false;
    }
}
