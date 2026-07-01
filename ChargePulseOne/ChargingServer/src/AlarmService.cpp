#include "AlarmService.h"
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

// 告警列表（支持筛选、分页）
void handleAlarmList(SOCKET client, const json& data, const std::string& token) {
    int page = data.value("page", 1);
    int size = data.value("size", 20);
    std::string level = data.value("level", "");  // info/warning/critical
    std::string status = data.value("status", ""); // active/resolved

    std::string where = "WHERE 1=1";
    if(!level.empty()) where += " AND level='" + level + "'";
    if(!status.empty()) where += " AND status='" + status + "'";

    auto totalRes = DatabaseManager::instance().query("SELECT COUNT(*) as cnt FROM alarms " + where);
    int total = totalRes.empty() ? 0 : std::stoi(totalRes[0]["cnt"]);

    int offset = (page - 1) * size;
    auto rows = DatabaseManager::instance().query(
        "SELECT id, charger_id, type, level, message, status, created_at FROM alarms " +
        where + " ORDER BY id DESC LIMIT " + std::to_string(size) + " OFFSET " + std::to_string(offset)
    );

    json resp;
    resp["total"] = total;
    resp["page"] = page;
    resp["list"] = json::array();
    for(auto& r : rows) resp["list"].push_back(r);
    response(client, CMD::ALARM_LIST, resp, token);
}

// 人工处理告警
void handleAlarmHandle(SOCKET client, const json& data, const std::string& token) {
    Session sess;
    if(!SessionManager::instance().validate(token, sess) || (sess.role != "admin" && sess.role != "operator")) {
        error(client, CMD::ALARM_HANDLE, "Permission denied");
        return;
    }
    int alarmId = data.value("id", 0);
    std::string note = data.value("note", "");
    DatabaseManager::instance().execute(
        "UPDATE alarms SET status='resolved', resolved_at=NOW(), note='" + note + "' WHERE id=" + std::to_string(alarmId)
    );
    response(client, CMD::ALARM_HANDLE, {{"message", "Alarm handled"}}, token);
}

// 自动恢复（模拟系统操作）
void handleAlarmAutoRecover(SOCKET client, const json& data, const std::string& token) {
    Session sess;
    if(!SessionManager::instance().validate(token, sess) || sess.role != "admin") {
        error(client, CMD::ALARM_HANDLE, "Permission denied");
        return;
    }
    int alarmId = data.value("id", 0);
    DatabaseManager::instance().execute(
        "UPDATE alarms SET status='resolved', resolved_at=NOW(), note='Auto-recovered by system' WHERE id=" + std::to_string(alarmId)
    );
    response(client, CMD::ALARM_HANDLE, {{"message", "Auto-recovered"}}, token);
}
