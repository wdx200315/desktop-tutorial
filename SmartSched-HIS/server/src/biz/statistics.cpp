/**
 * @file statistics.cpp
 * @brief 统计服务实现 - 提供日报/月报/年报、趋势分析、实时监控
 */
#include "statistics_service.h"
#include "database.h"
#include "service.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <algorithm>

namespace smartsched {

// ==================== 单例实现 ====================
StatisticsService& StatisticsService::instance()
{
    static StatisticsService instance;
    return instance;
}

StatisticsService::StatisticsService(QObject* parent)
    : QObject(parent)
    , m_db(Database::instance())
    , m_running(false)
{
    // 启动统计刷新定时器（每60秒刷新实时数据）
    m_refreshTimer = new QTimer(this);
    connect(m_refreshTimer, &QTimer::timeout, this, &StatisticsService::refreshRealtimeData);
    m_refreshTimer->start(60000);
}

StatisticsService::~StatisticsService()
{
    m_refreshTimer->stop();
}

// ==================== 日报统计 ====================
DailyStatistics StatisticsService::getDailyStatistics(const std::string& date)
{
    DailyStatistics stats;
    stats.date = date;
    
    QSqlQuery query(m_db.getConnection());
    
    // 总挂号数
    query.exec(QString("SELECT COUNT(*) FROM registration WHERE DATE(register_time) = '%1'").arg(date.c_str()));
    if (query.next()) {
        stats.total_registrations = query.value(0).toInt();
    }
    
    // 各科室挂号分布
    query.exec(QString(R"(
        SELECT d.department_id, d.name, COUNT(r.registration_id) as cnt
        FROM department d
        LEFT JOIN registration r ON d.department_id = r.department_id 
            AND DATE(r.register_time) = '%1'
        GROUP BY d.department_id, d.name
        ORDER BY cnt DESC
    )").arg(date.c_str()));
    
    while (query.next()) {
        int deptId = query.value(0).toInt();
        int cnt = query.value(2).toInt();
        stats.dept_registrations[deptId] = cnt;
        
        // 记录科室名称映射
        stats.dept_names[deptId] = query.value(1).toString().toStdString();
    }
    
    // 每小时挂号分布
    query.exec(QString(R"(
        SELECT HOUR(register_time) as hr, COUNT(*) as cnt
        FROM registration
        WHERE DATE(register_time) = '%1'
        GROUP BY HOUR(register_time)
        ORDER BY hr
    )").arg(date.c_str()));
    
    while (query.next()) {
        int hour = query.value(0).toInt();
        int cnt = query.value(1).toInt();
        stats.hourly_distribution[hour] = cnt;
    }
    
    // 平均等待时间
    query.exec(QString(R"(
        SELECT AVG(TIMESTAMPDIFF(MINUTE, r.register_time, c.call_time)) as avg_wait
        FROM registration r
        JOIN consultation c ON r.registration_id = c.registration_id
        WHERE DATE(r.register_time) = '%1'
    )").arg(date.c_str()));
    if (query.next()) {
        stats.avg_wait_time = query.value(0).toInt();
    }
    
    // 峰值排队人数
    stats.peak_queue_size = calculatePeakQueueSize(date);
    
    return stats;
}

int StatisticsService::calculatePeakQueueSize(const std::string& date)
{
    QSqlQuery query(m_db.getConnection());
    int maxQueue = 0;
    
    // 遍历每小时的记录数作为峰值估算
    query.exec(QString(R"(
        SELECT MAX(cnt) as max_cnt FROM (
            SELECT COUNT(*) as cnt
            FROM registration
            WHERE DATE(register_time) = '%1'
            GROUP BY HOUR(register_time)
        ) as hourly_counts
    )").arg(date.c_str()));
    
    if (query.next()) {
        maxQueue = query.value(0).toInt();
    }
    
    return maxQueue;
}

// ==================== 月报统计 ====================
MonthlyStatistics StatisticsService::getMonthlyStatistics(int year, int month)
{
    MonthlyStatistics stats;
    stats.year = year;
    stats.month = month;
    
    QSqlQuery query(m_db.getConnection());
    
    // 本月总挂号数
    query.exec(QString("SELECT COUNT(*) FROM registration WHERE YEAR(register_time) = %1 AND MONTH(register_time) = %2")
               .arg(year).arg(month));
    if (query.next()) {
        stats.total_registrations = query.value(0).toInt();
    }
    
    // 上月挂号数（计算增长率）
    int prevMonth = month - 1;
    int prevYear = year;
    if (prevMonth < 1) {
        prevMonth = 12;
        prevYear = year - 1;
    }
    query.exec(QString("SELECT COUNT(*) FROM registration WHERE YEAR(register_time) = %1 AND MONTH(register_time) = %2")
               .arg(prevYear).arg(prevMonth));
    if (query.next()) {
        int prevCount = query.value(0).toInt();
        if (prevCount > 0) {
            stats.registration_growth = (double)(stats.total_registrations - prevCount) / prevCount * 100.0;
        }
    }
    
    // 本月各科室分布
    query.exec(QString(R"(
        SELECT d.name, COUNT(r.registration_id) as cnt
        FROM department d
        LEFT JOIN registration r ON d.department_id = r.department_id 
            AND YEAR(r.register_time) = %1 AND MONTH(r.register_time) = %2
        GROUP BY d.department_id, d.name
        ORDER BY cnt DESC
    )").arg(year).arg(month));
    
    while (query.next()) {
        std::string deptName = query.value(0).toString().toStdString();
        int cnt = query.value(1).toInt();
        stats.dept_distribution[deptName] = cnt;
    }
    
    // 本月高峰时段
    query.exec(QString(R"(
        SELECT HOUR(register_time) as hr, COUNT(*) as cnt
        FROM registration
        WHERE YEAR(register_time) = %1 AND MONTH(register_time) = %2
        GROUP BY HOUR(register_time)
        ORDER BY cnt DESC
        LIMIT 3
    )").arg(year).arg(month));
    
    while (query.next()) {
        stats.peak_hours.push_back(query.value(0).toInt());
    }
    
    // 平均每日挂号
    QDate firstDay(year, month, 1);
    QDate lastDay = firstDay.addMonths(1).addDays(-1);
    int days = lastDay.day();
    stats.avg_daily_registrations = days > 0 ? stats.total_registrations / days : 0;
    
    return stats;
}

// ==================== 科室统计 ====================
std::vector<DepartmentStatistics> StatisticsService::getDepartmentStatistics(
    const std::string& startDate, const std::string& endDate)
{
    std::vector<DepartmentStatistics> result;
    
    QSqlQuery query(m_db.getConnection());
    query.exec(QString(R"(
        SELECT 
            d.department_id,
            d.name,
            d.description,
            COUNT(r.registration_id) as total_cases,
            AVG(TIMESTAMPDIFF(MINUTE, r.register_time, c.call_time)) as avg_wait,
            AVG(TIMESTAMPDIFF(MINUTE, c.call_time, c.finish_time)) as avg_consult,
            COUNT(DISTINCT r.patient_id) as unique_patients
        FROM department d
        LEFT JOIN registration r ON d.department_id = r.department_id 
            AND DATE(r.register_time) BETWEEN '%1' AND '%2'
        LEFT JOIN consultation c ON r.registration_id = c.registration_id
        GROUP BY d.department_id, d.name, d.description
        ORDER BY total_cases DESC
    )").arg(startDate.c_str()).arg(endDate.c_str()));
    
    while (query.next()) {
        DepartmentStatistics dept;
        dept.department_id = query.value(0).toInt();
        dept.department_name = query.value(1).toString().toStdString();
        dept.description = query.value(2).toString().toStdString();
        dept.total_cases = query.value(3).toInt();
        dept.avg_wait_time = query.value(4).isNull() ? 0 : query.value(4).toInt();
        dept.avg_consultation_time = query.value(5).isNull() ? 0 : query.value(5).toInt();
        dept.unique_patients = query.value(6).toInt();
        
        // 计算医生数量
        QSqlQuery doctorQuery(m_db.getConnection());
        doctorQuery.exec(QString("SELECT COUNT(*) FROM doctor WHERE department_id = %1")
                        .arg(dept.department_id));
        if (doctorQuery.next()) {
            dept.doctor_count = doctorQuery.value(0).toInt();
        }
        
        result.push_back(dept);
    }
    
    return result;
}

// ==================== 医生排行 ====================
std::vector<DoctorRanking> StatisticsService::getDoctorRankings(
    const std::string& startDate, const std::string& endDate, int limit)
{
    std::vector<DoctorRanking> result;
    
    QSqlQuery query(m_db.getConnection());
    query.exec(QString(R"(
        SELECT 
            d.doctor_id,
            CONCAT(d.name, ' (', dept.name, ')') as display_name,
            COUNT(c.consultation_id) as consultation_count,
            AVG(TIMESTAMPDIFF(MINUTE, c.call_time, c.finish_time)) as avg_time,
            COUNT(DISTINCT r.patient_id) as patient_count
        FROM doctor d
        JOIN department dept ON d.department_id = dept.department_id
        LEFT JOIN consultation c ON d.doctor_id = c.doctor_id
            AND DATE(c.call_time) BETWEEN '%1' AND '%2'
        LEFT JOIN registration r ON c.registration_id = r.registration_id
        GROUP BY d.doctor_id, d.name, dept.name
        ORDER BY consultation_count DESC
        LIMIT %3
    )").arg(startDate.c_str()).arg(endDate.c_str()).arg(limit));
    
    while (query.next()) {
        DoctorRanking ranking;
        ranking.doctor_id = query.value(0).toInt();
        ranking.doctor_name = query.value(1).toString().toStdString();
        ranking.consultation_count = query.value(2).toInt();
        ranking.avg_consultation_time = query.value(3).isNull() ? 0 : query.value(3).toInt();
        ranking.patient_count = query.value(4).toInt();
        ranking.rating = calculateDoctorRating(ranking.doctor_id, startDate, endDate);
        result.push_back(ranking);
    }
    
    return result;
}

double StatisticsService::calculateDoctorRating(int doctorId, const std::string& startDate, const std::string& endDate)
{
    // 简化评分算法：基于接诊量和效率评分
    QSqlQuery query(m_db.getConnection());
    query.exec(QString(R"(
        SELECT 
            COUNT(*) as cases,
            AVG(TIMESTAMPDIFF(MINUTE, call_time, finish_time)) as avg_time
        FROM consultation
        WHERE doctor_id = %1 AND DATE(call_time) BETWEEN '%2' AND '%3'
    )").arg(doctorId).arg(startDate.c_str()).arg(endDate.c_str()));
    
    if (query.next()) {
        int cases = query.value(0).toInt();
        double avgTime = query.value(1).isNull() ? 15.0 : query.value(1).toDouble();
        
        // 效率评分：10分钟为标准，每少1分钟+0.1，每多1分钟-0.1
        double efficiency = 5.0 - (avgTime - 10.0) * 0.1;
        efficiency = std::max(1.0, std::min(5.0, efficiency));
        
        // 数量评分：50例为满分
        double volume = std::min(5.0, cases / 10.0);
        
        return (efficiency + volume) / 2.0 * 5.0 / 5.5; // 归一化到5分制
    }
    
    return 0.0;
}

// ==================== 趋势分析 ====================
std::vector<TrendData> StatisticsService::getRegistrationTrend(
    const std::string& startDate, const std::string& endDate, TrendType type)
{
    std::vector<TrendData> result;
    
    QSqlQuery query(m_db.getConnection());
    QString groupBy;
    QString dateFormat;
    
    switch (type) {
        case TrendType::Daily:
            groupBy = "DATE(register_time)";
            dateFormat = "%Y-%m-%d";
            break;
        case TrendType::Weekly:
            groupBy = "YEARWEEK(register_time, 1)";
            dateFormat = "%Y-W%u";
            break;
        case TrendType::Monthly:
            groupBy = "DATE_FORMAT(register_time, '%Y-%m')";
            dateFormat = "%Y-%m";
            break;
    }
    
    query.exec(QString(R"(
        SELECT 
            %1 as period,
            COUNT(*) as count,
            COUNT(DISTINCT patient_id) as unique_patients
        FROM registration
        WHERE DATE(register_time) BETWEEN '%2' AND '%3'
        GROUP BY %1
        ORDER BY period
    )").arg(groupBy).arg(startDate.c_str()).arg(endDate.c_str()));
    
    while (query.next()) {
        TrendData data;
        data.period = query.value(0).toString().toStdString();
        data.value = query.value(1).toInt();
        data.unique_patients = query.value(2).toInt();
        result.push_back(data);
    }
    
    // 计算趋势（简单线性回归斜率）
    if (result.size() >= 2) {
        double sumX = 0, sumY = 0, sumXY = 0, sumXX = 0;
        for (size_t i = 0; i < result.size(); ++i) {
            sumX += i;
            sumY += result[i].value;
            sumXY += i * result[i].value;
            sumXX += i * i;
        }
        double n = result.size();
        double slope = (n * sumXY - sumX * sumY) / (n * sumXX - sumX * sumX);
        
        // 判断趋势方向
        if (slope > 0.5) result[0].trend = "上升";
        else if (slope < -0.5) result[0].trend = "下降";
        else result[0].trend = "平稳";
    }
    
    return result;
}

// ==================== 实时统计 ====================
RealtimeStatistics StatisticsService::getRealtimeStatistics()
{
    QMutexLocker locker(&m_mutex);
    return m_realtimeStats;
}

void StatisticsService::refreshRealtimeData()
{
    QMutexLocker locker(&m_mutex);
    
    QSqlQuery query(m_db.getConnection());
    QDateTime now = QDateTime::currentDateTime();
    QString today = now.toString("yyyy-MM-dd");
    
    // 今日总挂号
    query.exec(QString("SELECT COUNT(*) FROM registration WHERE DATE(register_time) = '%1'").arg(today));
    if (query.next()) {
        m_realtimeStats.today_registrations = query.value(0).toInt();
    }
    
    // 当前等待人数
    query.exec(QString("SELECT COUNT(*) FROM registration WHERE status IN ('waiting', 'called')"));
    if (query.next()) {
        m_realtimeStats.current_waiting = query.value(0).toInt();
    }
    
    // 各科室当前等待
    query.exec(QString(R"(
        SELECT d.name, COUNT(r.registration_id)
        FROM department d
        LEFT JOIN registration r ON d.department_id = r.department_id AND r.status IN ('waiting', 'called')
        GROUP BY d.department_id, d.name
    )"));
    
    m_realtimeStats.dept_waiting.clear();
    while (query.next()) {
        m_realtimeStats.dept_waiting[query.value(0).toString().toStdString()] = query.value(1).toInt();
    }
    
    // 今日已完成
    query.exec(QString("SELECT COUNT(*) FROM consultation WHERE DATE(call_time) = '%1'").arg(today));
    if (query.next()) {
        m_realtimeStats.today_completed = query.value(0).toInt();
    }
    
    m_realtimeStats.last_update = now.toString("yyyy-MM-dd HH:mm:ss").toStdString();
}

// ==================== 数据导出 ====================
bool StatisticsService::exportToJson(const DailyStatistics& stats, const std::string& filepath)
{
    QJsonObject root;
    root["date"] = QString::fromStdString(stats.date);
    root["total_registrations"] = stats.total_registrations;
    root["peak_queue_size"] = stats.peak_queue_size;
    root["avg_wait_time"] = stats.avg_wait_time;
    
    QJsonObject deptDist;
    for (const auto& [id, count] : stats.dept_registrations) {
        deptDist[QString::number(id)] = count;
    }
    root["department_distribution"] = deptDist;
    
    QJsonObject hourlyDist;
    for (const auto& [hour, count] : stats.hourly_distribution) {
        hourlyDist[QString::number(hour)] = count;
    }
    root["hourly_distribution"] = hourlyDist;
    
    QFile file(QString::fromStdString(filepath));
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << QJsonDocument(root).toJson(QJsonDocument::Indented);
        file.close();
        return true;
    }
    return false;
}

std::string StatisticsService::exportToJsonString(const DailyStatistics& stats)
{
    QJsonObject root;
    root["date"] = QString::fromStdString(stats.date);
    root["total_registrations"] = stats.total_registrations;
    root["peak_queue_size"] = stats.peak_queue_size;
    root["avg_wait_time"] = stats.avg_wait_time;
    
    return QJsonDocument(root).toJson(QJsonDocument::Compact).toStdString();
}

// ==================== 报表生成 ====================
std::string StatisticsService::generateDailyReport(const std::string& date)
{
    DailyStatistics stats = getDailyStatistics(date);
    
    QString report;
    QTextStream stream(&report);
    
    stream << "========== 智序医院门诊调度系统 - " << date.c_str() << " 日报 ==========\n\n";
    stream << "【总体概览】\n";
    stream << "  总挂号数: " << stats.total_registrations << " 人\n";
    stream << "  峰值排队: " << stats.peak_queue_size << " 人\n";
    stream << "  平均等待: " << stats.avg_wait_time << " 分钟\n\n";
    
    stream << "【科室分布】\n";
    for (const auto& [id, count] : stats.dept_registrations) {
        QString name = stats.dept_names.count(id) ? 
            QString::fromStdString(stats.dept_names.at(id)) : QString("科室%1").arg(id);
        double pct = stats.total_registrations > 0 ? 
            (count * 100.0 / stats.total_registrations) : 0;
        stream << "  " << name << ": " << count << " 人 (" << QString::number(pct, 'f', 1) << "%)\n";
    }
    
    stream << "\n【时段分布】\n";
    for (int h = 8; h <= 17; ++h) {
        int count = stats.hourly_distribution.count(h) ? stats.hourly_distribution.at(h) : 0;
        stream << "  " << QString("%1:00-%2:00").arg(h, 2, 10, QChar('0')).arg(h, 2, 10, QChar('0')) 
               << ": " << count << " 人\n";
    }
    
    return report.toStdString();
}

std::string StatisticsService::generateMonthlyReport(int year, int month)
{
    MonthlyStatistics stats = getMonthlyStatistics(year, month);
    
    QString report;
    QTextStream stream(&report);
    
    stream << "========== 智序医院门诊调度系统 - " << year << "年" << month << "月 月报 ==========\n\n";
    stream << "【总体概览】\n";
    stream << "  本月挂号: " << stats.total_registrations << " 人\n";
    stream << "  日均挂号: " << stats.avg_daily_registrations << " 人/天\n";
    stream << "  环比增长: " << QString::number(stats.registration_growth, 'f', 1) << "%\n\n";
    
    stream << "【科室分布】\n";
    for (const auto& [name, count] : stats.dept_distribution) {
        double pct = stats.total_registrations > 0 ? 
            (count * 100.0 / stats.total_registrations) : 0;
        stream << "  " << name.c_str() << ": " << count << " 人 (" << QString::number(pct, 'f', 1) << "%)\n";
    }
    
    stream << "\n【高峰时段】\n";
    for (int h : stats.peak_hours) {
        stream << "  " << QString::number(h) << ":00\n";
    }
    
    return report.toStdString();
}

} // namespace smartsched
