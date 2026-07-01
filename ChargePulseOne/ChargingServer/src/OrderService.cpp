#include "OrderService.h"
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

void handleOrderList(SOCKET client, const json& data, const std::string& token) {
    Session sess;
    if(!SessionManager::instance().validate(token, sess)) {
        error(client, CMD::ORDER_LIST, "Authentication required"); return;
    }
    int page = data.value("page", 1);
    int size = data.value("size", 20);
    std::string status = data.value("status", "");
    std::string where = "WHERE 1=1";
    // 司机只能看自己的订单，管理员/运营商看全部
    if(sess.role == "driver") where += " AND user_id=" + std::to_string(sess.userId);
    if(!status.empty()) where += " AND status='" + status + "'";
    auto totalRes = DatabaseManager::instance().query("SELECT COUNT(*) as cnt FROM charging_orders " + where);
    int total = totalRes.empty() ? 0 : std::stoi(totalRes[0]["cnt"]);
    int offset = (page-1)*size;
    auto rows = DatabaseManager::instance().query(
        "SELECT * FROM charging_orders " + where + " ORDER BY id DESC LIMIT " + std::to_string(size) + " OFFSET " + std::to_string(offset)
    );
    json resp;
    resp["total"] = total;
    resp["page"] = page;
    resp["list"] = json::array();
    for(auto& r : rows) resp["list"].push_back(r);
    response(client, CMD::ORDER_LIST, resp, token);
}

void handleOrderDetail(SOCKET client, const json& data, const std::string& token) {
    Session sess;
    if(!SessionManager::instance().validate(token, sess)) {
        error(client, CMD::ORDER_DETAIL, "Authentication required"); return;
    }
    int orderId = data.value("id", 0);
    auto rows = DatabaseManager::instance().query("SELECT * FROM charging_orders WHERE id=" + std::to_string(orderId));
    if(rows.empty()) { error(client, CMD::ORDER_DETAIL, "Order not found"); return; }
    auto& order = rows[0];
    // 权限：司机只能看自己的
    if(sess.role == "driver" && std::stoi(order["user_id"]) != sess.userId) {
        error(client, CMD::ORDER_DETAIL, "Permission denied"); return;
    }
    response(client, CMD::ORDER_DETAIL, order, token);
}

// 导出为CSV格式（简化返回字符串）
void handleOrderExport(SOCKET client, const json& data, const std::string& token) {
    Session sess;
    if(!SessionManager::instance().validate(token, sess) || sess.role == "driver") {
        error(client, CMD::ORDER_EXPORT, "Permission denied"); return;
    }
    auto rows = DatabaseManager::instance().query("SELECT * FROM charging_orders");
    std::string csv = "id,user_id,charger_id,mode,start_time,end_time,status\n";
    for(auto& r : rows) {
        csv += r["id"] + "," + r["user_id"] + "," + r["charger_id"] + "," + r["mode"] + "," +
               r["start_time"] + "," + r["end_time"] + "," + r["status"] + "\n";
    }
    json resp;
    resp["csv"] = csv;
    response(client, CMD::ORDER_EXPORT, resp, token);
}
