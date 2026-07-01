#include "LogService.h"
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

void handleLogQuery(SOCKET client, const json& data, const std::string& token) {
    Session sess;
    if(!SessionManager::instance().validate(token, sess) || sess.role != "admin") {
        error(client, CMD::LOG_QUERY, "Permission denied"); return;
    }
    int page = data.value("page", 1);
    int size = data.value("size", 20);
    std::string type = data.value("type", "");
    std::string where = "WHERE 1=1";
    if(!type.empty()) where += " AND type='" + type + "'";
    auto totalRes = DatabaseManager::instance().query("SELECT COUNT(*) as cnt FROM operation_logs " + where);
    int total = totalRes.empty() ? 0 : std::stoi(totalRes[0]["cnt"]);
    int offset = (page - 1) * size;
    auto rows = DatabaseManager::instance().query(
        "SELECT * FROM operation_logs " + where + " ORDER BY id DESC LIMIT " + std::to_string(size) + " OFFSET " + std::to_string(offset)
    );
    json resp;
    resp["total"] = total;
    resp["list"] = json::array();
    for(auto& r : rows) resp["list"].push_back(r);
    response(client, CMD::LOG_QUERY, resp, token);
}
