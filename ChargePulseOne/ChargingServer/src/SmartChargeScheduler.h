#pragma once
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include "json.hpp"

using json = nlohmann::json;

/**
 * SmartChargeScheduler - 智能充电调度系统
 * 
 * 核心功能：
 * 1. 峰谷电价优化 - 根据时段自动调整充电功率
 * 2. 电网负载均衡 - 避免充电桩过载
 * 3. 用户等待时间优化 - 公平调度
 * 4. 充电预测 - 估算充电时间和费用
 */
class SmartChargeScheduler {
public:
    // 充电时段类型
    enum class TimePeriod {
        Valley,      // 谷时 (00:00-06:00) - 电价最低
        Normal,      // 平时 (06:00-12:00, 18:00-24:00)
        Peak         // 峰时 (12:00-18:00) - 电价最高
    };

    // 充电计划
    struct ChargePlan {
        int userId;
        int chargerId;
        int orderId;
        double targetEnergy;      // 目标电量 (度)
        double targetSoc;         // 目标SOC (0-100%)
        TimePeriod period;        // 当前时段
        double currentPower;      // 当前功率 (kW)
        double pricePerKwh;       // 当前电价 (元/度)
        double estimatedTime;     // 预估时间 (小时)
        double estimatedCost;     // 预估费用 (元)
        int delayMinutes;        // 建议延迟分钟数
        std::string reason;       // 调度原因
    };

    // 电价配置
    struct PriceConfig {
        double valleyPrice;   // 谷时电价
        double normalPrice;   // 平时电价
        double peakPrice;     // 峰时电价
        double serviceFee;    // 服务费
    };

    // 电网负载信息
    struct GridLoad {
        int chargerId;
        double currentLoad;   // 当前负载 (0-100%)
        int activeSessions;   // 活跃会话数
    };

    SmartChargeScheduler();
    
    // 设置电价配置
    void setPriceConfig(const PriceConfig& config);
    
    // 获取当前时段
    TimePeriod getCurrentPeriod() const;
    
    // 获取当前电价
    double getCurrentPrice() const;
    
    // 获取电价时段名称
    std::string getPeriodName(TimePeriod period) const;
    
    // 创建智能充电计划
    ChargePlan createChargePlan(int userId, int chargerId, double targetSoc);
    
    // 创建预约充电计划
    ChargePlan createReservationPlan(int userId, int chargerId, 
                                      const std::chrono::system_clock::time_point& reserveTime,
                                      double targetEnergy);
    
    // 更新电网负载
    void updateGridLoad(int chargerId, double loadPercent);
    
    // 获取推荐功率
    double getRecommendedPower(int chargerId, double targetSoc) const;
    
    // 计算充电费用
    double calculateCost(double energyKwh, TimePeriod period) const;
    
    // 估算充电时间
    double estimateChargeTime(double energyKwh, double powerKw) const;
    
    // 检查是否应该延迟充电
    bool shouldDelayCharge(int chargerId) const;
    
    // 获取最优充电开始时间
    std::chrono::system_clock::time_point getOptimalStartTime(double targetEnergy) const;

private:
    PriceConfig priceConfig_;
    std::map<int, GridLoad> gridLoads_;
    
    // 获取当前时间结构
    std::tm getCurrentTimeStruct() const;
    
    // 计算到目标SOC需要的电量
    double calculateEnergyToTarget(double currentSoc, double targetSoc, double batteryCapacity) const;
};

// ============ 实现 ============

SmartChargeScheduler::SmartChargeScheduler() {
    // 默认电价配置 (元/度)
    priceConfig_.valleyPrice = 0.3;   // 谷时: 0.3元/度
    priceConfig_.normalPrice = 0.6;  // 平时: 0.6元/度
    priceConfig_.peakPrice = 1.2;    // 峰时: 1.2元/度
    priceConfig_.serviceFee = 0.1;   // 服务费: 0.1元/度
}

void SmartChargeScheduler::setPriceConfig(const PriceConfig& config) {
    priceConfig_ = config;
}

SmartChargeScheduler::TimePeriod SmartChargeScheduler::getCurrentPeriod() const {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm* timeInfo = std::localtime(&t);
    int hour = timeInfo->tm_hour;

    if (hour >= 0 && hour < 6) {
        return TimePeriod::Valley;  // 00:00-06:00 谷时
    } else if (hour >= 6 && hour < 12) {
        return TimePeriod::Normal;  // 06:00-12:00 平时
    } else if (hour >= 12 && hour < 18) {
        return TimePeriod::Peak;    // 12:00-18:00 峰时
    } else {
        return TimePeriod::Normal;  // 18:00-24:00 平时
    }
}

double SmartChargeScheduler::getCurrentPrice() const {
    TimePeriod period = getCurrentPeriod();
    double basePrice;
    
    switch (period) {
        case TimePeriod::Valley: basePrice = priceConfig_.valleyPrice; break;
        case TimePeriod::Peak:    basePrice = priceConfig_.peakPrice; break;
        default:                  basePrice = priceConfig_.normalPrice; break;
    }
    
    return basePrice + priceConfig_.serviceFee;
}

std::string SmartChargeScheduler::getPeriodName(TimePeriod period) const {
    switch (period) {
        case TimePeriod::Valley: return "谷时";
        case TimePeriod::Peak:    return "峰时";
        default:                  return "平时";
    }
}

double SmartChargeScheduler::calculateCost(double energyKwh, TimePeriod period) const {
    double basePrice;
    switch (period) {
        case TimePeriod::Valley: basePrice = priceConfig_.valleyPrice; break;
        case TimePeriod::Peak:    basePrice = priceConfig_.peakPrice; break;
        default:                  basePrice = priceConfig_.normalPrice; break;
    }
    return energyKwh * (basePrice + priceConfig_.serviceFee);
}

double SmartChargeScheduler::estimateChargeTime(double energyKwh, double powerKw) const {
    if (powerKw <= 0) return 0;
    return energyKwh / powerKw;
}

double SmartChargeScheduler::getRecommendedPower(int chargerId, double targetSoc) const {
    // 获取当前电网负载
    double currentLoad = 0;
    if (gridLoads_.count(chargerId)) {
        currentLoad = gridLoads_[chargerId].currentLoad;
    }

    // 根据电网负载和目标SOC调整功率
    double basePower = 60.0;  // 基础功率 60kW
    
    // 如果电网负载过高，降低功率
    if (currentLoad > 80) {
        basePower = 30.0;  // 降低到30kW
    } else if (currentLoad > 60) {
        basePower = 45.0;  // 降低到45kW
    }

    // 如果目标是慢充（目标SOC较低），可以使用较低功率
    if (targetSoc < 80) {
        basePower = std::min(basePower, 30.0);
    }

    return basePower;
}

bool SmartChargeScheduler::shouldDelayCharge(int chargerId) const {
    // 检查电网负载
    if (gridLoads_.count(chargerId)) {
        double load = gridLoads_[chargerId].currentLoad;
        if (load > 85) {
            return true;  // 电网负载过高，建议延迟
        }
    }

    // 检查当前是否是峰时
    TimePeriod period = getCurrentPeriod();
    if (period == TimePeriod::Peak) {
        double peakPrice = priceConfig_.peakPrice;
        double valleyPrice = priceConfig_.valleyPrice;
        // 如果峰谷价差超过2倍，建议延迟
        if (peakPrice > valleyPrice * 2) {
            return true;
        }
    }

    return false;
}

std::chrono::system_clock::time_point SmartChargeScheduler::getOptimalStartTime(double targetEnergy) const {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm* timeInfo = std::localtime(&t);
    int currentHour = timeInfo->tm_hour;

    // 如果当前是峰时(12:00-18:00)，建议延迟到谷时开始
    if (currentHour >= 12 && currentHour < 18) {
        // 计算到下一个谷时的时间
        // 谷时是00:00-06:00
        int hoursUntilValley;
        if (currentHour >= 18) {
            hoursUntilValley = 24 - currentHour;  // 到明天0点
        } else {
            hoursUntilValley = 6;  // 到今天6点
        }
        
        // 添加延迟时间到now
        return now + std::chrono::hours(hoursUntilValley);
    }

    // 如果当前负载过高，延迟1-2小时
    if (shouldDelayCharge(-1)) {
        return now + std::chrono::hours(1);
    }

    return now;
}

void SmartChargeScheduler::updateGridLoad(int chargerId, double loadPercent) {
    GridLoad load;
    load.chargerId = chargerId;
    load.currentLoad = std::min(100.0, std::max(0.0, loadPercent));
    
    if (gridLoads_.count(chargerId)) {
        load.activeSessions = gridLoads_[chargerId].activeSessions;
    } else {
        load.activeSessions = 0;
    }
    
    gridLoads_[chargerId] = load;
}

SmartChargeScheduler::ChargePlan SmartChargeScheduler::createChargePlan(
    int userId, int chargerId, double targetSoc) {
    
    ChargePlan plan;
    plan.userId = userId;
    plan.chargerId = chargerId;
    plan.targetSoc = targetSoc;
    plan.period = getCurrentPeriod();
    plan.pricePerKwh = getCurrentPrice();
    plan.currentPower = getRecommendedPower(chargerId, targetSoc);
    
    // 估算需要的电量 (假设电池容量60度)
    double batteryCapacity = 60.0;
    double currentSoc = 20.0;  // 假设当前SOC为20%
    plan.targetEnergy = calculateEnergyToTarget(currentSoc, targetSoc, batteryCapacity);
    
    // 估算充电时间和费用
    plan.estimatedTime = estimateChargeTime(plan.targetEnergy, plan.currentPower);
    plan.estimatedCost = calculateCost(plan.targetEnergy, plan.period);
    
    // 判断是否应该延迟
    if (shouldDelayCharge(chargerId)) {
        plan.delayMinutes = 120;  // 建议延迟2小时
        plan.reason = "当前为峰时电价，建议延迟到谷时充电可节省约" + 
                      std::to_string((int)(plan.estimatedCost * 0.5)) + "元";
    } else {
        plan.delayMinutes = 0;
        plan.reason = "当前电价合适，可以立即充电";
    }
    
    return plan;
}

SmartChargeScheduler::ChargePlan SmartChargeScheduler::createReservationPlan(
    int userId, int chargerId,
    const std::chrono::system_clock::time_point& reserveTime,
    double targetEnergy) {
    
    ChargePlan plan;
    plan.userId = userId;
    plan.chargerId = chargerId;
    plan.orderId = -1;
    plan.targetEnergy = targetEnergy;
    plan.targetSoc = 100;
    plan.currentPower = 60.0;
    
    // 计算预约时间属于哪个时段
    std::time_t t = std::chrono::system_clock::to_time_t(reserveTime);
    std::tm* timeInfo = std::localtime(&t);
    int hour = timeInfo->tm_hour;
    
    if (hour >= 0 && hour < 6) {
        plan.period = TimePeriod::Valley;
    } else if (hour >= 12 && hour < 18) {
        plan.period = TimePeriod::Peak;
    } else {
        plan.period = TimePeriod::Normal;
    }
    
    plan.pricePerKwh = calculateCost(1.0, plan.period);
    plan.estimatedTime = estimateChargeTime(targetEnergy, plan.currentPower);
    plan.estimatedCost = calculateCost(targetEnergy, plan.period);
    plan.delayMinutes = 0;
    
    // 根据时段给出建议
    switch (plan.period) {
        case TimePeriod::Valley:
            plan.reason = "预约在谷时，电价最优惠";
            break;
        case TimePeriod::Peak:
            plan.reason = "⚠️ 预约在峰时，电价较高";
            break;
        default:
            plan.reason = "预约在平时，电价适中";
            break;
    }
    
    return plan;
}

double SmartChargeScheduler::calculateEnergyToTarget(
    double currentSoc, double targetSoc, double batteryCapacity) const {
    
    if (targetSoc <= currentSoc) return 0;
    if (batteryCapacity <= 0) return 0;
    
    return (targetSoc - currentSoc) / 100.0 * batteryCapacity;
}

std::tm SmartChargeScheduler::getCurrentTimeStruct() const {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    return *std::localtime(&t);
}

// 辅助函数：将ChargePlan转换为JSON
inline json chargePlanToJson(const SmartChargeScheduler::ChargePlan& plan) {
    json j;
    j["user_id"] = plan.userId;
    j["charger_id"] = plan.chargerId;
    j["order_id"] = plan.orderId;
    j["target_energy"] = plan.targetEnergy;
    j["target_soc"] = plan.targetSoc;
    j["period"] = (int)plan.period;
    j["period_name"] = (plan.period == SmartChargeScheduler::TimePeriod::Valley ? "谷时" :
                         plan.period == SmartChargeScheduler::TimePeriod::Peak ? "峰时" : "平时");
    j["current_power"] = plan.currentPower;
    j["price_per_kwh"] = plan.pricePerKwh;
    j["estimated_time"] = plan.estimatedTime;
    j["estimated_cost"] = plan.estimatedCost;
    j["delay_minutes"] = plan.delayMinutes;
    j["reason"] = plan.reason;
    return j;
}
