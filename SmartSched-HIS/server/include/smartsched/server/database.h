/**
 * @file database.h
 * @brief MySQL数据库连接池
 * 
 * 数据视角: 医疗数据持久化
 * 性能视角: 连接池复用提升性能
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <queue>
#include <atomic>
#include <functional>
#include <mysql/mysql.h>

namespace smartsched {
namespace server {

// =============================================================================
// 数据库配置
// =============================================================================
struct DatabaseConfig {
    std::string host = "localhost";
    int port = 3306;
    std::string database = "smartsched";
    std::string username = "root";
    std::string password;
    
    // 连接池配置
    int min_connections = 2;
    int max_connections = 10;
    int connection_timeout = 30;  // seconds
    
    // 连接池健康检测
    bool enable_health_check = true;
    int health_check_interval = 60;  // seconds
    int max_idle_time = 300;  // seconds
    
    // 自动重连
    bool auto_reconnect = true;
    int max_reconnect_attempts = 3;
};

// =============================================================================
// 数据库连接
// =============================================================================
class DbConnection {
public:
    DbConnection();
    ~DbConnection();
    
    // 连接管理
    bool connect(const DatabaseConfig& config);
    void disconnect();
    bool isConnected() const;
    
    // 查询
    bool execute(const std::string& sql);
    bool query(const std::string& sql);
    
    // 结果集
    int getRowCount() const;
    int getColumnCount() const;
    bool nextRow();
    
    // 获取字段值
    std::string getString(int column);
    std::string getString(const std::string& column_name);
    int getInt(int column);
    int getInt(const std::string& column_name);
    long long getLong(int column);
    long long getLong(const std::string& column_name);
    double getDouble(int column);
    double getDouble(const std::string& column_name);
    
    // 预处理语句
    bool prepare(const std::string& sql);
    void bindString(int param, const std::string& value);
    void bindInt(int param, int value);
    void bindLong(int param, long long value);
    bool executePrepared();
    bool queryPrepared();
    
    // 事务
    bool beginTransaction();
    bool commit();
    bool rollback();
    
    // 连接信息
    MYSQL* getHandle() { return mysql_; }
    int getConnectionId() const { return connection_id_; }
    qint64 getLastActiveTime() const { return last_active_time_; }
    void updateLastActiveTime() { last_active_time_ = QDateTime::currentMSecsSinceEpoch(); }
    
private:
    MYSQL* mysql_;
    MYSQL_STMT* stmt_;
    bool is_prepared_;
    int connection_id_;
    qint64 last_active_time_;
    
    static std::atomic<int> connection_counter_;
};

// =============================================================================
// 连接池
// =============================================================================
class ConnectionPool {
public:
    explicit ConnectionPool(const DatabaseConfig& config);
    ~ConnectionPool();
    
    // 初始化
    bool initialize();
    void shutdown();
    
    // 获取连接
    std::shared_ptr<DbConnection> getConnection();
    void releaseConnection(std::shared_ptr<DbConnection> conn);
    
    // 配置
    void setConfig(const DatabaseConfig& config);
    DatabaseConfig config() const { return config_; }
    
    // 统计
    int activeConnections() const;
    int idleConnections() const;
    int totalConnections() const;
    
    // 健康检查
    void healthCheck();
    void reconnectAll();
    
signals:
    void connectionCreated(int pool_size);
    void connectionClosed();
    void healthCheckCompleted(int healthy, int unhealthy);
    void errorOccurred(const QString& error);

private:
    DbConnection* createConnection();
    bool validateConnection(DbConnection* conn);
    void closeConnection(DbConnection* conn);
    void cleanupIdleConnections();
    
    DatabaseConfig config_;
    std::queue<DbConnection*> idle_connections_;
    std::vector<DbConnection*> all_connections_;
    std::mutex mutex_;
    std::atomic<bool> running_;
    
    // 统计
    std::atomic<int> active_count_;
    std::atomic<int> total_acquired_;
    std::atomic<int> total_released_;
};

// =============================================================================
// 数据访问对象基类
// =============================================================================
class DataAccessObject {
public:
    explicit DataAccessObject(std::shared_ptr<ConnectionPool> pool);
    virtual ~DataAccessObject() = default;
    
protected:
    std::shared_ptr<DbConnection> getConnection();
    void releaseConnection(std::shared_ptr<DbConnection> conn);
    
    std::shared_ptr<ConnectionPool> pool_;
};

// =============================================================================
// SQL构建器
// =============================================================================
class SqlBuilder {
public:
    SqlBuilder& select(const std::string& columns = "*");
    SqlBuilder& from(const std::string& table);
    SqlBuilder& where(const std::string& condition);
    SqlBuilder& andWhere(const std::string& condition);
    SqlBuilder& orWhere(const std::string& condition);
    SqlBuilder& orderBy(const std::string& column, bool ascending = true);
    SqlBuilder& limit(int count);
    SqlBuilder& offset(int start);
    SqlBuilder& insert(const std::string& table, const std::map<std::string, std::string>& values);
    SqlBuilder& update(const std::string& table, const std::map<std::string, std::string>& values);
    SqlBuilder& remove(const std::string& table);
    
    std::string build();
    
private:
    std::ostringstream sql_;
    std::string where_clause_;
    std::string order_clause_;
    std::string limit_clause_;
};

// =============================================================================
// 内联实现
// =============================================================================

inline DbConnection::DbConnection() 
    : mysql_(nullptr)
    , stmt_(nullptr)
    , is_prepared_(false)
    , connection_id_(++connection_counter_)
    , last_active_time_(QDateTime::currentMSecsSinceEpoch())
{
    mysql_ = mysql_init(nullptr);
    if (mysql_) {
        // 设置连接选项
        mysql_options(mysql_, MYSQL_SET_CHARSET_NAME, "utf8mb4");
    }
}

inline DbConnection::~DbConnection() {
    if (stmt_) {
        mysql_stmt_close(stmt_);
    }
    if (mysql_) {
        mysql_close(mysql_);
    }
}

inline bool DbConnection::isConnected() const {
    return mysql_ && mysql_ping(mysql_) == 0;
}

inline int DbConnection::getColumnCount() const {
    return mysql_ ? mysql_num_fields(mysql_store_result(mysql_)) : 0;
}

inline std::string DbConnection::getString(int column) {
    if (MYSQL_ROW row = mysql_fetch_row(mysql_)) {
        return row[column] ? row[column] : "";
    }
    return "";
}

inline std::string DbConnection::getString(const std::string& column_name) {
    // 需要先获取列索引
    MYSQL_RES* result = mysql_store_result(mysql_);
    if (!result) return "";
    
    unsigned int num_fields = mysql_num_fields(result);
    MYSQL_FIELD* fields = mysql_fetch_fields(result);
    
    for (unsigned int i = 0; i < num_fields; ++i) {
        if (column_name == fields[i].name) {
            std::string value = getString(i);
            mysql_free_result(result);
            return value;
        }
    }
    
    mysql_free_result(result);
    return "";
}

inline int DbConnection::getInt(int column) {
    std::string val = getString(column);
    return val.empty() ? 0 : std::atoi(val.c_str());
}

inline int DbConnection::getInt(const std::string& column_name) {
    std::string val = getString(column_name);
    return val.empty() ? 0 : std::atoi(val.c_str());
}

inline long long DbConnection::getLong(int column) {
    std::string val = getString(column);
    return val.empty() ? 0 : std::atoll(val.c_str());
}

inline long long DbConnection::getLong(const std::string& column_name) {
    std::string val = getString(column_name);
    return val.empty() ? 0 : std::atoll(val.c_str());
}

inline double DbConnection::getDouble(int column) {
    std::string val = getString(column);
    return val.empty() ? 0.0 : std::atof(val.c_str());
}

inline double DbConnection::getDouble(const std::string& column_name) {
    std::string val = getString(column_name);
    return val.empty() ? 0.0 : std::atof(val.c_str());
}

inline bool DbConnection::beginTransaction() {
    return execute("START TRANSACTION");
}

inline bool DbConnection::commit() {
    return execute("COMMIT");
}

inline bool DbConnection::rollback() {
    return execute("ROLLBACK");
}

} // namespace server
} // namespace smartsched
