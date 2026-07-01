#pragma once
#include "ConnectionPool.h"
#include "common.h"
#include <mysql/mysql.h>
#include <vector>
#include <map>
#include <functional>
#include <string>
#include <memory>

using Row = std::map<std::string, std::string>;
using ResultSet = std::vector<Row>;

// SQL参数绑定结构
struct SQLBinding {
    std::string column;
    std::string value;
    bool isString;
};

class DatabaseManager {
public:
    static DatabaseManager& instance();
    
    // 执行查询，返回结果集
    ResultSet query(const std::string& sql);
    
    // 执行更新，返回影响行数
    int execute(const std::string& sql);
    
    // 执行带参数的查询（防SQL注入）
    ResultSet queryWithParams(const std::string& sql, const std::vector<SQLBinding>& params);
    
    // 执行带参数的更新（防SQL注入）
    int executeWithParams(const std::string& sql, const std::vector<SQLBinding>& params);
    
    // SQL转义
    std::string escape(const std::string& input);
    
    // 获取最后插入ID
    uint64_t lastInsertId();
    
    // 事务
    void begin();
    void commit();
    void rollback();
    
    // 验证字符串是否安全（不含SQL关键字注入）
    bool isSafeInput(const std::string& input);
};

// RAII 连接自动释放
class DBConnection {
public:
    DBConnection() : conn(ConnectionPool::instance().acquire()) {}
    ~DBConnection() { if(conn) ConnectionPool::instance().release(conn); }
    MYSQL* get() { return conn; }
private:
    MYSQL* conn;
};
