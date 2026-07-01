#include "NotificationService.h"
#include "SessionManager.h"
#include "DatabaseManager.h"

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

// 设备注册推送通道（模拟）
void handleNotificationRegister(SOCKET client, const json& data, const std::string& token) {
    Session sess;
    if(!SessionManager::instance().validate(token, sess)) {
        error(client, "notification", "Auth required"); return;
    }
    std::string deviceToken = data.value("device_token", "");
    if(deviceToken.empty()) {
        error(client, "notification", "device_token required"); return;
    }
    // 保存设备令牌到用户记录（简化）
    DatabaseManager::instance().execute(
        "UPDATE users SET device_token='" + deviceToken + "' WHERE id=" + std::to_string(sess.userId)
    );
    response(client, "notification", {{"message","Device registered"}}, token);
}
