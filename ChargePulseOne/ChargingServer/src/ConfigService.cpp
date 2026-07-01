#include "ConfigService.h"
#include "SessionManager.h"
#include "DatabaseManager.h"
#include "Logger.h"

#ifdef _WIN32
    #include <winsock2.h>
#else
    #include <sys/socket.h>
    #include <unistd.h>
    using SOCKET = int;
#endif

static void response(SOCKET client, const std::string& cmd, const json& data, const std::string& token = "") {
    json msg; msg["cmd"]=cmd; msg["status"]="ok"; msg["data"]=data;
    if(!token.empty()) msg["token"]=token;
    std::string raw = msg.dump();
    send(client, (raw+"\n").c_str(), raw.size()+1, 0);
}
static void error(SOCKET client, const std::string& cmd, const std::string& message) {
    json msg; msg["cmd"]=cmd; msg["status"]="error"; msg["message"]=message;
    std::string raw = msg.dump();
    send(client, (raw+"\n").c_str(), raw.size()+1, 0);
}

void handleConfigGet(SOCKET client, const json& data, const std::string& token) {
    Session sess;
    if(!SessionManager::instance().validate(token, sess) || sess.role!="admin") {
        error(client, CMD::SYS_CONFIG_GET, "Permission denied"); return;
    }
    
    try {
        auto rows = DatabaseManager::instance().query("SELECT * FROM sys_config");
        json resp = json::object();
        for(auto& r : rows) {
            resp[r["key"]] = r["value"];
        }
        response(client, CMD::SYS_CONFIG_GET, resp, token);
    } catch (const std::exception& e) {
        Logger::instance().log(ERROR, "ConfigGet error: " + std::string(e.what()));
        error(client, CMD::SYS_CONFIG_GET, "Failed to get config");
    }
}

void handleConfigSet(SOCKET client, const json& data, const std::string& token) {
    Session sess;
    if(!SessionManager::instance().validate(token, sess) || sess.role!="admin") {
        error(client, CMD::SYS_CONFIG_SET, "Permission denied"); return;
    }
    
    std::string key = data.value("key", "");
    std::string value = data.value("value", "");
    
    // 输入验证
    if(key.empty()) { 
        error(client, CMD::SYS_CONFIG_SET, "key required"); 
        return; 
    }
    
    // 安全检查：验证key是否只包含安全字符
    if (!DatabaseManager::instance().isSafeInput(key)) {
        Logger::instance().log(WARN, "Invalid config key format: " + key);
        error(client, CMD::SYS_CONFIG_SET, "Invalid key format");
        return;
    }
    
    try {
        // 使用安全的参数化查询
        std::vector<SQLBinding> params = {
            {"key", key, true},
            {"value", value, true}
        };
        
        DatabaseManager::instance().executeWithParams(
            "REPLACE INTO sys_config (`key`, `value`) VALUES (:key, :value)",
            params
        );
        
        Logger::instance().log(INFO, "Config updated: " + key);
        response(client, CMD::SYS_CONFIG_SET, {{"message","Config updated"}}, token);
    } catch (const std::exception& e) {
        Logger::instance().log(ERROR, "ConfigSet error: " + std::string(e.what()));
        error(client, CMD::SYS_CONFIG_SET, "Failed to update config");
    }
}
