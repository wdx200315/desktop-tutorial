#include "ChargerService.h"
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
    json msg;
    msg["cmd"] = cmd;
    msg["status"] = "ok";
    msg["data"] = data;
    if (!token.empty()) msg["token"] = token;
    std::string raw = msg.dump();
    send(client, (raw + "\n").c_str(), raw.size()+1, 0);
}

static void error(SOCKET client, const std::string& cmd, const std::string& message) {
    json msg;
    msg["cmd"] = cmd;
    msg["status"] = "error";
    msg["message"] = message;
    std::string raw = msg.dump();
    send(client, (raw + "\n").c_str(), raw.size()+1, 0);
}

// 辅助函数：验证管理员/操作员权限
static bool checkRole(const std::string& token, const std::vector<std::string>& allowed) {
    Session sess;
    if (!SessionManager::instance().validate(token, sess)) return false;
    for (auto& r : allowed) if (sess.role == r) return true;
    return false;
}

void handleChargerList(SOCKET client, const json& data, const std::string& token) {
    // 分页参数
    int page = data.value("page", 1);
    int size = data.value("size", 20);
    std::string status = data.value("status", "");
    std::string keyword = data.value("keyword", "");

    std::string where = "WHERE 1=1";
    if (!status.empty()) where += " AND status='" + status + "'";
    if (!keyword.empty()) where += " AND (serial_number LIKE '%" + keyword + "%' OR location LIKE '%" + keyword + "%')";

    auto totalRes = DatabaseManager::instance().query("SELECT COUNT(*) as cnt FROM chargers " + where);
    int total = totalRes.empty() ? 0 : std::stoi(totalRes[0]["cnt"]);

    int offset = (page - 1) * size;
    auto rows = DatabaseManager::instance().query(
        "SELECT id, serial_number, status, power_kw, temperature, location, health_score, last_heartbeat FROM chargers " +
        where + " ORDER BY id LIMIT " + std::to_string(size) + " OFFSET " + std::to_string(offset)
    );

    json resp;
    resp["total"] = total;
    resp["page"] = page;
    resp["list"] = json::array();
    for (auto& r : rows) {
        json item;
        item["id"] = r["id"];
        item["serial_number"] = r["serial_number"];
        item["status"] = r["status"];
        item["power_kw"] = r["power_kw"];
        item["temperature"] = r["temperature"];
        item["location"] = r["location"];
        item["health_score"] = r["health_score"];
        item["last_heartbeat"] = r["last_heartbeat"];
        resp["list"].push_back(item);
    }
    response(client, CMD::CHARGER_LIST, resp, token);
}

void handleChargerDetail(SOCKET client, const json& data, const std::string& token) {
    int chargerId = data.value("id", 0);
    if (chargerId <= 0) {
        error(client, CMD::CHARGER_DETAIL, "Invalid charger id");
        return;
    }
    auto rows = DatabaseManager::instance().query(
        "SELECT * FROM chargers WHERE id=" + std::to_string(chargerId)
    );
    if (rows.empty()) {
        error(client, CMD::CHARGER_DETAIL, "Charger not found");
        return;
    }
    json resp = rows[0];
    response(client, CMD::CHARGER_DETAIL, resp, token);
}

void handleChargerControl(SOCKET client, const json& data, const std::string& token) {
    if (!checkRole(token, {"operator", "admin"})) {
        error(client, CMD::CHARGER_CTRL, "Permission denied");
        return;
    }
    int chargerId = data.value("id", 0);
    std::string command = data.value("command", ""); // start, stop, restart, lock, unlock
    if (chargerId <= 0 || command.empty()) {
        error(client, CMD::CHARGER_CTRL, "Missing id or command");
        return;
    }
    // 模拟执行（实际应与硬件通信）
    Logger::instance().log(INFO, "Control command " + command + " sent to charger " + std::to_string(chargerId));
    json resp;
    resp["message"] = "Command " + command + " executed";
    response(client, CMD::CHARGER_CTRL, resp, token);
}

void handleChargerAdd(SOCKET client, const json& data, const std::string& token) {
    if (!checkRole(token, {"operator", "admin"})) {
        error(client, CMD::CHARGER_CTRL, "Permission denied");
        return;
    }
    std::string serial = data.value("serial_number", "");
    double power = data.value("power_kw", 0.0);
    std::string location = data.value("location", "");
    if (serial.empty()) {
        error(client, CMD::CHARGER_CTRL, "Serial number required");
        return;
    }
    std::string sql = "INSERT INTO chargers (serial_number, power_kw, location) VALUES ('" +
        serial + "'," + std::to_string(power) + ",'" + location + "')";
    if (DatabaseManager::instance().execute(sql) <= 0) {
        error(client, CMD::CHARGER_CTRL, "Failed to add charger");
        return;
    }
    json resp;
    resp["id"] = DatabaseManager::instance().lastInsertId();
    response(client, CMD::CHARGER_CTRL, resp, token);
}

void handleChargerUpdate(SOCKET client, const json& data, const std::string& token) {
    if (!checkRole(token, {"operator", "admin"})) {
        error(client, CMD::CHARGER_CTRL, "Permission denied");
        return;
    }
    int id = data.value("id", 0);
    if (id <= 0) { error(client, CMD::CHARGER_CTRL, "Id required"); return; }
    std::string sql = "UPDATE chargers SET ";
    if (data.contains("power_kw")) sql += "power_kw=" + std::to_string(data["power_kw"].get<double>()) + ",";
    if (data.contains("location")) sql += "location='" + data["location"].get<std::string>() + "',";
    if (data.contains("status")) sql += "status='" + data["status"].get<std::string>() + "',";
    sql.pop_back(); // 去掉最后的逗号
    sql += " WHERE id=" + std::to_string(id);
    DatabaseManager::instance().execute(sql);
    response(client, CMD::CHARGER_CTRL, {{"message", "Updated"}}, token);
}

void handleChargerDelete(SOCKET client, const json& data, const std::string& token) {
    if (!checkRole(token, {"admin"})) { // 只有管理员能删除
        error(client, CMD::CHARGER_CTRL, "Permission denied");
        return;
    }
    int id = data.value("id", 0);
    DatabaseManager::instance().execute("DELETE FROM chargers WHERE id=" + std::to_string(id));
    response(client, CMD::CHARGER_CTRL, {{"message", "Deleted"}}, token);
}
