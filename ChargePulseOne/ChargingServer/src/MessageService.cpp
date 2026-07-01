#include "MessageService.h"
#include "Logger.h"
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

// 消息列表（用户自己的）
void handleMessageList(SOCKET client, const json& data, const std::string& token) {
    Session sess;
    if(!SessionManager::instance().validate(token, sess)) {
        error(client, CMD::MESSAGE_LIST, "Auth required"); return;
    }
    int page = data.value("page",1);
    int size = data.value("size",20);
    int offset = (page-1)*size;
    auto totalRes = DatabaseManager::instance().query(
        "SELECT COUNT(*) as cnt FROM messages WHERE user_id=" + std::to_string(sess.userId)
    );
    int total = totalRes.empty()?0:std::stoi(totalRes[0]["cnt"]);
    auto rows = DatabaseManager::instance().query(
        "SELECT * FROM messages WHERE user_id=" + std::to_string(sess.userId) +
        " ORDER BY id DESC LIMIT " + std::to_string(size) + " OFFSET " + std::to_string(offset)
    );
    json resp;
    resp["total"] = total;
    resp["list"] = json::array();
    for(auto& r: rows) resp["list"].push_back(r);
    response(client, CMD::MESSAGE_LIST, resp, token);
}

// 发送消息（管理员/运营商）
void handleMessageSend(SOCKET client, const json& data, const std::string& token) {
    Session sess;
    if(!SessionManager::instance().validate(token, sess) || (sess.role!="admin" && sess.role!="operator")) {
        error(client, CMD::MESSAGE_SEND, "Permission denied"); return;
    }
    int toUser = data.value("user_id", 0);
    std::string title = data.value("title", "");
    std::string content = data.value("content", "");
    if(toUser<=0 || title.empty() || content.empty()) {
        error(client, CMD::MESSAGE_SEND, "user_id, title, content required");
        return;
    }
    DatabaseManager::instance().execute(
        "INSERT INTO messages (user_id, title, content) VALUES (" +
        std::to_string(toUser) + ",'" + title + "','" + content + "')"
    );
    // 记录操作日志
    DatabaseManager::instance().execute(
        "INSERT INTO operation_logs (user_id, type, description) VALUES (" +
        std::to_string(sess.userId) + ",'message','Sent message to user " + std::to_string(toUser) + "')"
    );
    response(client, CMD::MESSAGE_SEND, {{"message","Sent"}}, token);
}

// 标记已读
void handleMessageRead(SOCKET client, const json& data, const std::string& token) {
    Session sess;
    if(!SessionManager::instance().validate(token, sess)) {
        error(client, CMD::MESSAGE_READ, "Auth required"); return;
    }
    int msgId = data.value("id", 0);
    auto chk = DatabaseManager::instance().query(
        "SELECT id FROM messages WHERE id=" + std::to_string(msgId) + " AND user_id=" + std::to_string(sess.userId)
    );
    if(chk.empty()) { error(client, CMD::MESSAGE_READ, "Message not found"); return; }
    DatabaseManager::instance().execute("UPDATE messages SET is_read=1 WHERE id=" + std::to_string(msgId));
    response(client, CMD::MESSAGE_READ, {{"message","Read"}}, token);
}

// 删除消息
void handleMessageDelete(SOCKET client, const json& data, const std::string& token) {
    Session sess;
    if(!SessionManager::instance().validate(token, sess)) {
        error(client, CMD::MESSAGE_DELETE, "Auth required"); return;
    }
    int msgId = data.value("id", 0);
    auto chk = DatabaseManager::instance().query(
        "SELECT id FROM messages WHERE id=" + std::to_string(msgId) + " AND user_id=" + std::to_string(sess.userId)
    );
    if(chk.empty()) { error(client, CMD::MESSAGE_DELETE, "Not found"); return; }
    DatabaseManager::instance().execute("DELETE FROM messages WHERE id=" + std::to_string(msgId));
    response(client, CMD::MESSAGE_DELETE, {{"message","Deleted"}}, token);
}
