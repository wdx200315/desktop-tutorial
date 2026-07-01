#pragma once
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <numeric>
#include <algorithm>
#include "json.hpp"

using json = nlohmann::json;

/**
 * HealthPredictor - 充电桩健康度预测系统
 * 
 * 核心功能：
 * 1. 基于历史数据计算充电桩健康评分
 * 2. 预测充电桩是否需要维护
 * 3. 分析故障趋势，提前预警
 * 4. 提供维护建议
 */
class HealthPredictor {
public:
    // 健康指标
    struct HealthMetrics {
        int chargerId;
        std::string serialNumber;
        
        // 运行数据
        double avgTemp;           // 平均温度
        double maxTemp;           // 最高温度
        double avgPower;          // 平均功率
        double utilization;       // 利用率
        
        // 故障数据
        int faultCount;           // 故障次数
        int offlineCount;         // 离线次数
        int errorCodeCount;       // 各类错误码次数
        
        // 时间数据
        int totalChargeHours;     // 累计充电时长(小时)
        int totalCycles;          // 累计充电次数
        
        // 趋势数据
        double tempTrend;         // 温度趋势 (+1上升, 0稳定, -1下降)
        double faultTrend;        // 故障趋势
    };

    // 健康评估结果
    struct HealthReport {
        int chargerId;
        int healthScore;          // 健康评分 (0-100)
        std::string healthLevel;  // 健康等级: 优秀/良好/一般/较差/危险
        bool needsMaintenance;    // 是否需要维护
        int maintenanceDays;      // 建议维护天数
        double reliability;       // 可靠性指数 (0-1)
        double uptime;            // 可用率 (0-1)
        
        // 问题列表
        std::vector<std::string> issues;
        std::vector<std::string> recommendations;
        
        // 预测数据
        double predictedFailProbability30Days;  // 30天内故障概率
        double estimatedRemainingLife;          // 预估剩余寿命(天)
    };

    // 充电记录 (用于分析)
    struct ChargeRecord {
        int chargerId;
        std::chrono::system_clock::time_point startTime;
        std::chrono::system_clock::time_point endTime;
        double energy;            // 充电量
        double maxTemp;           // 充电中最高温度
        double avgTemp;           // 平均温度
        double maxPower;          // 最大功率
        bool hadFault;            // 是否有故障
        std::string faultCode;    // 故障码
    };

    HealthPredictor();

    // 添加充电记录
    void addChargeRecord(const ChargeRecord& record);
    
    // 添加故障记录
    void addFaultRecord(int chargerId, const std::string& faultCode, 
                        const std::chrono::system_clock::time_point& time);
    
    // 添加离线记录
    void addOfflineRecord(int chargerId, int durationMinutes);
    
    // 计算健康评分
    int calculateHealthScore(const HealthMetrics& metrics);
    
    // 获取健康等级
    std::string getHealthLevel(int score);
    
    // 评估是否需要维护
    bool needsMaintenance(const HealthMetrics& metrics);
    
    // 生成健康报告
    HealthReport generateReport(int chargerId);
    
    // 获取健康趋势
    HealthMetrics getMetrics(int chargerId);
    
    // 获取充电桩列表的健康状态
    std::map<int, HealthReport> getAllHealthReports();
    
    // 获取需要维护的充电桩列表
    std::vector<int> getChargersNeedingMaintenance(int thresholdScore = 70);

private:
    std::map<int, HealthMetrics> metrics_;
    std::map<int, std::vector<ChargeRecord>> chargeHistory_;
    std::map<int, std::vector<std::chrono::system_clock::time_point>> faultHistory_;
    std::map<int, std::vector<int>> offlineHistory_;  // 离线时长记录(分钟)
    
    // 计算温度趋势
    double calculateTempTrend(int chargerId);
    
    // 计算故障趋势
    double calculateFaultTrend(int chargerId);
    
    // 预测30天故障概率
    double predictFailureProbability30Days(int chargerId);
    
    // 估算剩余寿命
    double estimateRemainingLife(int chargerId);
    
    // 计算可靠性指数
    double calculateReliability(const HealthMetrics& metrics);
    
    // 计算可用率
    double calculateUptime(int chargerId);
};

// ============ 实现 ============

HealthPredictor::HealthPredictor() {
    // 初始化时会自动初始化各个充电桩的默认指标
}

void HealthPredictor::addChargeRecord(const ChargeRecord& record) {
    chargeHistory_[record.chargerId].push_back(record);
    
    // 更新指标
    auto& m = metrics_[record.chargerId];
    m.chargerId = record.chargerId;
    m.totalChargeHours += std::chrono::duration_cast<std::chrono::hours>(
        record.endTime - record.startTime).count();
    m.totalCycles++;
    
    // 更新温度统计
    if (record.maxTemp > m.maxTemp) {
        m.maxTemp = record.maxTemp;
    }
    
    // 计算平均温度
    double totalTemp = m.avgTemp * (m.totalCycles - 1) + record.avgTemp;
    m.avgTemp = totalTemp / m.totalCycles;
}

void HealthPredictor::addFaultRecord(int chargerId, const std::string& faultCode,
                                       const std::chrono::system_clock::time_point& time) {
    faultHistory_[chargerId].push_back(time);
    metrics_[chargerId].faultCount++;
    if (!faultCode.empty()) {
        metrics_[chargerId].errorCodeCount++;
    }
}

void HealthPredictor::addOfflineRecord(int chargerId, int durationMinutes) {
    offlineHistory_[chargerId].push_back(durationMinutes);
    metrics_[chargerId].offlineCount++;
}

int HealthPredictor::calculateHealthScore(const HealthMetrics& metrics) {
    int score = 100;
    
    // 1. 温度异常扣分 (权重: 30分)
    if (metrics.avgTemp > 60) {
        score -= 30;
    } else if (metrics.avgTemp > 50) {
        score -= 20;
    } else if (metrics.avgTemp > 40) {
        score -= 10;
    }
    
    // 最高温度检查
    if (metrics.maxTemp > 80) {
        score -= 15;
    } else if (metrics.maxTemp > 70) {
        score -= 10;
    }
    
    // 2. 故障次数扣分 (权重: 35分)
    int faultDeduction = std::min(35, metrics.faultCount * 7);
    score -= faultDeduction;
    
    // 3. 离线次数扣分 (权重: 15分)
    int offlineDeduction = std::min(15, metrics.offlineCount * 3);
    score -= offlineDeduction;
    
    // 4. 错误码次数扣分 (权重: 10分)
    int errorDeduction = std::min(10, metrics.errorCodeCount * 2);
    score -= errorDeduction;
    
    // 5. 利用率异常扣分 (权重: 10分)
    if (metrics.utilization > 0.95) {
        score -= 5;  // 利用率过高可能预示过载
    } else if (metrics.utilization < 0.05 && metrics.totalCycles > 10) {
        score -= 3;  // 利用率过低可能是设备问题
    }
    
    // 确保分数在0-100范围内
    return std::max(0, std::min(100, score));
}

std::string HealthPredictor::getHealthLevel(int score) {
    if (score >= 90) return "优秀";
    if (score >= 80) return "良好";
    if (score >= 60) return "一般";
    if (score >= 40) return "较差";
    return "危险";
}

bool HealthPredictor::needsMaintenance(const HealthMetrics& metrics) {
    // 多条件判断是否需要维护
    if (calculateHealthScore(metrics) < 70) return true;
    if (metrics.maxTemp > 75) return true;
    if (metrics.faultCount > 5) return true;
    if (metrics.offlineCount > 3) return true;
    return false;
}

HealthPredictor::HealthReport HealthPredictor::generateReport(int chargerId) {
    HealthReport report;
    report.chargerId = chargerId;
    
    auto it = metrics_.find(chargerId);
    if (it == metrics_.end()) {
        // 如果没有数据，返回默认健康状态
        report.healthScore = 100;
        report.healthLevel = "优秀";
        report.needsMaintenance = false;
        report.maintenanceDays = 365;
        report.reliability = 1.0;
        report.uptime = 1.0;
        report.issues.push_back("无历史数据");
        report.recommendations.push_back("首次使用，建议运行一周后重新评估");
        return report;
    }
    
    const HealthMetrics& m = it->second;
    
    // 计算各项指标
    report.healthScore = calculateHealthScore(m);
    report.healthLevel = getHealthLevel(report.healthScore);
    report.needsMaintenance = needsMaintenance(m);
    report.reliability = calculateReliability(m);
    report.uptime = calculateUptime(chargerId);
    
    // 预测数据
    report.predictedFailProbability30Days = predictFailureProbability30Days(chargerId);
    report.estimatedRemainingLife = estimateRemainingLife(chargerId);
    
    // 生成问题列表
    if (m.avgTemp > 50) {
        report.issues.push_back("平均温度过高: " + std::to_string((int)m.avgTemp) + "°C");
        report.recommendations.push_back("建议检查散热系统，清理风扇和散热片");
    }
    if (m.maxTemp > 70) {
        report.issues.push_back("最高温度异常: " + std::to_string((int)m.maxTemp) + "°C");
        report.recommendations.push_back("立即检查，可能存在短路或过载");
    }
    if (m.faultCount > 3) {
        report.issues.push_back("故障次数过多: " + std::to_string(m.faultCount) + "次");
        report.recommendations.push_back("建议进行全面检修，检查电路板");
    }
    if (m.offlineCount > 2) {
        report.issues.push_back("离线次数过多: " + std::to_string(m.offlineCount) + "次");
        report.recommendations.push_back("检查网络连接和电源稳定性");
    }
    if (report.predictedFailProbability30Days > 0.3) {
        report.issues.push_back("30天故障概率较高: " + 
                                 std::to_string((int)(report.predictedFailProbability30Days * 100)) + "%");
        report.recommendations.push_back("建议本周内安排预防性维护");
    }
    
    // 根据健康等级给出建议
    if (report.healthScore >= 90) {
        report.maintenanceDays = 90;
        report.recommendations.push_back("设备状态优秀，按常规维护即可");
    } else if (report.healthScore >= 80) {
        report.maintenanceDays = 60;
        report.recommendations.push_back("设备状态良好，建议60天内检查");
    } else if (report.healthScore >= 60) {
        report.maintenanceDays = 30;
        report.recommendations.push_back("设备状态一般，建议30天内维护");
    } else {
        report.maintenanceDays = 7;
        report.recommendations.push_back("设备状态较差，建议立即安排维护");
    }
    
    return report;
}

HealthPredictor::HealthMetrics HealthPredictor::getMetrics(int chargerId) {
    if (metrics_.count(chargerId)) {
        HealthMetrics m = metrics_[chargerId];
        m.tempTrend = calculateTempTrend(chargerId);
        m.faultTrend = calculateFaultTrend(chargerId);
        return m;
    }
    
    // 返回默认指标
    HealthMetrics defaultMetrics;
    defaultMetrics.chargerId = chargerId;
    defaultMetrics.avgTemp = 30.0;
    defaultMetrics.maxTemp = 40.0;
    defaultMetrics.avgPower = 0;
    defaultMetrics.utilization = 0;
    defaultMetrics.faultCount = 0;
    defaultMetrics.offlineCount = 0;
    defaultMetrics.errorCodeCount = 0;
    defaultMetrics.totalChargeHours = 0;
    defaultMetrics.totalCycles = 0;
    defaultMetrics.tempTrend = 0;
    defaultMetrics.faultTrend = 0;
    return defaultMetrics;
}

std::map<int, HealthPredictor::HealthReport> HealthPredictor::getAllHealthReports() {
    std::map<int, HealthReport> reports;
    for (const auto& pair : metrics_) {
        reports[pair.first] = generateReport(pair.first);
    }
    return reports;
}

std::vector<int> HealthPredictor::getChargersNeedingMaintenance(int thresholdScore) {
    std::vector<int> result;
    for (const auto& pair : metrics_) {
        int score = calculateHealthScore(pair.second);
        if (score < thresholdScore) {
            result.push_back(pair.first);
        }
    }
    return result;
}

double HealthPredictor::calculateTempTrend(int chargerId) {
    auto it = chargeHistory_.find(chargerId);
    if (it == chargeHistory_.end() || it->second.size() < 5) {
        return 0;  // 数据不足
    }
    
    const auto& records = it->second;
    size_t n = records.size();
    
    // 比较前半部分和后半部分的平均温度
    double earlyAvg = 0, lateAvg = 0;
    for (size_t i = 0; i < n / 2; i++) {
        earlyAvg += records[i].avgTemp;
    }
    earlyAvg /= (n / 2);
    
    for (size_t i = n / 2; i < n; i++) {
        lateAvg += records[i].avgTemp;
    }
    lateAvg /= (n - n / 2);
    
    if (lateAvg > earlyAvg + 2) return 1;   // 上升
    if (lateAvg < earlyAvg - 2) return -1;  // 下降
    return 0;  // 稳定
}

double HealthPredictor::calculateFaultTrend(int chargerId) {
    auto it = faultHistory_.find(chargerId);
    if (it == faultHistory_.end() || it->second.size() < 3) {
        return 0;  // 数据不足
    }
    
    const auto& faults = it->second;
    size_t n = faults.size();
    
    // 比较前半部分和后半部分的故障频率
    double earlyRate = 0, lateRate = 0;
    if (faults.size() >= 2) {
        earlyRate = 1.0 / (std::chrono::duration_cast<std::chrono::days>(
            faults[n/2] - faults[0]).count() + 1);
        lateRate = 1.0 / (std::chrono::duration_cast<std::chrono::days>(
            faults[n-1] - faults[n/2]).count() + 1);
    }
    
    if (lateRate > earlyRate * 1.5) return 1;   // 上升趋势
    if (lateRate < earlyRate * 0.5) return -1; // 下降趋势
    return 0;  // 稳定
}

double HealthPredictor::predictFailureProbability30Days(int chargerId) {
    auto it = metrics_.find(chargerId);
    if (it == metrics_.end()) return 0.1;  // 默认10%
    
    const HealthMetrics& m = it->second;
    
    // 简单概率模型
    double baseProbability = 0.05;  // 基础概率5%
    
    // 根据故障次数调整
    double faultFactor = 1.0 + m.faultCount * 0.1;
    
    // 根据温度调整
    double tempFactor = 1.0;
    if (m.avgTemp > 60) tempFactor = 3.0;
    else if (m.avgTemp > 50) tempFactor = 2.0;
    else if (m.avgTemp > 40) tempFactor = 1.5;
    
    // 根据趋势调整
    double trendFactor = 1.0;
    if (m.tempTrend > 0) trendFactor *= 1.5;
    if (m.faultTrend > 0) trendFactor *= 1.5;
    
    double probability = baseProbability * faultFactor * tempFactor * trendFactor;
    return std::min(0.95, std::max(0.01, probability));
}

double HealthPredictor::estimateRemainingLife(int chargerId) {
    auto it = metrics_.find(chargerId);
    if (it == metrics_.end()) return 365;  // 默认1年
    
    const HealthMetrics& m = it->second;
    
    // 根据健康评分估算
    int score = calculateHealthScore(m);
    int baseDays = 365;
    
    // 分数越高，剩余寿命越长
    double lifeDays = baseDays * (score / 100.0);
    
    // 根据温度调整
    if (m.avgTemp > 50) lifeDays *= 0.5;  // 高温环境减半
    else if (m.avgTemp > 40) lifeDays *= 0.8;
    
    // 根据使用时长调整
    if (m.totalChargeHours > 10000) lifeDays *= 0.7;
    else if (m.totalChargeHours > 5000) lifeDays *= 0.85;
    
    return std::max(1.0, lifeDays);  // 至少1天
}

double HealthPredictor::calculateReliability(const HealthMetrics& metrics) {
    // 可靠性 = 1 - (故障概率 * 温度系数 * 趋势系数)
    double baseReliability = 1.0;
    
    // 故障影响
    double faultImpact = std::min(0.3, metrics.faultCount * 0.05);
    
    // 温度影响
    double tempImpact = 0;
    if (metrics.avgTemp > 60) tempImpact = 0.2;
    else if (metrics.avgTemp > 50) tempImpact = 0.1;
    else if (metrics.avgTemp > 40) tempImpact = 0.05;
    
    // 趋势影响
    double trendImpact = 0;
    if (metrics.tempTrend > 0) trendImpact += 0.1;
    if (metrics.faultTrend > 0) trendImpact += 0.1;
    
    return std::max(0.0, baseReliability - faultImpact - tempImpact - trendImpact);
}

double HealthPredictor::calculateUptime(int chargerId) {
    auto offlineIt = offlineHistory_.find(chargerId);
    if (offlineIt == offlineHistory_.end() || offlineIt->second.empty()) {
        return 1.0;  // 没有离线记录
    }
    
    const auto& offlineDurations = offlineIt->second;
    int totalOfflineMinutes = std::accumulate(offlineDurations.begin(), 
                                                offlineDurations.end(), 0);
    
    // 假设统计周期为30天
    int totalMinutesInPeriod = 30 * 24 * 60;
    
    return 1.0 - (totalOfflineMinutes / (double)totalMinutesInPeriod);
}

// 辅助函数：将HealthReport转换为JSON
inline json healthReportToJson(const HealthPredictor::HealthReport& report) {
    json j;
    j["charger_id"] = report.chargerId;
    j["health_score"] = report.healthScore;
    j["health_level"] = report.healthLevel;
    j["needs_maintenance"] = report.needsMaintenance;
    j["maintenance_days"] = report.maintenanceDays;
    j["reliability"] = report.reliability;
    j["uptime"] = report.uptime;
    j["issues"] = report.issues;
    j["recommendations"] = report.recommendations;
    j["fail_probability_30d"] = report.predictedFailProbability30Days;
    j["remaining_life_days"] = report.estimatedRemainingLife;
    return j;
}
