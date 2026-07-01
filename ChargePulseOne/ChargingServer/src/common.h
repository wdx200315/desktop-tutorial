#pragma once
#include <string>
#include <cstdint>
#include <map>
#include <vector>
#include <functional>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <queue>
#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <random>
#include <algorithm>
#include <stdexcept>
#include <cstring>
#include <atomic>

// ===== 协议命令码 =====
namespace CMD {
    // 用户认证
    const std::string LOGIN           = "1001";
    const std::string REGISTER        = "1002";
    const std::string CHANGE_PWD      = "1003";
    const std::string RESET_PWD       = "1004";
    const std::string USER_INFO       = "1005";
    const std::string UPDATE_USER     = "1006";
    const std::string USER_LIST       = "1007";
    const std::string USER_EDIT       = "1008";
    // 车辆
    const std::string VEHICLE_ADD     = "1009";
    const std::string VEHICLE_LIST    = "1010";
    const std::string VEHICLE_DEL     = "1011";

    // 充电桩
    const std::string CHARGER_LIST    = "2001";
    const std::string CHARGER_DETAIL  = "2002";
    const std::string CHARGER_CTRL    = "2003";

    // 充电控制
    const std::string START_CHARGE    = "3001";
    const std::string STOP_CHARGE     = "3002";
    const std::string CHARGE_MONITOR  = "3003";
    const std::string RESERVE         = "3004";

    // 计费
    const std::string RATE_LIST       = "4001";
    const std::string FEE_ESTIMATE    = "4002";
    const std::string REAL_BILLING    = "4003";
    const std::string PAYMENT         = "4004";

    // 订单
    const std::string ORDER_LIST      = "5001";
    const std::string ORDER_DETAIL    = "5002";
    const std::string ORDER_EXPORT    = "5003";

    // 告警
    const std::string ALARM_LIST      = "6001";
    const std::string ALARM_HANDLE    = "6002";

    // 报表
    const std::string REPORT_DAILY    = "7001";
    const std::string REPORT_MONTHLY  = "7002";

    // 系统
    const std::string SYS_CONFIG_GET  = "8001";
    const std::string SYS_CONFIG_SET  = "8002";
    const std::string LOG_QUERY       = "8003";
    const std::string HEARTBEAT       = "9999";

    // 数据分析
    const std::string ANALYTICS_OVERVIEW   = "8004";
    const std::string ANALYTICS_REVENUE    = "8005";
    const std::string ANALYTICS_CHARGER   = "8006";
    const std::string USER_STATS           = "8007";

    // 消息中心
    const std::string MESSAGE_LIST    = "9001";
    const std::string MESSAGE_SEND    = "9002";
    const std::string MESSAGE_READ    = "9003";
    const std::string MESSAGE_DELETE  = "9004";

    // 优惠券
    const std::string COUPON_LIST      = "9101";
    const std::string COUPON_ADD       = "9102";
    const std::string COUPON_EDIT      = "9103";
    const std::string COUPON_DELETE    = "9104";
    const std::string COUPON_CLAIM     = "9105";
    const std::string COUPON_USER_LIST = "9106";
    const std::string COUPON_USE       = "9107";

    // 会员
    const std::string MEMBER_LEVELS   = "9201";
    const std::string MEMBER_STATS    = "9202";
}

// 跨平台 sleep
inline void sleep_ms(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// 时间戳
inline std::string timestamp() {
    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&tt);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

// 随机字符串
inline std::string random_str(int len) {
    static const char charset[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    static std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> dist(0, sizeof(charset)-2);
    std::string s;
    for(int i=0;i<len;++i) s += charset[dist(rng)];
    return s;
}
