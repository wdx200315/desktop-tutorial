#include "ReservationService.h"
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

// 统一分发：根据 action 调用不同子功能
void handleReserveDispatch(SOCKET client, const json& data, const std::string& token) {
    std::string action = data.value("action", "list");
    if(action == "create") handleReserveCreate(client, data, token);
    else if(action == "cancel") handleReserveCancel(client, data, token);
    else handleReserveList(client, data, token);
}

// 创建预约
void handleReserveCreate(SOCKET client, const json& data, const std::string& token) {
    Session sess;
    if(!SessionManager::instance().validate(token, sess)) {
        error(client, CMD::RESERVE, "Authentication required");
        return;
    }
    int chargerId = data.value("charger_id", 0);
    std::string reserveTime = data.value("reserve_time", "");
    std::string mode = data.value("mode", "auto");
    double target = data.value("target", 0.0);
    if(chargerId <= 0 || reserveTime.empty()) {
        error(client, CMD::RESERVE, "charger_id and reserve_time required");
        return;
    }
    // 检查桩是否在线
    auto ch = DatabaseManager::instance().query("SELECT status FROM chargers WHERE id=" + std::to_string(chargerId));
    if(ch.empty() || ch[0]["status"] != "online") {
        error(client, CMD::RESERVE, "Charger not available");
        return;
    }
    // 插入预约
    std::string sql = "INSERT INTO reservations (user_id, charger_id, reserve_time, mode, target_value) VALUES (" +
        std::to_string(sess.userId) + "," + std::to_string(chargerId) + ",'" + reserveTime + "','" + mode + "'," + std::to_string(target) + ")";
    if(DatabaseManager::instance().execute(sql) <= 0) {
        error(client, CMD::RESERVE, "Reservation failed");
        return;
    }
    int resId = DatabaseManager::instance().lastInsertId();
    json resp; resp["reservation_id"] = resId; resp["message"] = "Reservation created";
    response(client, CMD::RESERVE, resp, token);
}

// 列表：管理员可看全部，车主只看自己的
void handleReserveList(SOCKET client, const json& data, const std::string& token) {
    Session sess;
    if(!SessionManager::instance().validate(token, sess)) {
        error(client, CMD::RESERVE, "Authentication required");
        return;
    }
    int page = data.value("page", 1);
    int size = data.value("size", 20);
    std::string status = data.value("status", "");
    std::string where = "WHERE 1=1";
    // 普通用户只看自己的
    if(sess.role == "driver") where += " AND user_id=" + std::to_string(sess.userId);
    if(!status.empty()) where += " AND status='" + status + "'";
    auto totalRes = DatabaseManager::instance().query("SELECT COUNT(*) as cnt FROM reservations " + where);
    int total = totalRes.empty()?0:std::stoi(totalRes[0]["cnt"]);
    int offset = (page-1)*size;
    auto rows = DatabaseManager::instance().query(
        "SELECT * FROM reservations " + where + " ORDER BY id DESC LIMIT " + std::to_string(size) + " OFFSET " + std::to_string(offset)
    );
    json resp;
    resp["total"] = total;
    resp["list"] = json::array();
    for(auto& r: rows) resp["list"].push_back(r);
    response(client, CMD::RESERVE, resp, token);
}

// 取消预约（仅车主可取消自己的 pending 预约，管理员可取消任意）
void handleReserveCancel(SOCKET client, const json& data, const std::string& token) {
    Session sess;
    if(!SessionManager::instance().validate(token, sess)) {
        error(client, CMD::RESERVE, "Authentication required");
        return;
    }
    int resId = data.value("id", 0);
    if(resId <= 0) { error(client, CMD::RESERVE, "id required"); return; }
    // 查询预约
    auto res = DatabaseManager::instance().query("SELECT user_id, status FROM reservations WHERE id=" + std::to_string(resId));
    if(res.empty()) { error(client, CMD::RESERVE, "Reservation not found"); return; }
    // 权限：管理员可操作任何，普通用户只能取消自己的且状态为 pending
    if(sess.role != "admin" && sess.role != "operator") {
        if(std::stoi(res[0]["user_id"]) != sess.userId) {
            error(client, CMD::RESERVE, "Permission denied");
            return;
        }
        if(res[0]["status"] != "pending") {
            error(client, CMD::RESERVE, "Only pending reservations can be cancelled");
            return;
        }
    }
    DatabaseManager::instance().execute("UPDATE reservations SET status='cancelled' WHERE id=" + std::to_string(resId));
    response(client, CMD::RESERVE, {{"message","Cancelled"}}, token);
}
