/**
 * @file config.h
 * @brief 配置管理系统 - JSON配置文件读写、命令行参数解析
 */
#ifndef SMARTSCHED_CONFIG_H
#define SMARTSCHED_CONFIG_H

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <mutex>

namespace smartsched {

/**
 * @brief 服务端配置结构
 */
struct ServerConfig {
    // 网络配置
    std::string host = "0.0.0.0";
    int port = 8888;
    int max_connections = 1024;
    int thread_pool_size = 4;
    
    // 数据库配置
    std::string db_host = "localhost";
    int db_port = 3306;
    std::string db_name = "smartsched";
    std::string db_user = "root";
    std::string db_password = "";
    int db_pool_size = 10;
    
    // 安全配置
    bool encryption_enabled = true;
    std::string aes_key = "32BytesLongSecretKeyHere!!";  // 256-bit
    std::string aes_iv = "16BytesInitVect";              // 128-bit
    
    // 日志配置
    std::string log_dir = "logs";
    std::string log_level = "info";
    int log_max_size = 10 * 1024 * 1024;  // 10MB
    int log_max_files = 10;
    bool log_async = true;
    
    // 业务配置
    int queue_capacity = 1000;
    int max_wait_time = 120;        // 分钟
    int call_timeout = 300;         // 秒
    bool auto_reassign = true;      // 自动重新分配
    
    // 监控配置
    bool stats_enabled = true;
    int stats_interval = 60;        // 秒
};

/**
 * @brief 客户端配置结构
 */
struct ClientConfig {
    // 服务器连接
    std::string server_host = "localhost";
    int server_port = 8888;
    bool auto_reconnect = true;
    int reconnect_interval = 5;     // 秒
    int connection_timeout = 10;    // 秒
    
    // 安全配置
    bool encryption_enabled = true;
    
    // UI配置
    int refresh_interval = 2;       // 秒
    bool sound_enabled = true;
    bool notifications_enabled = true;
    
    // 角色配置
    std::string role;               // patient, doctor, admin
    int user_id = 0;
    std::string auth_token;
};

/**
 * @brief 配置管理器 - 单例模式
 */
class ConfigManager {
public:
    static ConfigManager& instance();
    
    // 服务端配置
    const ServerConfig& server() const { return m_serverConfig; }
    ServerConfig& server() { return m_serverConfig; }
    
    // 客户端配置
    const ClientConfig& client() const { return m_clientConfig; }
    ClientConfig& client() { return m_clientConfig; }
    
    // 文件操作
    bool loadFromFile(const std::string& filepath);
    bool saveToFile(const std::string& filepath) const;
    
    // 命令行参数解析
    void parseArgs(int argc, char* argv[]);
    
    // 便捷访问
    std::string get(const std::string& key, const std::string& defaultValue = "") const;
    void set(const std::string& key, const std::string& value);
    
    // 配置验证
    bool validate() const;
    std::vector<std::string> getValidationErrors() const;
    
    // 环境变量支持
    std::string getEnv(const std::string& key, const std::string& defaultValue = "") const;
    
    // 打印当前配置（脱敏）
    void printConfig() const;

private:
    ConfigManager();
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    
    void loadFromJson(const std::string& json);
    std::string saveToJson() const;
    
    ServerConfig m_serverConfig;
    ClientConfig m_clientConfig;
    std::map<std::string, std::string> m_customConfig;
    mutable std::mutex m_mutex;
};

/**
 * @brief 命令行参数解析器
 */
class ArgParser {
public:
    struct Option {
        std::string shortName;
        std::string longName;
        std::string description;
        bool hasValue;
        std::string defaultValue;
    };
    
    static void addOption(const std::string& shortName, 
                          const std::string& longName,
                          const std::string& description,
                          bool hasValue = false,
                          const std::string& defaultValue = "");
    
    static bool parse(int argc, char* argv[]);
    static std::string get(const std::string& name, const std::string& defaultValue = "");
    static bool has(const std::string& name);
    static void printHelp(const std::string& programName);
    
private:
    static std::vector<Option> s_options;
    static std::map<std::string, std::string> s_values;
};

} // namespace smartsched

#endif // SMARTSCHED_CONFIG_H
