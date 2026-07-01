/**
 * @file filelogger.h
 * @brief 文件日志系统 - 支持日志轮转、自动清理
 * 
 * 运维视角: 企业级日志管理
 * 审计视角: 操作日志可追溯
 */

#pragma once

#include <string>
#include <memory>
#include <fstream>
#include <mutex>
#include <queue>
#include <atomic>
#include <chrono>
#include <ctime>
#include <algorithm>
#include <cstring>

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
// 日志配置
// =============================================================================
struct FileLoggerConfig {
    std::string log_dir = "logs";
    std::string prefix = "smartsched";
    
    // 文件轮转配置
    int max_file_size = 10 * 1024 * 1024;  // 10MB
    int max_file_count = 10;
    int max_keep_days = 30;
    
    // 日志级别
    LogLevel min_level = LogLevel::Info;
    
    // 是否输出到控制台
    bool console_output = true;
    
    // 异步模式
    bool async_mode = true;
    int flush_interval_ms = 1000;
    int queue_size = 10000;
    
    // 时间格式
    bool use_local_time = true;
};

// =============================================================================
// 日志消息
// =============================================================================
struct LogMessage {
    LogLevel level;
    std::string file;
    int line;
    std::string function;
    std::string message;
    std::chrono::system_clock::time_point timestamp;
    int thread_id;
    
    LogMessage() : level(LogLevel::Info), line(0), thread_id(0) {}
    
    LogMessage(LogLevel lvl, const char* f, int l, const char* func, const std::string& msg)
        : level(lvl), file(f), line(l), function(func), message(msg), thread_id(0) {
        timestamp = std::chrono::system_clock::now();
    }
};

// =============================================================================
// 文件日志器
// =============================================================================
class FileLogger {
public:
    static FileLogger& instance();
    
    // 配置
    void configure(const FileLoggerConfig& config);
    FileLoggerConfig config() const { return config_; }
    
    // 写入日志
    void log(LogLevel level, const char* file, int line, const char* function,
             const std::string& message);
    
    // 便捷方法
    void trace(const std::string& msg, const char* file = "", int line = 0, const char* func = "");
    void debug(const std::string& msg, const char* file = "", int line = 0, const char* func = "");
    void info(const std::string& msg, const char* file = "", int line = 0, const char* func = "");
    void warn(const std::string& msg, const char* file = "", int line = 0, const char* func = "");
    void error(const std::string& msg, const char* file = "", int line = 0, const char* func = "");
    void fatal(const std::string& msg, const char* file = "", int line = 0, const char* func = "");
    
    // 刷新和关闭
    void flush();
    void close();
    
    // 获取统计信息
    struct Statistics {
        int64_t total_messages;
        int64_t messages_by_level[6];
        int64_t total_bytes_written;
        int current_file_index;
    };
    Statistics getStatistics() const;
    
    // 设置最小日志级别
    void setLevel(LogLevel level) { config_.min_level = level; }

private:
    FileLogger();
    ~FileLogger();
    SMARTSCHED_DISALLOW_COPY_AND_MOVE(FileLogger);
    
    void init();
    void writerThread();
    void rotateFile();
    bool shouldRotate();
    void cleanupOldFiles();
    std::string generateFilename(int index);
    std::string formatMessage(const LogMessage& msg);
    std::string getCurrentDate();
    
    FileLoggerConfig config_;
    std::ofstream file_;
    std::string current_file_path_;
    std::string current_date_;
    int current_file_index_;
    
    // 异步队列
    std::queue<LogMessage> queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::atomic<bool> running_;
    
    // 统计
    Statistics statistics_;
    mutable std::mutex stats_mutex_;
    
    // 线程
    std::thread writer_thread_;
};

// =============================================================================
// 宏定义（简化使用）
// =============================================================================
#define FILE_LOG_TRACE(msg) smartsched::utils::FileLogger::instance().trace(msg, __FILE__, __LINE__, __func__)
#define FILE_LOG_DEBUG(msg) smartsched::utils::FileLogger::instance().debug(msg, __FILE__, __LINE__, __func__)
#define FILE_LOG_INFO(msg) smartsched::utils::FileLogger::instance().info(msg, __FILE__, __LINE__, __func__)
#define FILE_LOG_WARN(msg) smartsched::utils::FileLogger::instance().warn(msg, __FILE__, __LINE__, __func__)
#define FILE_LOG_ERROR(msg) smartsched::utils::FileLogger::instance().error(msg, __FILE__, __LINE__, __func__)
#define FILE_LOG_FATAL(msg) smartsched::utils::FileLogger::instance().fatal(msg, __FILE__, __LINE__, __func__)

// =============================================================================
// 内联实现
// =============================================================================

inline FileLogger::FileLogger()
    : running_(true)
    , current_file_index_(0)
{
    statistics_ = {};
    init();
}

inline FileLogger::~FileLogger() {
    close();
}

inline FileLogger& FileLogger::instance() {
    static FileLogger logger;
    return logger;
}

inline void FileLogger::configure(const FileLoggerConfig& config) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    // 先关闭当前文件
    if (file_.is_open()) {
        file_.flush();
        file_.close();
    }
    
    config_ = config;
    
    // 重新初始化
    init();
}

inline void FileLogger::init() {
    // 创建日志目录
#ifdef _WIN32
    mkdir(config_.log_dir.c_str());
#else
    mkdir(config_.log_dir.c_str(), 0755);
#endif
    
    // 生成当前文件名
    current_date_ = getCurrentDate();
    current_file_path_ = generateFilename(current_file_index_);
    
    // 打开文件
    file_.open(current_file_path_, std::ios::app | std::ios::binary);
    if (!file_.is_open()) {
        // 尝试创建目录
#ifdef _WIN32
        system(("mkdir " + config_.log_dir).c_str());
#else
        system(("mkdir -p " + config_.log_dir).c_str());
#endif
        file_.open(current_file_path_, std::ios::app | std::ios::binary);
    }
    
    // 清理旧文件
    cleanupOldFiles();
    
    // 启动异步写入线程
    if (config_.async_mode) {
        running_ = true;
        writer_thread_ = std::thread(&FileLogger::writerThread, this);
    }
}

inline void FileLogger::writerThread() {
    std::vector<LogMessage> batch;
    batch.reserve(100);
    
    while (running_) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        
        // 等待消息或超时
        queue_cv_.wait_for(lock, 
            std::chrono::milliseconds(config_.flush_interval_ms),
            [this] { return !queue_.empty() || !running_; });
        
        // 批量取出消息
        while (!queue_.empty() && batch.size() < 100) {
            batch.push_back(std::move(queue_.front()));
            queue_.pop();
        }
        
        lock.unlock();
        
        // 写入文件
        if (!batch.empty()) {
            std::lock_guard<std::mutex> file_lock(stats_mutex_);
            
            for (const auto& msg : batch) {
                if (file_.is_open()) {
                    std::string formatted = formatMessage(msg);
                    file_.write(formatted.data(), formatted.size());
                    
                    statistics_.total_bytes_written += formatted.size();
                    statistics_.total_messages++;
                    statistics_.messages_by_level[static_cast<int>(msg.level)]++;
                }
            }
            
            file_.flush();
            batch.clear();
            
            // 检查是否需要轮转
            if (shouldRotate()) {
                rotateFile();
            }
        }
    }
}

inline void FileLogger::log(LogLevel level, const char* file, int line, 
                           const char* function, const std::string& message) {
    if (level < config_.min_level) return;
    
    LogMessage msg(level, file, line, function, message);
    msg.thread_id = static_cast<int>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    
    if (config_.async_mode) {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        queue_.push(std::move(msg));
        queue_cv_.notify_one();
        
        // 队列满时同步写入
        if (queue_.size() >= static_cast<size_t>(config_.queue_size)) {
            // 这里简单处理，实际可以用条件变量阻塞
        }
    } else {
        // 同步模式
        std::lock_guard<std::mutex> lock(stats_mutex_);
        
        if (file_.is_open()) {
            std::string formatted = formatMessage(msg);
            file_.write(formatted.data(), formatted.size());
            file_.flush();
            
            statistics_.total_bytes_written += formatted.size();
            statistics_.total_messages++;
            statistics_.messages_by_level[static_cast<int>(msg.level)]++;
            
            if (shouldRotate()) {
                rotateFile();
            }
        }
    }
}

inline void FileLogger::flush() {
    if (config_.async_mode) {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        // 等待队列清空（简化处理）
    }
    
    std::lock_guard<std::mutex> lock(stats_mutex_);
    if (file_.is_open()) {
        file_.flush();
    }
}

inline void FileLogger::close() {
    running_ = false;
    queue_cv_.notify_one();
    
    if (writer_thread_.joinable()) {
        writer_thread_.join();
    }
    
    // 写入剩余消息
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        while (!queue_.empty()) {
            auto msg = queue_.front();
            queue_.pop();
            
            if (file_.is_open()) {
                std::string formatted = formatMessage(msg);
                file_.write(formatted.data(), formatted.size());
            }
        }
    }
    
    std::lock_guard<std::mutex> lock(stats_mutex_);
    if (file_.is_open()) {
        file_.flush();
        file_.close();
    }
}

inline std::string FileLogger::formatMessage(const LogMessage& msg) {
    std::ostringstream oss;
    
    // 时间戳
    auto time = std::chrono::system_clock::to_time_t(msg.timestamp);
    char time_buf[32];
    struct tm tm_buf;
    
    if (config_.use_local_time) {
#ifdef _WIN32
        localtime_s(&tm_buf, &time);
#else
        localtime_r(&time, &tm_buf);
#endif
        std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &tm_buf);
    } else {
        gmtime_s(&tm_buf, &time);
        std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &tm_buf);
    }
    
    // 毫秒
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        msg.timestamp.time_since_epoch()) % 1000;
    
    oss << "[" << time_buf << "." << std::setfill('0') << std::setw(3) << ms.count() << "]";
    
    // 级别
    oss << " [" << logLevelToString(msg.level) << "]";
    
    // 线程ID
    oss << " [T" << std::setw(4) << std::setfill('0') << msg.thread_id << "]";
    
    // 文件和行号
    if (!msg.file.empty()) {
        std::string filename = msg.file;
        size_t pos = filename.find_last_of("/\\");
        if (pos != std::string::npos) {
            filename = filename.substr(pos + 1);
        }
        oss << " [" << filename << ":" << msg.line << "]";
    }
    
    // 函数名
    if (!msg.function.empty()) {
        oss << " (" << msg.function << ")";
    }
    
    // 消息
    oss << " " << msg.message;
    
    // 换行
    oss << "\n";
    
    return oss.str();
}

inline bool FileLogger::shouldRotate() {
    if (!file_.is_open()) return false;
    
    // 检查文件大小
    auto pos = file_.tellp();
    return pos >= config_.max_file_size;
}

inline void FileLogger::rotateFile() {
    if (file_.is_open()) {
        file_.flush();
        file_.close();
    }
    
    // 增加文件索引
    current_file_index_++;
    
    // 检查是否需要循环
    if (current_file_index_ >= config_.max_file_count) {
        current_file_index_ = 0;
    }
    
    // 生成新文件名
    current_file_path_ = generateFilename(current_file_index_);
    
    // 打开新文件
    file_.open(current_file_path_, std::ios::trunc | std::ios::binary);
}

inline void FileLogger::cleanupOldFiles() {
    // 获取当前日期之前的日期列表
    std::string current_date = getCurrentDate();
    
    // 删除过期的日志文件
    for (int i = 0; i < config_.max_file_count; i++) {
        std::string filename = generateFilename(i);
        
        // 检查文件是否存在
        std::ifstream test(filename, std::ios::binary);
        if (!test.is_open()) continue;
        test.close();
        
        // 检查文件修改时间
        std::time_t file_time = 0;
        struct stat st;
        if (stat(filename.c_str(), &st) == 0) {
            file_time = st.st_mtime;
        }
        
        // 计算文件年龄
        auto now = std::chrono::system_clock::now();
        auto age = std::chrono::duration_cast<std::chrono::hours>(
            now - std::chrono::system_clock::from_time_t(file_time)).count();
        
        // 删除超过保留期限的文件
        if (age > config_.max_keep_days * 24) {
            std::remove(filename.c_str());
        }
    }
}

inline std::string FileLogger::generateFilename(int index) {
    std::ostringstream oss;
    oss << config_.log_dir << "/"
        << config_.prefix << "_" 
        << current_date_ << "_" 
        << std::setw(3) << std::setfill('0') << index 
        << ".log";
    return oss.str();
}

inline std::string FileLogger::getCurrentDate() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    char buf[32];
    struct tm tm_buf;
    
#ifdef _WIN32
    localtime_s(&tm_buf, &time);
#else
    localtime_r(&time, &tm_buf);
#endif
    
    std::strftime(buf, sizeof(buf), "%Y%m%d", &tm_buf);
    return std::string(buf);
}

inline FileLogger::Statistics FileLogger::getStatistics() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return statistics_;
}

// 便捷方法实现
inline void FileLogger::trace(const std::string& msg, const char* file, int line, const char* func) {
    log(LogLevel::Trace, file, line, func, msg);
}
inline void FileLogger::debug(const std::string& msg, const char* file, int line, const char* func) {
    log(LogLevel::Debug, file, line, func, msg);
}
inline void FileLogger::info(const std::string& msg, const char* file, int line, const char* func) {
    log(LogLevel::Info, file, line, func, msg);
}
inline void FileLogger::warn(const std::string& msg, const char* file, int line, const char* func) {
    log(LogLevel::Warn, file, line, func, msg);
}
inline void FileLogger::error(const std::string& msg, const char* file, int line, const char* func) {
    log(LogLevel::Error, file, line, func, msg);
}
inline void FileLogger::fatal(const std::string& msg, const char* file, int line, const char* func) {
    log(LogLevel::Fatal, file, line, func, msg);
}

} // namespace utils
} // namespace smartsched
