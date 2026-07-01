#include "ReportService.h"
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

// 日报表（统计指定日期）
void handleReportDaily(SOCKET client, const json& data, const std::string& token) {
    std::string date = data.value("date", "");
    if(date.empty()) {
        error(client, CMD::REPORT_DAILY, "Date required (YYYY-MM-DD)");
        return;
    }
    // 总订单、总电量、总收入、告警次数
    auto orders = DatabaseManager::instance().query(
        "SELECT COUNT(*) as total_orders, COALESCE(SUM(energy_kwh),0) as total_energy, COALESCE(SUM(amount),0) as total_revenue "
        "FROM charging_orders WHERE DATE(start_time)='" + date + "'"
    );
    auto alarms = DatabaseManager::instance().query(
        "SELECT COUNT(*) as total_alarms FROM alarms WHERE DATE(created_at)='" + date + "'"
    );
    json resp;
    if(!orders.empty()) {
        resp["total_orders"] = orders[0]["total_orders"];
        resp["total_energy"] = orders[0]["total_energy"];
        resp["total_revenue"] = orders[0]["total_revenue"];
    } else {
        resp["total_orders"] = 0;
        resp["total_energy"] = 0;
        resp["total_revenue"] = 0;
    }
    resp["total_alarms"] = alarms.empty() ? "0" : alarms[0]["total_alarms"];
    response(client, CMD::REPORT_DAILY, resp, token);
}

// 月报表
void handleReportMonthly(SOCKET client, const json& data, const std::string& token) {
    std::string month = data.value("month", ""); // YYYY-MM
    if(month.empty()) {
        error(client, CMD::REPORT_MONTHLY, "Month required (YYYY-MM)");
        return;
    }
    auto orders = DatabaseManager::instance().query(
        "SELECT COUNT(*) as total_orders, COALESCE(SUM(energy_kwh),0) as total_energy, COALESCE(SUM(amount),0) as total_revenue "
        "FROM charging_orders WHERE DATE_FORMAT(start_time,'%Y-%m')='" + month + "'"
    );
    auto alarms = DatabaseManager::instance().query(
        "SELECT COUNT(*) as total_alarms FROM alarms WHERE DATE_FORMAT(created_at,'%Y-%m')='" + month + "'"
    );
    json resp;
    if(!orders.empty()) {
        resp["total_orders"] = orders[0]["total_orders"];
        resp["total_energy"] = orders[0]["total_energy"];
        resp["total_revenue"] = orders[0]["total_revenue"];
    } else {
        resp["total_orders"] = 0;
        resp["total_energy"] = 0;
        resp["total_revenue"] = 0;
    }
    resp["total_alarms"] = alarms.empty() ? "0" : alarms[0]["total_alarms"];
    response(client, CMD::REPORT_MONTHLY, resp, token);
}
