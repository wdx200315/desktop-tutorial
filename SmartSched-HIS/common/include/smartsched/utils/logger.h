/**
 * @file logger.h
 * @brief 日志系统
 * 
 * 运维视角: 分级日志便于问题定位
 * 审计视角: 记录关键操作用于合规
 */

#pragma once

#include "../common/compiler.h"
#include "../common/macros.h"
#include "../common/version.h"
#include <string>
#include <memory>
#include <fstream>
#include <mutex>
#include <sstream>
#include <vector>
#include <functional>

namespace smartsched {
namespace utils {

// =============================================================================
// 日志级别
// =============================================================================
enum class LogLevel : int {
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warn = 3,
    Error = 4,
    Fatal = 5
};

// 日志级别到字符串
inline const char* logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO ";
        case LogLevel::Warn:  return "WARN ";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Fatal: return "FATAL";
        default: return "UNKN ";
    }
}

// =============================================================================
// 日志输出目标
// =============================================================================
enum class LogOutput {
    Console,
    File,
    Both
};

// =============================================================================
// 日志格式化器
// =============================================================================
class LogFormatter {
public:
    virtual ~LogFormatter() = default;
    
    virtual std::string format(
        LogLevel level,
        const char* file,
        int line,
        const char* function,
        const std::string& message
    ) = 0;
};

// 默认格式化器
class DefaultLogFormatter : public LogFormatter {
public:
    std::string format(
        LogLevel level,
        const char* file,
        int line,
        const char* function,
        const std::string& message
    ) override {
        std::ostringstream oss;
        
        // 时间戳
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        char time_buf[32];
        std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S",
                     std::localtime(&time_t));
        
        oss << "[" << time_buf << "." << std::setfill('0') << std::setw(3) << ms.count() << "]";
        
        // 级别
        oss << " [" << logLevelToString(level) << "]";
        
        // 位置信息
        oss << " [" << file << ":" << line << "]";
        
        // 函数名
        oss << " (" << function << ")";
        
        // 消息
        oss << " " << message;
        
        return oss.str();
    }
};

// =============================================================================
// 日志输出器接口
// =============================================================================
class ILogSink {
public:
    virtual ~ILogSink() = default;
    virtual void write(const std::string& formatted_message, LogLevel level) = 0;
};

// 控制台输出
class ConsoleSink : public ILogSink {
public:
    void write(const std::string& formatted_message, LogLevel level) override {
        if (level >= LogLevel::Warn) {
            std::cerr << formatted_message << std::endl;
        } else {
            std::cout << formatted_message << std::endl;
        }
    }
};

// 文件输出
class FileSink : public ILogSink {
public:
    explicit FileSink(const std::string& filepath) : filepath_(filepath) {
        open();
    }
    
    ~FileSink() override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (file_.is_open()) {
            file_.close();
        }
    }
    
    void write(const std::string& formatted_message, LogLevel level) override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (file_.is_open()) {
            file_ << formatted_message << std::endl;
            file_.flush();  // 实时写入，防止崩溃丢失日志
        }
    }
    
    void rotate() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (file_.is_open()) {
            file_.close();
        }
        
        // 重命名旧文件
        std::string backup_path = filepath_ + ".old";
        std::rename(filepath_.c_str(), backup_path.c_str());
        
        // 打开新文件
        open();
    }

private:
    void open() {
        file_.open(filepath_, std::ios::app | std::ios::binary);
        if (!file_.is_open()) {
            std::cerr << "Failed to open log file: " << filepath_ << std::endl;
        }
    }
    
    std::string filepath_;
    std::ofstream file_;
    std::mutex mutex_;
};

// =============================================================================
// 日志器
// =============================================================================
class Logger {
public:
    // 单例获取
    static Logger& instance() {
        static Logger logger;
        return logger;
    }
    
    // 配置
    void setLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(mutex_);
        min_level_ = level;
    }
    
    void setOutput(LogOutput output) {
        std::lock_guard<std::mutex> lock(mutex_);
        output_ = output;
    }
    
    void addFileSink(const std::string& filepath) {
        std::lock_guard<std::mutex> lock(mutex_);
        file_sinks_.push_back(std::make_unique<FileSink>(filepath));
    }
    
    void setFormatter(std::unique_ptr<LogFormatter> formatter) {
        std::lock_guard<std::mutex> lock(mutex_);
        formatter_ = std::move(formatter);
    }
    
    // 写日志
    void log(LogLevel level, const char* file, int line, const char* function,
             const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (level < min_level_) return;
        
        std::string formatted = formatter_->format(level, file, line, function, message);
        
        // 输出到控制台
        if (output_ == LogOutput::Console || output_ == LogOutput::Both) {
            console_sink_.write(formatted, level);
        }
        
        // 输出到文件
        if (output_ == LogOutput::File || output_ == LogOutput::Both) {
            for (auto& sink : file_sinks_) {
                sink->write(formatted, level);
            }
        }
    }
    
    // 便捷方法
    void trace(const std::string& msg, const char* file = __FILE__, int line = __LINE__) {
        log(LogLevel::Trace, file, line, __func__, msg);
    }
    
    void debug(const std::string& msg, const char* file = __FILE__, int line = __LINE__) {
        log(LogLevel::Debug, file, line, __func__, msg);
    }
    
    void info(const std::string& msg, const char* file = __FILE__, int line = __LINE__) {
        log(LogLevel::Info, file, line, __func__, msg);
    }
    
    void warn(const std::string& msg, const char* file = __FILE__, int line = __LINE__) {
        log(LogLevel::Warn, file, line, __func__, msg);
    }
    
    void error(const std::string& msg, const char* file = __FILE__, int line = __LINE__) {
        log(LogLevel::Error, file, line, __func__, msg);
    }
    
    void fatal(const std::string& msg, const char* file = __FILE__, int line = __LINE__) {
        log(LogLevel::Fatal, file, line, __func__, msg);
    }

private:
    Logger() {
        // 默认配置
        min_level_ = LogLevel::Info;
        output_ = LogOutput::Both;
        formatter_ = std::make_unique<DefaultLogFormatter>();
        
        // 默认添加一个文件sink
        std::string default_log = "logs/smartsched.log";
        file_sinks_.push_back(std::make_unique<FileSink>(default_log));
    }
    
    ~Logger() = default;
    SMARTSCHED_DISALLOW_COPY_AND_MOVE(Logger);
    
    LogLevel min_level_ = LogLevel::Info;
    LogOutput output_ = LogOutput::Both;
    std::unique_ptr<LogFormatter> formatter_;
    ConsoleSink console_sink_;
    std::vector<std::unique_ptr<FileSink>> file_sinks_;
    std::mutex mutex_;
};

// =============================================================================
// 日志宏（简化使用）
// =============================================================================
#define LOG_TRACE(msg) smartsched::utils::Logger::instance().trace(msg, __FILE__, __LINE__)
#define LOG_DEBUG(msg) smartsched::utils::Logger::instance().debug(msg, __FILE__, __LINE__)
#define LOG_INFO(msg)  smartsched::utils::Logger::instance().info(msg, __FILE__, __LINE__)
#define LOG_WARN(msg)  smartsched::utils::Logger::instance().warn(msg, __FILE__, __LINE__)
#define LOG_ERROR(msg) smartsched::utils::Logger::instance().error(msg, __FILE__, __LINE__)
#define LOG_FATAL(msg) smartsched::utils::Logger::instance().fatal(msg, __FILE__, __LINE__)

// 条件日志
#define LOG_IF(condition, level, msg) \
    do { if (condition) LOG_##level(msg); } while(0)

} // namespace utils
} // namespace smartsched
