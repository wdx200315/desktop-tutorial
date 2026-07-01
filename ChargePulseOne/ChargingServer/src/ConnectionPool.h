#pragma once
#include <mysql/mysql.h>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <string>

class ConnectionPool {
public:
    static ConnectionPool& instance();
    void init(const std::string& host, const std::string& user, const std::string& pass, 
              const std::string& db, int port, int poolSize = 10);
    MYSQL* acquire();
    void release(MYSQL* conn);
    void destroy();

private:
    ConnectionPool() = default;
    ~ConnectionPool();
    std::queue<MYSQL*> pool;
    std::mutex mtx;
    std::condition_variable cv;
    int size = 0;
    bool initialized = false;
};
