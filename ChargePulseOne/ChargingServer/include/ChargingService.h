#ifndef CHARGING_SERVICE_H
#define CHARGING_SERVICE_H

#include <string>
#include <unordered_map>
#include <functional>

// 充电订单实时数据模拟
struct ChargeSimulator {
    int orderId;
    int chargerId;
    int userId;
    double initialEnergy;      // 初始电量
    double currentEnergy;       // 当前电量 (kWh)
    double power;               // 充电功率 (kW)
    double voltage;             // 当前电压 (V)
    double current;             // 当前电流 (A)
    double temperature;         // 充电桩温度 (℃)
    double soc;                 // 电池SOC (0-100%)
    double targetSoc;           // 目标SOC
    double feePerKwh;           // 每度电费
    double serviceFee;          // 服务费
    double startTime;           // 开始时间戳
    double lastUpdateTime;      // 上次更新时间
    bool running;               // 是否运行中
    std::string mode;           // 充电模式: auto, timed, soc
    std::string chargerSerial;  // 充电桩序列号
    
    // 计算当前实时费用
    double getCurrentFee() const {
        double energyFee = currentEnergy * feePerKwh;
        double svcFee = currentEnergy * serviceFee;
        return energyFee + svcFee;
    }
    
    // 计算充电时长(秒)
    int getDuration() const {
        return (int)((clock() / (double)CLOCKS_PER_SEC - startTime));
    }
    
    // 获取充电进度
    double getProgress() const {
        if (targetSoc <= 0) return 0;
        return std::min(100.0, (soc / targetSoc) * 100.0);
    }
};

// 全局充电模拟器实例
extern std::unordered_map<int, ChargeSimulator> g_chargeSimulators;
extern std::unordered_map<int, void*> g_orderClients;  // orderId -> client socket
extern std::unordered_map<int, void*> g_userClients;  // userId -> client socket

// 峰谷电价时段
struct PricePeriod {
    int startHour;    // 开始小时 (0-23)
    int endHour;      // 结束小时 (0-23)
    double price;     // 电价 (元/度)
    std::string name; // 时段名称: peak, normal, valley
};

// 获取当前时段电价
inline double getCurrentPrice() {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    int hour = timeinfo->tm_hour;
    
    // 峰谷电价配置
    static std::vector<PricePeriod> periods = {
        {0, 6, 0.3, "valley"},    // 谷时: 0:00-6:00
        {6, 9, 0.6, "normal"},    // 平段: 6:00-9:00
        {9, 12, 1.2, "peak"},     // 峰时: 9:00-12:00
        {12, 14, 0.8, "normal"},  // 平段: 12:00-14:00
        {14, 18, 1.2, "peak"},   // 峰时: 14:00-18:00
        {18, 21, 0.8, "normal"}, // 平段: 18:00-21:00
        {21, 24, 0.5, "valley"}  // 谷时: 21:00-24:00
    };
    
    for (const auto& p : periods) {
        if (hour >= p.startHour && hour < p.endHour) {
            return p.price;
        }
    }
    return 0.6; // 默认电价
}

// 获取时段名称
inline std::string getCurrentPeriodName() {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    int hour = timeinfo->tm_hour;
    
    if (hour >= 0 && hour < 6) return "谷时";
    if (hour >= 9 && hour < 12) return "峰时";
    if (hour >= 14 && hour < 18) return "峰时";
    return "平段";
}

// 更新充电模拟数据
void updateChargeSimulation();

// 充电控制函数
void handleStartCharge(void* client, const std::string& data, const std::string& token, void* serverClient);
void handleStopCharge(void* client, const std::string& data, const std::string& token);
void handleChargeMonitor(void* client, const std::string& data, const std::string& token);

#endif // CHARGING_SERVICE_H
