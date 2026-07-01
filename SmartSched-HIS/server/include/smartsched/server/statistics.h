/**
 * @file statistics.h
 * @brief 统计报表模块
 * 
 * 医院视角: 提供日报、月报、年报数据支持
 * 管理视角: 数据导出、趋势分析
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include "../../common/include/smartsched/utils/datetime.h"

namespace smartsched {
namespace server {

// =============================================================================
// 统计数据结构
// =============================================================================

// 日统计
struct DailyStatistics {
    std::string date;           // 日期 YYYY-MM-DD
    int total_registrations;    // 总挂号数
    int total_consultations;    // 总就诊数
    int total_completions;      // 已完成数
    int total_cancellations;    // 取消数
    
    // 排队统计
    int peak_queue_size;        // 峰值排队人数
    int avg_wait_time;         // 平均等待时间(秒)
    int max_wait_time;         // 最大等待时间(秒)
    int min_wait_time;         // 最小等待时间(秒)
    
    // 就诊统计
    int avg_consult_time;      // 平均就诊时长(秒)
    int max_consult_time;      // 最大就诊时长(秒)
    
    // B超统计
    int ultrasound_requests;    // B超申请数
    int ultrasound_completed;   // B超完成数
    
    // 各科室统计
    std::map<int, int> dept_registrations;  // 科室ID -> 挂号数
    std::map<int, int> dept_consultations;   // 科室ID -> 就诊数
    
    // 时间分布 (小时 -> 挂号数)
    std::map<int, int> hourly_distribution;
};

// 月统计
struct MonthlyStatistics {
    int year;
    int month;
    
    int total_registrations;   // 月总挂号数
    int total_consultations;   // 月总就诊数
    int avg_daily_registrations; // 日均挂号数
    int avg_daily_consultations;  // 日均就诊数
    
    // 环比增长
    double registration_growth; // 挂号增长率(%)
    double consultation_growth; // 就诊增长率(%)
    
    // 日统计列表
    std::vector<DailyStatistics> daily_stats;
    
    // 医生排名
    std::vector<DoctorRanking> doctor_rankings;
};

// 医生排名
struct DoctorRanking {
    int doctor_id;
    std::string doctor_name;
    std::string dept_name;
    int consultation_count;
    double avg_consult_time;
    int patient_satisfaction;
};

// 科室统计
struct DepartmentStatistics {
    int dept_id;
    std::string dept_name;
    int total_registrations;
    int total_consultations;
    int avg_wait_time;
    int avg_consult_time;
    double completion_rate;
    
    // 医生列表
    std::vector<DoctorRanking> doctors;
};

// 患者统计
struct PatientStatistics {
    int total_patients;        // 总患者数
    int new_patients;          // 新患者数
    int repeat_patients;       // 复诊患者数
    double repeat_rate;         // 复诊率
    
    // 年龄分布
    std::map<std::string, int> age_distribution;  //年龄段 -> 人数
    
    // 性别分布
    std::map<std::string, int> gender_distribution;
};

// 趋势数据
struct TrendData {
    std::string date;
    double value;
};

} // namespace server
} // namespace smartsched
