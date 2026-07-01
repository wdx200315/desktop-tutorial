#include "AnalyticsService.h"
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

void handleAnalyticsOverview(SOCKET client, const json& data, const std::string& token) {
    Session sess;
    if(!SessionManager::instance().validate(token, sess) || (sess.role != "admin" && sess.role != "operator")) {
        error(client, CMD::ANALYTICS_OVERVIEW, "Permission denied"); return;
    }

    json resp;

    // 今日充电次数
    auto todayCount = DatabaseManager::instance().query(
        "SELECT COUNT(*) as cnt FROM charging_orders WHERE DATE(start_time) = CURDATE()"
    );
    resp["today_charge_count"] = todayCount.empty() ? 0 : std::stoi(todayCount[0]["cnt"]);

    // 今日充电量
    auto todayEnergy = DatabaseManager::instance().query(
        "SELECT COALESCE(SUM(energy_kwh), 0) as total FROM charging_orders WHERE DATE(start_time) = CURDATE()"
    );
    resp["today_energy"] = todayEnergy.empty() ? 0 : std::stod(todayEnergy[0]["total"]);

    // 今日营收
    auto todayRevenue = DatabaseManager::instance().query(
        "SELECT COALESCE(SUM(amount), 0) as total FROM charging_orders WHERE DATE(start_time) = CURDATE() AND status='completed'"
    );
    resp["today_revenue"] = todayRevenue.empty() ? 0 : std::stod(todayRevenue[0]["total"]);

    // 在线充电桩数
    auto onlineChargers = DatabaseManager::instance().query(
        "SELECT COUNT(*) as cnt FROM chargers WHERE status='online'"
    );
    resp["online_chargers"] = onlineChargers.empty() ? 0 : std::stoi(onlineChargers[0]["cnt"]);

    // 总充电桩数
    auto totalChargers = DatabaseManager::instance().query("SELECT COUNT(*) as cnt FROM chargers");
    resp["total_chargers"] = totalChargers.empty() ? 0 : std::stoi(totalChargers[0]["cnt"]);

    // 总用户数
    auto totalUsers = DatabaseManager::instance().query("SELECT COUNT(*) as cnt FROM users WHERE role='driver'");
    resp["total_users"] = totalUsers.empty() ? 0 : std::stoi(totalUsers[0]["cnt"]);

    // 本月充电次数
    auto monthCount = DatabaseManager::instance().query(
        "SELECT COUNT(*) as cnt FROM charging_orders WHERE YEAR(start_time)=YEAR(CURDATE()) AND MONTH(start_time)=MONTH(CURDATE())"
    );
    resp["month_charge_count"] = monthCount.empty() ? 0 : std::stoi(monthCount[0]["cnt"]);

    // 本月营收
    auto monthRevenue = DatabaseManager::instance().query(
        "SELECT COALESCE(SUM(amount), 0) as total FROM charging_orders WHERE YEAR(start_time)=YEAR(CURDATE()) AND MONTH(start_time)=MONTH(CURDATE()) AND status='completed'"
    );
    resp["month_revenue"] = monthRevenue.empty() ? 0 : std::stod(monthRevenue[0]["total"]);

    // 在线用户数
    resp["online_users"] = SessionManager::instance().getActiveCount();

    response(client, CMD::ANALYTICS_OVERVIEW, resp, token);
}

void handleAnalyticsRevenue(SOCKET client, const json& data, const std::string& token) {
    Session sess;
    if(!SessionManager::instance().validate(token, sess) || (sess.role != "admin" && sess.role != "operator")) {
        error(client, CMD::ANALYTICS_REVENUE, "Permission denied"); return;
    }

    std::string period = data.value("period", "daily");
    json resp;
    json revenueData = json::array();

    if (period == "daily") {
        // 最近7天营收
        auto rows = DatabaseManager::instance().query(
            "SELECT DATE(start_time) as date, SUM(amount) as revenue, COUNT(*) as count "
            "FROM charging_orders WHERE start_time >= DATE_SUB(CURDATE(), INTERVAL 7 DAY) AND status='completed' "
            "GROUP BY DATE(start_time) ORDER BY date"
        );
        for (auto& r : rows) {
            json item;
            item["date"] = r["date"];
            item["revenue"] = std::stod(r["revenue"]);
            item["count"] = std::stoi(r["count"]);
            revenueData.push_back(item);
        }
    } else if (period == "monthly") {
        // 最近6个月营收
        auto rows = DatabaseManager::instance().query(
            "SELECT DATE_FORMAT(start_time, '%Y-%m') as month, SUM(amount) as revenue, COUNT(*) as count "
            "FROM charging_orders WHERE start_time >= DATE_SUB(CURDATE(), INTERVAL 6 MONTH) AND status='completed' "
            "GROUP BY DATE_FORMAT(start_time, '%Y-%m') ORDER BY month"
        );
        for (auto& r : rows) {
            json item;
            item["month"] = r["month"];
            item["revenue"] = std::stod(r["revenue"]);
            item["count"] = std::stoi(r["count"]);
            revenueData.push_back(item);
        }
    }

    resp["period"] = period;
    resp["data"] = revenueData;
    response(client, CMD::ANALYTICS_REVENUE, resp, token);
}

void handleAnalyticsChargerStats(SOCKET client, const json& data, const std::string& token) {
    Session sess;
    if(!SessionManager::instance().validate(token, sess) || (sess.role != "admin" && sess.role != "operator")) {
        error(client, CMD::ANALYTICS_CHARGER, "Permission denied"); return;
    }

    // 各状态充电桩统计
    auto rows = DatabaseManager::instance().query(
        "SELECT status, COUNT(*) as count FROM chargers GROUP BY status"
    );

    json resp;
    json statusData = json::object();
    for (auto& r : rows) {
        statusData[r["status"]] = std::stoi(r["count"]);
    }
    resp["status_distribution"] = statusData;

    // 使用率最高的充电桩
    auto topChargers = DatabaseManager::instance().query(
        "SELECT c.id, c.serial_number, COUNT(o.id) as charge_count, SUM(o.energy_kwh) as total_energy "
        "FROM chargers c LEFT JOIN charging_orders o ON c.id=o.charger_id AND o.status='completed' "
        "GROUP BY c.id ORDER BY charge_count DESC LIMIT 5"
    );
    json topList = json::array();
    for (auto& r : topChargers) {
        json item;
        item["id"] = std::stoi(r["id"]);
        item["serial_number"] = r["serial_number"];
        item["charge_count"] = std::stoi(r["charge_count"]);
        item["total_energy"] = r["total_energy"].empty() ? 0 : std::stod(r["total_energy"]);
        topList.push_back(item);
    }
    resp["top_chargers"] = topList;

    response(client, CMD::ANALYTICS_CHARGER, resp, token);
}

void handleUserStatistics(SOCKET client, const json& data, const std::string& token) {
    Session sess;
    if(!SessionManager::instance().validate(token, sess)) {
        error(client, CMD::USER_STATS, "Authentication required"); return;
    }

    json resp;

    // 用户充电统计
    auto stats = DatabaseManager::instance().query(
        "SELECT COUNT(*) as total_orders, COALESCE(SUM(energy_kwh), 0) as total_energy, COALESCE(SUM(amount), 0) as total_spent "
        "FROM charging_orders WHERE user_id=" + std::to_string(sess.userId)
    );

    if (!stats.empty()) {
        resp["total_orders"] = std::stoi(stats[0]["total_orders"]);
        resp["total_energy"] = std::stod(stats[0]["total_energy"]);
        resp["total_spent"] = std::stod(stats[0]["total_spent"]);
    }

    // 最近7天充电统计
    auto weekStats = DatabaseManager::instance().query(
        "SELECT COUNT(*) as cnt, COALESCE(SUM(energy_kwh), 0) as energy "
        "FROM charging_orders WHERE user_id=" + std::to_string(sess.userId) +
        " AND start_time >= DATE_SUB(CURDATE(), INTERVAL 7 DAY)"
    );

    if (!weekStats.empty()) {
        resp["week_charge_count"] = std::stoi(weekStats[0]["cnt"]);
        resp["week_energy"] = std::stod(weekStats[0]["energy"]);
    }

    response(client, CMD::USER_STATS, resp, token);
}
