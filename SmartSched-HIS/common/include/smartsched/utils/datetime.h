/**
 * @file datetime.h
 * @brief 日期时间工具
 * 
 * 业务视角: 医院业务的精确时间记录
 * 报表视角: 支持多种时间格式统计
 */

#pragma once

#include "../common/compiler.h"
#include <string>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

namespace smartsched {
namespace utils {

// =============================================================================
// 日期时间类
// =============================================================================
class DateTime {
public:
    using Clock = std::chrono::system_clock;
    using TimePoint = Clock::time_point;
    using Duration = std::chrono::milliseconds;
    
    // =============================================================================
    // 构造函数
    // =============================================================================
    
    DateTime() : time_point_(Clock::now()) {}
    
    explicit DateTime(const TimePoint& tp) : time_point_(tp) {}
    
    explicit DateTime(time_t t) : time_point_(Clock::from_time_t(t)) {}
    
    DateTime(int year, int month, int day, int hour = 0, int minute = 0, int second = 0) {
        struct tm tm_struct = {};
        tm_struct.tm_year = year - 1900;
        tm_struct.tm_mon = month - 1;
        tm_struct.tm_mday = day;
        tm_struct.tm_hour = hour;
        tm_struct.tm_min = minute;
        tm_struct.tm_sec = second;
        time_point_ = Clock::from_time_t(std::mktime(&tm_struct));
    }
    
    // =============================================================================
    // 静态工厂方法
    // =============================================================================
    
    static DateTime now() { return DateTime(); }
    
    static DateTime today() {
        auto now = Clock::now();
        auto date = std::chrono::floor<std::chrono::days>(now);
        return DateTime(std::chrono::sys_days(date));
    }
    
    static DateTime fromTimestamp(int64_t timestamp_ms) {
        return DateTime(TimePoint(Duration(timestamp_ms)));
    }
    
    // =============================================================================
    // 格式化输出
    // =============================================================================
    
    std::string toString(const std::string& format = "%Y-%m-%d %H:%M:%S") const {
        auto time = Clock::to_time_t(time_point_);
        char buffer[64];
        std::strftime(buffer, sizeof(buffer), format.c_str(), std::localtime(&time));
        return std::string(buffer);
    }
    
    // 标准格式
    std::string toDateString() const { return toString("%Y-%m-%d"); }
    std::string toTimeString() const { return toString("%H:%M:%S"); }
    std::string toDateTimeString() const { return toString("%Y-%m-%d %H:%M:%S"); }
    std::string toIsoString() const { return toString("%Y-%m-%dT%H:%M:%S"); }
    
    // 中文格式
    std::string toChineseString() const {
        return toString("%Y年%m月%d日 %H:%M:%S");
    }
    
    // 短格式（用于显示）
    std::string toShortString() const {
        auto now_t = Clock::to_time_t(Clock::now());
        auto this_t = Clock::to_time_t(time_point_);
        auto today_start = std::chrono::floor<std::chrono::days>(Clock::now());
        auto this_date = std::chrono::floor<std::chrono::days>(time_point_);
        
        std::ostringstream oss;
        
        if (this_date == today_start) {
            // 今天
            oss << "今天 " << toString("%H:%M");
        } else if (this_date == today_start - std::chrono::days(1)) {
            // 昨天
            oss << "昨天 " << toString("%H:%M");
        } else {
            oss << toString("%m-%d %H:%M");
        }
        
        return oss.str();
    }
    
    // =============================================================================
    // 时间戳
    // =============================================================================
    
    int64_t toTimestamp() const {
        return std::chrono::duration_cast<Duration>(time_point_.time_since_epoch()).count();
    }
    
    int64_t toUnixTimestamp() const {
        return Clock::to_time_t(time_point_);
    }
    
    time_t toTimeT() const {
        return Clock::to_time_t(time_point_);
    }
    
    // =============================================================================
    // 日期组件
    // =============================================================================
    
    int year() const {
        auto date = std::chrono::floor<std::chrono::days>(time_point_);
        auto ymd = std::chrono::year_month_day(date);
        return static_cast<int>(ymd.year());
    }
    
    int month() const {
        auto date = std::chrono::floor<std::chrono::days>(time_point_);
        auto ymd = std::chrono::year_month_day(date);
        return static_cast<unsigned>(ymd.month());
    }
    
    int day() const {
        auto date = std::chrono::floor<std::chrono::days>(time_point_);
        auto ymd = std::chrono::year_month_day(date);
        return static_cast<unsigned>(ymd.day());
    }
    
    int hour() const {
        auto time = Clock::to_time_t(time_point_);
        struct tm* tm = std::localtime(&time);
        return tm->tm_hour;
    }
    
    int minute() const {
        auto time = Clock::to_time_t(time_point_);
        struct tm* tm = std::localtime(&time);
        return tm->tm_min;
    }
    
    int second() const {
        auto time = Clock::to_time_t(time_point_);
        struct tm* tm = std::localtime(&time);
        return tm->tm_sec;
    }
    
    int dayOfWeek() const {
        auto time = Clock::to_time_t(time_point_);
        struct tm* tm = std::localtime(&time);
        return tm->tm_wday;
    }
    
    std::string dayOfWeekChinese() const {
        static const char* days[] = {"周日", "周一", "周二", "周三", "周四", "周五", "周六"};
        return days[dayOfWeek()];
    }
    
    // =============================================================================
    // 时间运算
    // =============================================================================
    
    DateTime& addYears(int years) {
        auto date = std::chrono::floor<std::chrono::days>(time_point_);
        auto ymd = std::chrono::year_month_day(date);
        ymd = ymd + std::chrono::years(years);
        time_point_ = Clock::from_time_t(std::mktime(
            [ymd]() { 
                std::tm t = {}; 
                t.tm_year = static_cast<int>(ymd.year()) - 1900; 
                t.tm_mon = static_cast<unsigned>(ymd.month()) - 1; 
                t.tm_mday = static_cast<unsigned>(ymd.day()); 
                return t; 
            }()
        ));
        return *this;
    }
    
    DateTime& addDays(int days) {
        time_point_ += std::chrono::days(days);
        return *this;
    }
    
    DateTime& addHours(int hours) {
        time_point_ += std::chrono::hours(hours);
        return *this;
    }
    
    DateTime& addMinutes(int minutes) {
        time_point_ += std::chrono::minutes(minutes);
        return *this;
    }
    
    DateTime& addSeconds(int seconds) {
        time_point_ += std::chrono::seconds(seconds);
        return *this;
    }
    
    // =============================================================================
    // 比较操作
    // =============================================================================
    
    bool operator<(const DateTime& other) const { return time_point_ < other.time_point_; }
    bool operator<=(const DateTime& other) const { return time_point_ <= other.time_point_; }
    bool operator>(const DateTime& other) const { return time_point_ > other.time_point_; }
    bool operator>=(const DateTime& other) const { return time_point_ >= other.time_point_; }
    bool operator==(const DateTime& other) const { return time_point_ == other.time_point_; }
    bool operator!=(const DateTime& other) const { return time_point_ != other.time_point_; }
    
    // =============================================================================
    // 运算符
    // =============================================================================
    
    int64_t operator-(const DateTime& other) const {
        return std::chrono::duration_cast<Duration>(time_point_ - other.time_point_).count();
    }
    
    DateTime operator+(const Duration& dur) const {
        return DateTime(time_point_ + dur);
    }
    
    DateTime operator-(const Duration& dur) const {
        return DateTime(time_point_ - dur);
    }
    
private:
    TimePoint time_point_;
};

// =============================================================================
// Duration格式化
// =============================================================================
inline std::string formatDuration(int64_t milliseconds) {
    if (milliseconds < 0) return "0秒";
    
    int64_t seconds = milliseconds / 1000;
    int64_t minutes = seconds / 60;
    int64_t hours = minutes / 60;
    int64_t days = hours / 24;
    
    std::ostringstream oss;
    if (days > 0) {
        oss << days << "天" << (hours % 24) << "小时";
    } else if (hours > 0) {
        oss << hours << "小时" << (minutes % 60) << "分钟";
    } else if (minutes > 0) {
        oss << minutes << "分钟" << (seconds % 60) << "秒";
    } else {
        oss << seconds << "秒";
    }
    
    return oss.str();
}

// =============================================================================
// 等待时间格式化（用于排队显示）
// =============================================================================
inline std::string formatWaitTime(int64_t seconds) {
    if (seconds < 0) return "无需等待";
    if (seconds < 60) return std::to_string(seconds) + "秒";
    if (seconds < 3600) {
        int mins = static_cast<int>(seconds / 60);
        int secs = static_cast<int>(seconds % 60);
        return std::to_string(mins) + "分" + std::to_string(secs) + "秒";
    }
    
    int hours = static_cast<int>(seconds / 3600);
    int mins = static_cast<int>((seconds % 3600) / 60);
    return std::to_string(hours) + "小时" + std::to_string(mins) + "分钟";
}

} // namespace utils
} // namespace smartsched
