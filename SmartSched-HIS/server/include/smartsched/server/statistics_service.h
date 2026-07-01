/**
 * @file statistics_service.h
 * @brief 统计服务
 * 
 * 医院视角: 提供多维度统计数据
 * 管理视角: 支持数据导出和报表生成
 */

#pragma once

#include "smartsched/server/statistics.h"
#include "smartsched/server/database.h"
#include <QObject>
#include <QMap>
#include <QString>

namespace smartsched {
namespace server {

// =============================================================================
// 统计服务
// =============================================================================
class StatisticsService : public QObject {
    Q_OBJECT
    
public:
    explicit StatisticsService(std::shared_ptr<ConnectionPool> db_pool, QObject* parent = nullptr);
    ~StatisticsService() override;
    
    // =========================================================================
    // 日统计
    // =========================================================================
    
    /**
     * @brief 获取指定日期的统计
     * @param date 日期 (YYYY-MM-DD)
     * @return 日统计数据
     */
    DailyStatistics getDailyStatistics(const std::string& date);
    
    /**
     * @brief 获取今日统计
     * @return 今日统计数据
     */
    DailyStatistics getTodayStatistics();
    
    /**
     * @brief 获取日期范围统计
     * @param start_date 开始日期
     * @param end_date 结束日期
     * @return 日统计列表
     */
    std::vector<DailyStatistics> getRangeStatistics(
        const std::string& start_date, 
        const std::string& end_date
    );
    
    // =========================================================================
    // 月统计
    // =========================================================================
    
    /**
     * @brief 获取指定月份的统计
     * @param year 年份
     * @param month 月份
     * @return 月统计数据
     */
    MonthlyStatistics getMonthlyStatistics(int year, int month);
    
    /**
     * @brief 获取指定年份的统计
     * @param year 年份
     * @return 月统计列表
     */
    std::vector<MonthlyStatistics> getYearStatistics(int year);
    
    // =========================================================================
    // 科室统计
    // =========================================================================
    
    /**
     * @brief 获取所有科室统计
     * @param start_date 开始日期
     * @param end_date 结束日期
     * @return 科室统计列表
     */
    std::vector<DepartmentStatistics> getDepartmentStatistics(
        const std::string& start_date,
        const std::string& end_date
    );
    
    /**
     * @brief 获取单个科室统计
     * @param dept_id 科室ID
     * @param start_date 开始日期
     * @param end_date 结束日期
     * @return 科室统计数据
     */
    DepartmentStatistics getSingleDepartmentStatistics(
        int dept_id,
        const std::string& start_date,
        const std::string& end_date
    );
    
    // =========================================================================
    // 医生统计
    // =========================================================================
    
    /**
     * @brief 获取医生接诊排名
     * @param start_date 开始日期
     * @param end_date 结束日期
     * @param limit 返回数量
     * @return 医生排名列表
     */
    std::vector<DoctorRanking> getDoctorRankings(
        const std::string& start_date,
        const std::string& end_date,
        int limit = 10
    );
    
    /**
     * @brief 获取医生统计
     * @param doctor_id 医生ID
     * @param start_date 开始日期
     * @param end_date 结束日期
     * @return 医生统计数据
     */
    DoctorRanking getDoctorStatistics(
        int doctor_id,
        const std::string& start_date,
        const std::string& end_date
    );
    
    // =========================================================================
    // 患者统计
    // =========================================================================
    
    /**
     * @brief 获取患者统计
     * @param start_date 开始日期
     * @param end_date 结束日期
     * @return 患者统计数据
     */
    PatientStatistics getPatientStatistics(
        const std::string& start_date,
        const std::string& end_date
    );
    
    // =========================================================================
    // 趋势分析
    // =========================================================================
    
    /**
     * @brief 获取挂号趋势
     * @param start_date 开始日期
     * @param end_date 结束日期
     * @return 趋势数据列表
     */
    std::vector<TrendData> getRegistrationTrend(
        const std::string& start_date,
        const std::string& end_date
    );
    
    /**
     * @brief 获取就诊趋势
     * @param start_date 开始日期
     * @param end_date 结束日期
     * @return 趋势数据列表
     */
    std::vector<TrendData> getConsultationTrend(
        const std::string& start_date,
        const std::string& end_date
    );
    
    /**
     * @brief 获取等待时间趋势
     * @param start_date 开始日期
     * @param end_date 结束日期
     * @return 趋势数据列表
     */
    std::vector<TrendData> getWaitTimeTrend(
        const std::string& start_date,
        const std::string& end_date
    );
    
    // =========================================================================
    // 实时统计
    // =========================================================================
    
    /**
     * @brief 获取当前在线统计
     * @return 当前统计数据
     */
    struct RealtimeStatistics {
        int current_queue_size;
        int current_waiting;
        int current_consulting;
        int today_registrations;
        int today_consultations;
        int available_doctors;
        int ultrasound_available;
    };
    
    RealtimeStatistics getRealtimeStatistics();
    
signals:
    void statisticsUpdated();
    
private:
    // 内部辅助方法
    std::string getToday();
    std::vector<DailyStatistics> queryDailyStats(
        const std::string& start_date,
        const std::string& end_date
    );
    
    std::shared_ptr<ConnectionPool> db_pool_;
};

} // namespace server
} // namespace smartsched
