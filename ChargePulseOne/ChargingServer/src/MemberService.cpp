#include "MemberService.h"
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

void handleMemberLevels(SOCKET client, const json& data, const std::string& token) {
    auto rows = DatabaseManager::instance().query("SELECT * FROM member_levels ORDER BY min_amount ASC");
    json resp = json::array();
    for(auto& r: rows) resp.push_back(r);
    response(client, CMD::MEMBER_LEVELS, resp, token);
}

void handleMemberStats(SOCKET client, const json& data, const std::string& token) {
    // 统计各等级人数
    auto rows = DatabaseManager::instance().query(
        "SELECT level, COUNT(*) as count FROM users WHERE role='driver' GROUP BY level"
    );
    json resp = json::array();
    for(auto& r: rows) resp.push_back(r);
    response(client, CMD::MEMBER_STATS, resp, token);
}
