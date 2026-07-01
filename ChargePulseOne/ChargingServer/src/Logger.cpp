#include "Logger.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <sys/stat.h>
#include <cstring>

#ifdef _WIN32
    #include <direct.h>
    #include <io.h>
    #define getcwd _getcwd
    #define stat _stat
#else
    #include <unistd.h>
    #include <sys/stat.h>
#endif

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

Logger::Logger() 
    : m_minLevel(INFO)
    , m_consoleOutput(true)
    , m_currentSize(0)
    , m_maxSizeMB(10)
    , m_maxFiles(5)
{
}

Logger::~Logger() {
    flush();
    closeFile();
}

void Logger::log(LogLevel level, const std::string& msg) {
    std::ostringstream oss;
    oss << "[" << timestamp() << "] [" << levelStr(level) << "] " << msg;
    std::string full = oss.str();
    
    std::lock_guard<std::mutex> lock(m_mtx);
    
    if (m_consoleOutput) {
        if (level >= ERROR) {
            std::cerr << full << std::endl;
        } else {
            std::cout << full << std::endl;
        }
    }
    
    if (m_file.is_open() && level >= m_minLevel) {
        m_file << full << std::endl;
        m_currentSize += full.length() + 1;
        checkRotation();
    }
}

void Logger::log(LogLevel level, const std::string& file, int line, const std::string& func, const std::string& msg) {
    if (level < m_minLevel) return;
    
    // 提取文件名
    std::string filename = file;
    size_t pos = file.find_last_of("/\\");
    if (pos != std::string::npos) {
        filename = file.substr(pos + 1);
    }
    
    std::ostringstream oss;
    oss << "[" << timestamp() << "] [" << levelStr(level) << "] [" 
        << filename << ":" << line << " " << func << "] " << msg;
    std::string full = oss.str();
    
    std::lock_guard<std::mutex> lock(m_mtx);
    
    if (m_consoleOutput) {
        if (level >= ERROR) {
            std::cerr << full << std::endl;
        } else {
            std::cout << full << std::endl;
        }
    }
    
    if (m_file.is_open()) {
        m_file << full << std::endl;
        m_currentSize += full.length() + 1;
        checkRotation();
    }
}

void Logger::debug(const std::string& msg) { log(DEBUG, msg); }
void Logger::info(const std::string& msg) { log(INFO, msg); }
void Logger::warn(const std::string& msg) { log(WARN, msg); }
void Logger::error(const std::string& msg) { log(ERROR, msg); }
void Logger::fatal(const std::string& msg) { log(FATAL, msg); }

void Logger::setFile(const std::string& path) {
    setFile(path, 10, 5);
}

void Logger::setFile(const std::string& path, int maxSizeMB, int maxFiles) {
    std::lock_guard<std::mutex> lock(m_mtx);
    
    closeFile();
    
    m_logFilePath = path;
    m_maxSizeMB = maxSizeMB;
    m_maxFiles = maxFiles;
    
    // 确保目录存在
    std::string dir = path;
    size_t pos = path.find_last_of("/\\");
    if (pos != std::string::npos) {
        dir = path.substr(0, pos);
#ifdef _WIN32
        _mkdir(dir.c_str());
#else
        mkdir(dir.c_str(), 0755);
#endif
    }
    
    // 检查现有文件大小
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        m_currentSize = st.st_size;
    } else {
        m_currentSize = 0;
    }
    
    m_file.open(path, std::ios::app);
    if (!m_file) {
        std::cerr << "Cannot open log file: " << path << std::endl;
    }
}

void Logger::closeFile() {
    if (m_file.is_open()) {
        m_file.flush();
        m_file.close();
    }
}

void Logger::setLevel(LogLevel level) {
    m_minLevel = level;
}

void Logger::setConsoleOutput(bool enable) {
    m_consoleOutput = enable;
}

void Logger::flush() {
    std::lock_guard<std::mutex> lock(m_mtx);
    if (m_file.is_open()) {
        m_file.flush();
    }
}

uint64_t Logger::logSize() const {
    return m_currentSize;
}

void Logger::checkRotation() {
    if (m_currentSize >= (uint64_t)m_maxSizeMB * 1024 * 1024) {
        rotate();
    }
}

void Logger::rotate() {
    if (!m_file.is_open()) return;
    
    m_file.flush();
    m_file.close();
    
    // 删除最旧的备份
    std::ostringstream oldestPath;
    oldestPath << m_logFilePath << "." << m_maxFiles;
    remove(oldestPath.str().c_str());
    
    // 移动现有备份
    for (int i = m_maxFiles - 1; i >= 1; --i) {
        std::ostringstream from, to;
        from << m_logFilePath << "." << i;
        to << m_logFilePath << "." << (i + 1);
        rename(from.str().c_str(), to.str().c_str());
    }
    
    // 重命名当前文件
    rename(m_logFilePath.c_str(), (m_logFilePath + ".1").c_str());
    
    // 重新打开
    m_file.open(m_logFilePath, std::ios::app);
    m_currentSize = 0;
}

std::string Logger::levelStr(LogLevel lv) {
    switch(lv) {
        case DEBUG: return "DEBUG";
        case INFO:  return "INFO ";
        case WARN:  return "WARN ";
        case ERROR: return "ERROR";
        case FATAL: return "FATAL";
    }
    return "UNKWN";
}

void Logger::writeToFile(const std::string& msg) {
    if (m_file.is_open()) {
        m_file << msg << std::endl;
        m_currentSize += msg.length() + 1;
        checkRotation();
    }
}
