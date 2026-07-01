/**
 * @file config.cpp
 * @brief 配置管理系统实现
 */
#include "config.h"
#include "json.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cstdlib>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
    #include <sys/types.h>
#endif

using json = nlohmann::json;

namespace smartsched {

// ==================== ConfigManager 实现 ====================
ConfigManager& ConfigManager::instance()
{
    static ConfigManager instance;
    return instance;
}

ConfigManager::ConfigManager()
{
    // 设置默认值
}

bool ConfigManager::loadFromFile(const std::string& filepath)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open config file: " << filepath << std::endl;
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    loadFromJson(buffer.str());
    
    return true;
}

void ConfigManager::loadFromJson(const std::string& jsonStr)
{
    try {
        auto j = json::parse(jsonStr);
        
        // 服务端配置
        if (j.contains("server")) {
            auto& s = j["server"];
            if (s.contains("host")) m_serverConfig.host = s["host"];
            if (s.contains("port")) m_serverConfig.port = s["port"];
            if (s.contains("max_connections")) m_serverConfig.max_connections = s["max_connections"];
            if (s.contains("thread_pool_size")) m_serverConfig.thread_pool_size = s["thread_pool_size"];
            
            if (s.contains("db_host")) m_serverConfig.db_host = s["db_host"];
            if (s.contains("db_port")) m_serverConfig.db_port = s["db_port"];
            if (s.contains("db_name")) m_serverConfig.db_name = s["db_name"];
            if (s.contains("db_user")) m_serverConfig.db_user = s["db_user"];
            if (s.contains("db_password")) m_serverConfig.db_password = s["db_password"];
            if (s.contains("db_pool_size")) m_serverConfig.db_pool_size = s["db_pool_size"];
            
            if (s.contains("encryption_enabled")) m_serverConfig.encryption_enabled = s["encryption_enabled"];
            if (s.contains("aes_key")) m_serverConfig.aes_key = s["aes_key"];
            if (s.contains("aes_iv")) m_serverConfig.aes_iv = s["aes_iv"];
            
            if (s.contains("log_dir")) m_serverConfig.log_dir = s["log_dir"];
            if (s.contains("log_level")) m_serverConfig.log_level = s["log_level"];
            if (s.contains("log_max_size")) m_serverConfig.log_max_size = s["log_max_size"];
            if (s.contains("log_max_files")) m_serverConfig.log_max_files = s["log_max_files"];
            if (s.contains("log_async")) m_serverConfig.log_async = s["log_async"];
            
            if (s.contains("queue_capacity")) m_serverConfig.queue_capacity = s["queue_capacity"];
            if (s.contains("max_wait_time")) m_serverConfig.max_wait_time = s["max_wait_time"];
            if (s.contains("call_timeout")) m_serverConfig.call_timeout = s["call_timeout"];
            if (s.contains("auto_reassign")) m_serverConfig.auto_reassign = s["auto_reassign"];
        }
        
        // 客户端配置
        if (j.contains("client")) {
            auto& c = j["client"];
            if (c.contains("server_host")) m_clientConfig.server_host = c["server_host"];
            if (c.contains("server_port")) m_clientConfig.server_port = c["server_port"];
            if (c.contains("auto_reconnect")) m_clientConfig.auto_reconnect = c["auto_reconnect"];
            if (c.contains("refresh_interval")) m_clientConfig.refresh_interval = c["refresh_interval"];
            if (c.contains("sound_enabled")) m_clientConfig.sound_enabled = c["sound_enabled"];
            if (c.contains("role")) m_clientConfig.role = c["role"];
        }
        
        // 自定义配置
        if (j.contains("custom")) {
            for (auto& [key, value] : j["custom"].items()) {
                m_customConfig[key] = value.get<std::string>();
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse config JSON: " << e.what() << std::endl;
    }
}

bool ConfigManager::saveToFile(const std::string& filepath) const
{
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to create config file: " << filepath << std::endl;
        return false;
    }
    
    file << saveToJson();
    return true;
}

std::string ConfigManager::saveToJson() const
{
    json j;
    
    // 服务端配置
    j["server"] = {
        {"host", m_serverConfig.host},
        {"port", m_serverConfig.port},
        {"max_connections", m_serverConfig.max_connections},
        {"thread_pool_size", m_serverConfig.thread_pool_size},
        {"db_host", m_serverConfig.db_host},
        {"db_port", m_serverConfig.db_port},
        {"db_name", m_serverConfig.db_name},
        {"db_user", m_serverConfig.db_user},
        {"db_password", m_serverConfig.db_password},  // 注意：生产环境应加密
        {"db_pool_size", m_serverConfig.db_pool_size},
        {"encryption_enabled", m_serverConfig.encryption_enabled},
        {"aes_key", m_serverConfig.aes_key},
        {"aes_iv", m_serverConfig.aes_iv},
        {"log_dir", m_serverConfig.log_dir},
        {"log_level", m_serverConfig.log_level},
        {"log_max_size", m_serverConfig.log_max_size},
        {"log_max_files", m_serverConfig.log_max_files},
        {"log_async", m_serverConfig.log_async},
        {"queue_capacity", m_serverConfig.queue_capacity},
        {"max_wait_time", m_serverConfig.max_wait_time},
        {"call_timeout", m_serverConfig.call_timeout},
        {"auto_reassign", m_serverConfig.auto_reassign}
    };
    
    // 客户端配置
    j["client"] = {
        {"server_host", m_clientConfig.server_host},
        {"server_port", m_clientConfig.server_port},
        {"auto_reconnect", m_clientConfig.auto_reconnect},
        {"reconnect_interval", m_clientConfig.reconnect_interval},
        {"connection_timeout", m_clientConfig.connection_timeout},
        {"encryption_enabled", m_clientConfig.encryption_enabled},
        {"refresh_interval", m_clientConfig.refresh_interval},
        {"sound_enabled", m_clientConfig.sound_enabled},
        {"notifications_enabled", m_clientConfig.notifications_enabled},
        {"role", m_clientConfig.role},
        {"user_id", m_clientConfig.user_id}
    };
    
    // 自定义配置
    if (!m_customConfig.empty()) {
        json custom;
        for (const auto& [key, value] : m_customConfig) {
            custom[key] = value;
        }
        j["custom"] = custom;
    }
    
    return j.dump(4);
}

void ConfigManager::parseArgs(int argc, char* argv[])
{
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        // --config <file>
        if (arg == "--config" && i + 1 < argc) {
            loadFromFile(argv[++i]);
            continue;
        }
        
        // --server-host, --server-port, etc.
        if (arg.rfind("--", 0) == 0) {
            std::string key = arg.substr(2);
            std::replace(key.begin(), key.end(), '_', '-');
            
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                set(key, argv[++i]);
            } else {
                set(key, "true");
            }
        }
        
        // -h, -p, etc.
        if (arg.size() == 2 && arg[0] == '-') {
            char c = arg[1];
            switch (c) {
                case 'h':
                    if (i + 1 < argc) {
                        m_clientConfig.server_host = argv[++i];
                        m_serverConfig.host = argv[i];
                    }
                    break;
                case 'p':
                    if (i + 1 < argc) {
                        m_clientConfig.server_port = std::stoi(argv[++i]);
                        m_serverConfig.port = m_clientConfig.server_port;
                    }
                    break;
            }
        }
    }
}

std::string ConfigManager::get(const std::string& key, const std::string& defaultValue) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_customConfig.count(key)) {
        return m_customConfig.at(key);
    }
    return defaultValue;
}

void ConfigManager::set(const std::string& key, const std::string& value)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // 解析嵌套key如 "server.host"
    std::string prefix = key.substr(0, key.find('.'));
    std::string subkey = key.substr(key.find('.') + 1);
    
    if (prefix == "server" || prefix == "s") {
        if (subkey == "host") m_serverConfig.host = value;
        else if (subkey == "port") m_serverConfig.port = std::stoi(value);
        else if (subkey == "db_host") m_serverConfig.db_host = value;
        else if (subkey == "db_port") m_serverConfig.db_port = std::stoi(value);
        else if (subkey == "db_name") m_serverConfig.db_name = value;
        else if (subkey == "db_user") m_serverConfig.db_user = value;
        else if (subkey == "db_password") m_serverConfig.db_password = value;
        else if (subkey == "log_level") m_serverConfig.log_level = value;
        else m_customConfig[key] = value;
    } else if (prefix == "client" || prefix == "c") {
        if (subkey == "server_host") m_clientConfig.server_host = value;
        else if (subkey == "server_port") m_clientConfig.server_port = std::stoi(value);
        else if (subkey == "role") m_clientConfig.role = value;
        else m_customConfig[key] = value;
    } else {
        m_customConfig[key] = value;
    }
}

bool ConfigManager::validate() const
{
    return getValidationErrors().empty();
}

std::vector<std::string> ConfigManager::getValidationErrors() const
{
    std::vector<std::string> errors;
    
    // 服务端验证
    if (m_serverConfig.port < 1024 || m_serverConfig.port > 65535) {
        errors.push_back("Server port must be between 1024 and 65535");
    }
    if (m_serverConfig.thread_pool_size < 1 || m_serverConfig.thread_pool_size > 32) {
        errors.push_back("Thread pool size must be between 1 and 32");
    }
    if (m_serverConfig.db_pool_size < 1 || m_serverConfig.db_pool_size > 100) {
        errors.push_back("Database pool size must be between 1 and 100");
    }
    if (m_serverConfig.aes_key.length() != 32) {
        errors.push_back("AES key must be exactly 32 bytes");
    }
    if (m_serverConfig.aes_iv.length() != 16) {
        errors.push_back("AES IV must be exactly 16 bytes");
    }
    
    // 客户端验证
    if (m_clientConfig.server_port < 1024 || m_clientConfig.server_port > 65535) {
        errors.push_back("Server port must be between 1024 and 65535");
    }
    if (m_clientConfig.refresh_interval < 1) {
        errors.push_back("Refresh interval must be at least 1 second");
    }
    
    return errors;
}

std::string ConfigManager::getEnv(const std::string& key, const std::string& defaultValue) const
{
#ifdef _WIN32
    char buffer[32767];
    DWORD len = GetEnvironmentVariableA(key.c_str(), buffer, sizeof(buffer));
    if (len > 0 && len < sizeof(buffer)) {
        return std::string(buffer, len);
    }
#else
    const char* val = getenv(key.c_str());
    if (val) {
        return std::string(val);
    }
#endif
    return defaultValue;
}

void ConfigManager::printConfig() const
{
    std::cout << "========== Configuration ==========\n";
    std::cout << "Server:\n";
    std::cout << "  Host: " << m_serverConfig.host << "\n";
    std::cout << "  Port: " << m_serverConfig.port << "\n";
    std::cout << "  Threads: " << m_serverConfig.thread_pool_size << "\n";
    std::cout << "  DB: " << m_serverConfig.db_host << ":" << m_serverConfig.db_port 
              << "/" << m_serverConfig.db_name << "\n";
    std::cout << "  Encryption: " << (m_serverConfig.encryption_enabled ? "ON" : "OFF") << "\n";
    std::cout << "  Log: " << m_serverConfig.log_dir << " (" << m_serverConfig.log_level << ")\n";
    
    std::cout << "\nClient:\n";
    std::cout << "  Server: " << m_clientConfig.server_host << ":" << m_clientConfig.server_port << "\n";
    std::cout << "  Role: " << m_clientConfig.role << "\n";
    std::cout << "  Refresh: " << m_clientConfig.refresh_interval << "s\n";
    std::cout << "==================================\n";
}

// ==================== ArgParser 实现 ====================
std::vector<ArgParser::Option> ArgParser::s_options;
std::map<std::string, std::string> ArgParser::s_values;

void ArgParser::addOption(const std::string& shortName,
                          const std::string& longName,
                          const std::string& description,
                          bool hasValue,
                          const std::string& defaultValue)
{
    Option opt;
    opt.shortName = shortName;
    opt.longName = longName;
    opt.description = description;
    opt.hasValue = hasValue;
    opt.defaultValue = defaultValue;
    s_options.push_back(opt);
}

bool ArgParser::parse(int argc, char* argv[])
{
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        // 查找匹配的选项
        for (const auto& opt : s_options) {
            bool matched = false;
            
            if (!opt.shortName.empty() && arg == "-" + opt.shortName) {
                matched = true;
            } else if (!opt.longName.empty() && arg == "--" + opt.longName) {
                matched = true;
            }
            
            if (matched) {
                if (opt.hasValue) {
                    if (i + 1 < argc) {
                        s_values[opt.longName] = argv[++i];
                    } else {
                        std::cerr << "Option --" << opt.longName << " requires a value\n";
                        return false;
                    }
                } else {
                    s_values[opt.longName] = "true";
                }
                break;
            }
        }
        
        // 非选项参数（位置参数）
        if (arg[0] != '-') {
            s_values["_positional"] += arg + " ";
        }
    }
    
    return true;
}

std::string ArgParser::get(const std::string& name, const std::string& defaultValue)
{
    auto it = s_values.find(name);
    if (it != s_values.end()) {
        return it->second;
    }
    return defaultValue;
}

bool ArgParser::has(const std::string& name)
{
    return s_values.count(name) > 0;
}

void ArgParser::printHelp(const std::string& programName)
{
    std::cout << "Usage: " << programName << " [OPTIONS]\n\n";
    std::cout << "Options:\n";
    
    for (const auto& opt : s_options) {
        std::string shortForm = opt.shortName.empty() ? "    " : "-" + opt.shortName + ", ";
        std::string longForm = "--" + opt.longName;
        if (opt.hasValue) {
            longForm += " <value>";
        }
        std::cout << "  " << shortForm << std::left << std::setw(24) << longForm;
        std::cout << " " << opt.description << "\n";
    }
}

} // namespace smartsched
