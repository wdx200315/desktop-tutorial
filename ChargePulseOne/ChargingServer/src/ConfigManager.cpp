#include "ConfigManager.h"
#include "Logger.h"

ConfigManager& ConfigManager::instance() {
    static ConfigManager inst;
    return inst;
}

void ConfigManager::load(const std::string& file) {
    path = file;
    std::ifstream in(path, std::ios::binary);
    if(!in) {
        Logger::instance().log(WARN, "Config not found, using defaults.");
        // 设置默认值
        kv["db_host"] = "127.0.0.1";
        kv["db_port"] = "3306";
        kv["db_user"] = "root";
        kv["db_pass"] = "123456";
        kv["db_name"] = "chargepulse";
        kv["server_port"] = "9010";
        return;
    }
    std::stringstream buf;
    buf << in.rdbuf();
    std::string cipher = buf.str();
    std::string plain = CryptoUtils::aesDecrypt(cipher, encKey, encIv);
    // 解析 key=value\n
    std::istringstream iss(plain);
    std::string line;
    while(std::getline(iss, line)) {
        if(line.empty()) continue;
        size_t pos = line.find('=');
        if(pos != std::string::npos) {
            kv[line.substr(0, pos)] = line.substr(pos+1);
        }
    }
}

void ConfigManager::save() {
    std::ostringstream oss;
    for(const auto& p : kv) oss << p.first << "=" << p.second << "\n";
    std::string plain = oss.str();
    std::string cipher = CryptoUtils::aesEncrypt(plain, encKey, encIv);
    std::ofstream out(path, std::ios::binary);
    if(!out) Logger::instance().log(ERROR, "Cannot save config.");
    out.write(cipher.c_str(), cipher.size());
}

std::string ConfigManager::get(const std::string& key, const std::string& def) const {
    auto it = kv.find(key);
    return it != kv.end() ? it->second : def;
}

void ConfigManager::set(const std::string& key, const std::string& val) {
    kv[key] = val;
}
