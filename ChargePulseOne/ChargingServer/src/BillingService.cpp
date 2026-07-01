#include "BillingService.h"
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

// 费率列表
void handleRateList(SOCKET client, const json& data, const std::string& token) {
    auto rows = DatabaseManager::instance().query("SELECT * FROM rates ORDER BY id");
    json resp = json::array();
    for(auto& r : rows) resp.push_back(r);
    response(client, CMD::RATE_LIST, resp, token);
}

// 添加费率 (时段、电价、服务费)
void handleRateAdd(SOCKET client, const json& data, const std::string& token) {
    Session sess;
    if(!SessionManager::instance().validate(token, sess) || (sess.role != "admin" && sess.role != "operator")) {
        error(client, "rate_add", "Permission denied"); return;
    }
    std::string name = data.value("name", "标准费率");
    double price_kwh = data.value("price_kwh", 1.0);
    double service_fee = data.value("service_fee", 0.5);
    std::string start_time = data.value("start_time", "00:00");
    std::string end_time = data.value("end_time", "23:59");
    // 检查时段重叠（简化）
    DatabaseManager::instance().execute(
        "INSERT INTO rates (name, price_kwh, service_fee, start_time, end_time) VALUES ('" +
        name + "'," + std::to_string(price_kwh) + "," + std::to_string(service_fee) + ",'" +
        start_time + "','" + end_time + "')");
    response(client, CMD::RATE_LIST, {{"message","Rate added"}}, token);
}

void handleRateUpdate(SOCKET client, const json& data, const std::string& token) {
    Session sess;
    if(!SessionManager::instance().validate(token, sess) || (sess.role != "admin" && sess.role != "operator")) {
        error(client, "rate_update", "Permission denied"); return;
    }
    int id = data.value("id",0);
    std::string sql = "UPDATE rates SET ";
    if(data.contains("price_kwh")) sql += "price_kwh=" + std::to_string(data["price_kwh"].get<double>()) + ",";
    if(data.contains("service_fee")) sql += "service_fee=" + std::to_string(data["service_fee"].get<double>()) + ",";
    if(data.contains("start_time")) sql += "start_time='" + data["start_time"].get<std::string>() + "',";
    if(data.contains("end_time")) sql += "end_time='" + data["end_time"].get<std::string>() + "',";
    sql.pop_back();
    sql += " WHERE id=" + std::to_string(id);
    DatabaseManager::instance().execute(sql);
    response(client, CMD::RATE_LIST, {{"message","Updated"}}, token);
}

void handleRateDelete(SOCKET client, const json& data, const std::string& token) {
    Session sess;
    if(!SessionManager::instance().validate(token, sess) || sess.role != "admin") {
        error(client, "rate_delete", "Permission denied"); return;
    }
    int id = data.value("id",0);
    DatabaseManager::instance().execute("DELETE FROM rates WHERE id=" + std::to_string(id));
    response(client, CMD::RATE_LIST, {{"message","Deleted"}}, token);
}

// 费用预估 (基于充电模式)
void handleFeeEstimate(SOCKET client, const json& data, const std::string& token) {
    double power = data.value("power_kw", 60.0);
    double hours = data.value("hours", 1.0);
    // 查当前时段费率（简化取第一条）
    auto rate = DatabaseManager::instance().query("SELECT price_kwh, service_fee FROM rates LIMIT 1");
    double price = 1.0, service = 0.5;
    if(!rate.empty()) {
        price = std::stod(rate[0]["price_kwh"]);
        service = std::stod(rate[0]["service_fee"]);
    }
    double energy = power * hours;
    double cost = energy * price + service; // 简化：服务费按小时计
    json resp;
    resp["energy_kwh"] = energy;
    resp["electricity_cost"] = energy * price;
    resp["service_cost"] = service;
    resp["total"] = cost;
    response(client, CMD::FEE_ESTIMATE, resp, token);
}

// 实时计费查询（订单进行中的费用）
void handleRealBilling(SOCKET client, const json& data, const std::string& token) {
    int orderId = data.value("order_id", 0);
    // 简化：从订单获取开始时间，计算时长，乘费率
    auto order = DatabaseManager::instance().query("SELECT start_time, charger_id FROM charging_orders WHERE id=" + std::to_string(orderId));
    if(order.empty()) { error(client, CMD::REAL_BILLING, "Order not found"); return; }
    // 假设即时算费（真正实现需记录累计）
    json resp;
    resp["order_id"] = orderId;
    resp["current_cost"] = 12.34; // 模拟
    response(client, CMD::REAL_BILLING, resp, token);
}
