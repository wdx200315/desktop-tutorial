#pragma once
#include "common.h"
#include <fstream>
#include <mutex>
#include <sstream>
#include <queue>
#include <atomic>

enum LogLevel { 
    DEBUG = 0, 
    INFO = 1, 
    WARN = 2, 
    ERROR = 3,
    FATAL = 4
};

class Logger {
public:
    static Logger& instance();
    
    // 基础日志方法
    void log(LogLevel level, const std::string& msg);
    void log(LogLevel level, const std::string& file, int line, const std::string& func, const std::string& msg);
    
    // 便捷日志方法
    void debug(const std::string& msg);
    void info(const std::string& msg);
    void warn(const std::string& msg);
    void error(const std::string& msg);
    void fatal(const std::string& msg);
    
    // 文件配置
    void setFile(const std::string& path);
    void setFile(const std::string& path, int maxSizeMB, int maxFiles);
    void closeFile();
    
    // 日志级别控制
    void setLevel(LogLevel level);
    LogLevel level() const { return m_minLevel; }
    
    // 控制台输出控制
    void setConsoleOutput(bool enable);
    bool consoleOutput() const { return m_consoleOutput; }
    
    // 刷新缓冲区
    void flush();
    
    // 获取日志文件路径
    std::string logFilePath() const { return m_logFilePath; }
    
    // 获取当前日志大小
    uint64_t logSize() const;
    
    // 日志轮转
    void rotate();

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    std::string levelStr(LogLevel lv);
    void writeToFile(const std::string& msg);
    void checkRotation();
    
    std::ofstream m_file;
    std::mutex m_mtx;
    std::atomic<LogLevel> m_minLevel;
    std::atomic<bool> m_consoleOutput;
    std::string m_logFilePath;
    std::atomic<uint64_t> m_currentSize;
    int m_maxSizeMB;
    int m_maxFiles;
};

// 便捷宏定义
#define LOG_DEBUG(msg) Logger::instance().log(DEBUG, __FILE__, __LINE__, __FUNCTION__, msg)
#define LOG_INFO(msg) Logger::instance().log(INFO, __FILE__, __LINE__, __FUNCTION__, msg)
#define LOG_WARN(msg) Logger::instance().log(WARN, __FILE__, __LINE__, __FUNCTION__, msg)
#define LOG_ERROR(msg) Logger::instance().log(ERROR, __FILE__, __LINE__, __FUNCTION__, msg)
#define LOG_FATAL(msg) Logger::instance().log(FATAL, __FILE__, __LINE__, __FUNCTION__, msg)
