#pragma once
#include "common.h"
#include "CryptoUtils.h"
#include <map>
#include <string>
#include <cstdlib>

class ConfigManager {
public:
    static ConfigManager& instance();
    void load(const std::string& file);
    void save();
    std::string get(const std::string& key, const std::string& def = "") const;
    void set(const std::string& key, const std::string& val);
    
    std::string getDBHost()    const { return getWithEnv("db_host", "CHARGE_DB_HOST", "127.0.0.1"); }
    std::string getDBUser()    const { return getWithEnv("db_user", "CHARGE_DB_USER", "root"); }
    std::string getDBPass()    const { return getWithEnv("db_pass", "CHARGE_DB_PASS", ""); }
    std::string getDBName()    const { return getWithEnv("db_name", "CHARGE_DB_NAME", "chargepulse"); }
    int getDBPort()            const { return std::stoi(getWithEnv("db_port", "CHARGE_DB_PORT", "3306")); }
    int getServerPort()        const { return std::stoi(getWithEnv("server_port", "CHARGE_SERVER_PORT", "9010")); }

private:
    ConfigManager() = default;
    std::map<std::string, std::string> kv;
    std::string path;
    std::string encKey = "ChargePulse2026Key!@#$%^&*()"; // 32字节
    std::string encIv  = "ChargePulseIV_16";             // 16字节

    std::string getWithEnv(const std::string& key, const std::string& envVar, const std::string& defaultVal) const {
        // 优先使用环境变量
        const char* env = std::getenv(envVar.c_str());
        if (env && strlen(env) > 0) return std::string(env);
        // 其次使用配置文件中的值
        auto it = kv.find(key);
        if (it != kv.end() && !it->second.empty()) return it->second;
        return defaultVal;
    }
};
