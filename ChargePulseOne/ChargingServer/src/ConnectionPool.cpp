#include "ConnectionPool.h"
#include "Logger.h"
#include <stdexcept>
#include "ConfigManager.h"

ConnectionPool& ConnectionPool::instance() {
    static ConnectionPool inst;
    return inst;
}

void ConnectionPool::init(const std::string& host, const std::string& user, const std::string& pass, 
                          const std::string& db, int port, int poolSize) {
    std::lock_guard<std::mutex> lock(mtx);
    if(initialized) return;
    
    for(int i=0;i<poolSize;++i) {
        MYSQL* conn = mysql_init(nullptr);
        if(!conn) throw std::runtime_error("mysql_init failed");
        // 设置自动重连和UTF-8
        bool reconnect = true;
        mysql_options(conn, MYSQL_OPT_RECONNECT, &reconnect);
        mysql_options(conn, MYSQL_SET_CHARSET_NAME, "utf8mb4");
        
        if(!mysql_real_connect(conn, host.c_str(), user.c_str(), pass.c_str(), 
                               db.c_str(), port, nullptr, CLIENT_MULTI_STATEMENTS)) {
            std::string err = mysql_error(conn);
            mysql_close(conn);
            throw std::runtime_error("mysql_real_connect: " + err);
        }
        pool.push(conn);
    }
    size = poolSize;
    initialized = true;
    Logger::instance().log(INFO, "Connection pool initialized with " + std::to_string(poolSize) + " connections");
}

MYSQL* ConnectionPool::acquire() {
    std::unique_lock<std::mutex> lock(mtx);
    while(pool.empty()) {
        cv.wait(lock);
    }
    MYSQL* conn = pool.front();
    pool.pop();
    // 检查连接有效性，若断开则重连
    if(mysql_ping(conn) != 0) {
        Logger::instance().log(WARN, "MYSQL connection lost, attempting reconnect");
        // 简单重连（生产环境应复杂处理）
        mysql_close(conn);
        conn = mysql_init(nullptr);
        bool reconnect = true;
        mysql_options(conn, MYSQL_OPT_RECONNECT, &reconnect);
        std::string host = ConfigManager::instance().getDBHost();
        std::string user = ConfigManager::instance().getDBUser();
        std::string pass = ConfigManager::instance().getDBPass();
        std::string db   = ConfigManager::instance().getDBName();
        int port = ConfigManager::instance().getDBPort();
        if(!mysql_real_connect(conn, host.c_str(), user.c_str(), pass.c_str(), 
                               db.c_str(), port, nullptr, 0)) {
            Logger::instance().log(ERROR, "Reconnect failed: " + std::string(mysql_error(conn)));
            mysql_close(conn);
            conn = nullptr;
        }
    }
    return conn;
}

void ConnectionPool::release(MYSQL* conn) {
    if(!conn) return;
    std::lock_guard<std::mutex> lock(mtx);
    pool.push(conn);
    cv.notify_one();
}

void ConnectionPool::destroy() {
    std::lock_guard<std::mutex> lock(mtx);
    while(!pool.empty()) {
        MYSQL* conn = pool.front();
        pool.pop();
        mysql_close(conn);
    }
    initialized = false;
}

ConnectionPool::~ConnectionPool() {
    destroy();
}
