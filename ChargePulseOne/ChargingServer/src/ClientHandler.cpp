#include "ClientHandler.h"
#include "Logger.h"
#include "SessionManager.h"
#include "UserService.h"
#include "ChargerService.h"
#include "ChargingService.h"
#include "BillingService.h"
#include "OrderService.h"
#include "AlarmService.h"
#include "ReportService.h"
#include "ConfigService.h"
#include "LogService.h"
#include "ReservationService.h"
#include "PaymentService.h"
#include "CouponService.h"
#include "MemberService.h"
#include "MessageService.h"
#include "AnalyticsService.h"

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

// ---------- 注意：不再定义 handleReserveDispatch，它已在 ReservationService.h 中声明 ----------
// Payment 统一分发也已移至 PaymentService.cpp，此处不再重复定义

ClientHandler::ClientHandler(TcpServer& srv) : server(srv) {
    server.setMessageCallback([this](SOCKET s, const std::string& msg) {
        onMessage(s, msg);
    });
    server.setConnectCallback([this](SOCKET s) { onConnect(s); });
    server.setDisconnectCallback([this](SOCKET s) { onDisconnect(s); });
    registerHandlers();
}

void ClientHandler::registerHandlers() {
    // 用户认证
    handlers[CMD::LOGIN]          = handleLogin;
    handlers[CMD::REGISTER]       = handleRegister;
    handlers[CMD::CHANGE_PWD]     = handleChangePassword;
    handlers[CMD::RESET_PWD]      = handleResetPassword;
    handlers[CMD::USER_INFO]      = handleUserInfo;
    handlers[CMD::UPDATE_USER]    = handleUpdateUser;
    handlers[CMD::USER_LIST]      = handleUserList;
    handlers[CMD::USER_EDIT]      = handleUserEdit;
    handlers[CMD::VEHICLE_ADD]    = handleVehicleAdd;
    handlers[CMD::VEHICLE_LIST]   = handleVehicleList;
    handlers[CMD::VEHICLE_DEL]    = handleVehicleDelete;

    // 充电桩
    handlers[CMD::CHARGER_LIST]   = handleChargerList;
    handlers[CMD::CHARGER_DETAIL] = handleChargerDetail;
    handlers[CMD::CHARGER_CTRL]   = handleChargerControl;

    // 充电控制
    handlers[CMD::START_CHARGE]   = handleStartCharge;
    handlers[CMD::STOP_CHARGE]    = handleStopCharge;
    handlers[CMD::CHARGE_MONITOR] = handleChargeMonitor;

    // 预约（直接使用 ReservationService.h 中的函数）
    handlers[CMD::RESERVE]        = handleReserveDispatch;

    // 计费
    handlers[CMD::RATE_LIST]      = handleRateList;
    handlers[CMD::FEE_ESTIMATE]   = handleFeeEstimate;
    handlers[CMD::REAL_BILLING]   = handleRealBilling;

    // 订单
    handlers[CMD::ORDER_LIST]     = handleOrderList;
    handlers[CMD::ORDER_DETAIL]   = handleOrderDetail;
    handlers[CMD::ORDER_EXPORT]   = handleOrderExport;

    // 支付（直接使用 PaymentService.h 中的函数）
    handlers[CMD::PAYMENT]        = handlePaymentDispatch;

    // 告警
    handlers[CMD::ALARM_LIST]     = handleAlarmList;
    handlers[CMD::ALARM_HANDLE]   = handleAlarmHandle;

    // 报表
    handlers[CMD::REPORT_DAILY]   = handleReportDaily;
    handlers[CMD::REPORT_MONTHLY] = handleReportMonthly;

    // 消息
    handlers[CMD::MESSAGE_LIST]   = handleMessageList;
    handlers[CMD::MESSAGE_SEND]   = handleMessageSend;
    handlers[CMD::MESSAGE_READ]   = handleMessageRead;
    handlers[CMD::MESSAGE_DELETE] = handleMessageDelete;

    // 优惠券
    handlers[CMD::COUPON_LIST]      = handleCouponList;
    handlers[CMD::COUPON_ADD]       = handleCouponAdd;
    handlers[CMD::COUPON_EDIT]      = handleCouponEdit;
    handlers[CMD::COUPON_DELETE]    = handleCouponDelete;
    handlers[CMD::COUPON_CLAIM]     = handleCouponClaim;
    handlers[CMD::COUPON_USER_LIST] = handleCouponUserList;
    handlers[CMD::COUPON_USE]       = handleCouponUse;

    // 会员
    handlers[CMD::MEMBER_LEVELS]   = handleMemberLevels;
    handlers[CMD::MEMBER_STATS]    = handleMemberStats;

    // 系统配置
    handlers[CMD::SYS_CONFIG_GET]  = handleConfigGet;
    handlers[CMD::SYS_CONFIG_SET]  = handleConfigSet;

    // 操作日志
    handlers[CMD::LOG_QUERY]       = handleLogQuery;

    // 数据分析
    handlers[CMD::ANALYTICS_OVERVIEW] = handleAnalyticsOverview;
    handlers[CMD::ANALYTICS_REVENUE]  = handleAnalyticsRevenue;
    handlers[CMD::ANALYTICS_CHARGER] = handleAnalyticsChargerStats;
    handlers[CMD::USER_STATS]        = handleUserStatistics;
}

void ClientHandler::onMessage(SOCKET client, const std::string& raw) {
    std::string cmd, token;
    json data;
    if (!ProtocolParser::parse(raw, cmd, data, token)) {
        error(client, "", "Invalid JSON");
        return;
    }
    Logger::instance().log(DEBUG, "Received cmd: " + cmd);

    // 无需认证的命令
    if (cmd != CMD::LOGIN && cmd != CMD::REGISTER && cmd != CMD::RESET_PWD) {
        Session sess;
        if (!SessionManager::instance().validate(token, sess)) {
            error(client, cmd, "Authentication required or session expired");
            return;
        }
        SessionManager::instance().updateActivity(token);
    }

    auto it = handlers.find(cmd);
    if (it != handlers.end()) {
        it->second(client, data, token);
    } else {
        error(client, cmd, "Unknown command");
    }
}

void ClientHandler::onConnect(SOCKET client) {
    Logger::instance().log(INFO, "Client connected");
}

void ClientHandler::onDisconnect(SOCKET client) {
    Logger::instance().log(INFO, "Client disconnected");
}
