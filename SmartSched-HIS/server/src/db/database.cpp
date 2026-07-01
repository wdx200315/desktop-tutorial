/**
 * @file database.cpp
 * @brief MySQL数据库实现
 */

#include "smartsched/server/database.h"
#include "../../common/include/smartsched/utils/logger.h"
#include <QDateTime>
#include <QTimer>

namespace smartsched {
namespace server {

// =============================================================================
// 静态成员
// =============================================================================
std::atomic<int> DbConnection::connection_counter_(0);

// =============================================================================
// DbConnection实现
// =============================================================================

bool DbConnection::connect(const DatabaseConfig& config) {
    if (!mysql_) {
        LOG_ERROR("MySQL connection not initialized");
        return false;
    }
    
    // 尝试连接
    if (!mysql_real_connect(
            mysql_,
            config.host.c_str(),
            config.username.c_str(),
            config.password.c_str(),
            config.database.c_str(),
            config.port,
            nullptr,
            0
    )) {
        LOG_ERROR("MySQL connection failed: " + std::string(mysql_error(mysql_)));
        return false;
    }
    
    LOG_INFO("MySQL connection established: id=" + std::to_string(connection_id_));
    return true;
}

void DbConnection::disconnect() {
    if (mysql_) {
        mysql_close(mysql_);
        mysql_ = mysql_init(nullptr);
    }
}

bool DbConnection::execute(const std::string& sql) {
    if (!mysql_) return false;
    
    updateLastActiveTime();
    
    if (mysql_query(mysql_, sql.c_str())) {
        LOG_ERROR("MySQL execute failed: " + std::string(mysql_error(mysql_)));
        LOG_DEBUG("SQL: " + sql);
        return false;
    }
    
    return true;
}

bool DbConnection::query(const std::string& sql) {
    if (!mysql_) return false;
    
    updateLastActiveTime();
    
    // 先清除之前的结果
    mysql_free_result(mysql_store_result(mysql_));
    
    if (mysql_query(mysql_, sql.c_str())) {
        LOG_ERROR("MySQL query failed: " + std::string(mysql_error(mysql_)));
        LOG_DEBUG("SQL: " + sql);
        return false;
    }
    
    MYSQL_RES* result = mysql_store_result(mysql_);
    if (!result) {
        // 不是SELECT，可能是INSERT/UPDATE/DELETE
        return true;
    }
    
    mysql_free_result(result);
    return true;
}

int DbConnection::getRowCount() const {
    if (!mysql_) return 0;
    MYSQL_RES* result = mysql_store_result(mysql_);
    if (!result) return 0;
    
    int count = mysql_num_rows(result);
    mysql_free_result(result);
    return count;
}

bool DbConnection::nextRow() {
    if (!mysql_) return false;
    MYSQL_ROW row = mysql_fetch_row(mysql_store_result(mysql_));
    return row != nullptr;
}

bool DbConnection::prepare(const std::string& sql) {
    if (!mysql_) return false;
    
    if (stmt_) {
        mysql_stmt_close(stmt_);
    }
    
    stmt_ = mysql_stmt_init(mysql_);
    if (!stmt_) {
        LOG_ERROR("Failed to initialize statement");
        return false;
    }
    
    if (mysql_stmt_prepare(stmt_, sql.c_str(), sql.length())) {
        LOG_ERROR("Failed to prepare statement: " + std::string(mysql_stmt_error(stmt_)));
        mysql_stmt_close(stmt_);
        stmt_ = nullptr;
        return false;
    }
    
    is_prepared_ = true;
    return true;
}

void DbConnection::bindString(int param, const std::string& value) {
    if (!stmt_ || !is_prepared_) return;
    
    MYSQL_BIND bind;
    std::memset(&bind, 0, sizeof(bind));
    bind.buffer_type = MYSQL_TYPE_STRING;
    bind.buffer = const_cast<char*>(value.data());
    bind.buffer_length = value.length();
    bind.is_null = 0;
    
    mysql_stmt_bind_param(stmt_, &bind);
}

void DbConnection::bindInt(int param, int value) {
    if (!stmt_ || !is_prepared_) return;
    
    MYSQL_BIND bind;
    std::memset(&bind, 0, sizeof(bind));
    bind.buffer_type = MYSQL_TYPE_LONG;
    bind.buffer = &value;
    bind.is_null = 0;
    
    mysql_stmt_bind_param(stmt_, &bind);
}

void DbConnection::bindLong(int param, long long value) {
    if (!stmt_ || !is_prepared_) return;
    
    MYSQL_BIND bind;
    std::memset(&bind, 0, sizeof(bind));
    bind.buffer_type = MYSQL_TYPE_LONGLONG;
    bind.buffer = &value;
    bind.is_null = 0;
    
    mysql_stmt_bind_param(stmt_, &bind);
}

bool DbConnection::executePrepared() {
    if (!stmt_ || !is_prepared_) return false;
    
    updateLastActiveTime();
    
    if (mysql_stmt_execute(stmt_)) {
        LOG_ERROR("Failed to execute prepared statement: " + std::string(mysql_stmt_error(stmt_)));
        return false;
    }
    
    return true;
}

bool DbConnection::queryPrepared() {
    if (!stmt_ || !is_prepared_) return false;
    
    updateLastActiveTime();
    
    if (mysql_stmt_execute(stmt_)) {
        LOG_ERROR("Failed to execute prepared query: " + std::string(mysql_stmt_error(stmt_)));
        return false;
    }
    
    return true;
}

// =============================================================================
// ConnectionPool实现
// =============================================================================

ConnectionPool::ConnectionPool(const DatabaseConfig& config)
    : config_(config)
    , running_(true)
    , active_count_(0)
    , total_acquired_(0)
    , total_released_(0)
{
}

ConnectionPool::~ConnectionPool() {
    shutdown();
}

bool ConnectionPool::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!running_) {
        LOG_ERROR("Connection pool is not running");
        return false;
    }
    
    // 创建最小连接数
    for (int i = 0; i < config_.min_connections; ++i) {
        DbConnection* conn = createConnection();
        if (conn) {
            idle_connections_.push(conn);
            all_connections_.push_back(conn);
        } else {
            LOG_ERROR("Failed to create initial connection " + std::to_string(i));
            return false;
        }
    }
    
    LOG_INFO("Connection pool initialized: min=" + std::to_string(config_.min_connections) +
            ", max=" + std::to_string(config_.max_connections));
    
    emit connectionCreated(idle_connections_.size());
    return true;
}

void ConnectionPool::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    running_ = false;
    
    // 关闭所有连接
    while (!idle_connections_.empty()) {
        DbConnection* conn = idle_connections_.front();
        idle_connections_.pop();
        closeConnection(conn);
    }
    
    for (DbConnection* conn : all_connections_) {
        closeConnection(conn);
    }
    
    all_connections_.clear();
    
    LOG_INFO("Connection pool shutdown");
}

DbConnection* ConnectionPool::createConnection() {
    DbConnection* conn = new DbConnection();
    
    if (!conn->connect(config_)) {
        delete conn;
        return nullptr;
    }
    
    return conn;
}

bool ConnectionPool::validateConnection(DbConnection* conn) {
    if (!conn || !conn->isConnected()) {
        return false;
    }
    
    // 简单的ping测试
    return mysql_ping(conn->getHandle()) == 0;
}

void ConnectionPool::closeConnection(DbConnection* conn) {
    if (conn) {
        conn->disconnect();
        delete conn;
        emit connectionClosed();
    }
}

std::shared_ptr<DbConnection> ConnectionPool::getConnection() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 尝试从空闲队列获取
    while (!idle_connections_.empty()) {
        DbConnection* conn = idle_connections_.front();
        idle_connections_.pop();
        
        // 验证连接
        if (validateConnection(conn)) {
            active_count_++;
            total_acquired_++;
            
            std::shared_ptr<DbConnection> shPtr(conn, [this](DbConnection* c) {
                releaseConnection(c);
            });
            
            return shPtr;
        } else {
            // 连接无效，关闭并重新创建
            LOG_WARN("Removing invalid connection from pool");
            closeConnection(conn);
        }
    }
    
    // 如果可以创建新连接
    if (static_cast<int>(all_connections_.size()) < config_.max_connections) {
        DbConnection* conn = createConnection();
        if (conn) {
            all_connections_.push_back(conn);
            active_count_++;
            total_acquired_++;
            
            std::shared_ptr<DbConnection> shPtr(conn, [this](DbConnection* c) {
                releaseConnection(c);
            });
            
            return shPtr;
        }
    }
    
    // 等待（简化处理，实际应该使用条件变量）
    LOG_WARN("Connection pool exhausted, waiting...");
    return nullptr;
}

void ConnectionPool::releaseConnection(std::shared_ptr<DbConnection> conn) {
    if (!conn) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    active_count_--;
    total_released_++;
    
    // 检查连接是否仍然有效
    if (conn->isConnected()) {
        // 如果空闲连接过多，可以关闭
        if (static_cast<int>(idle_connections_.size()) >= config_.max_connections) {
            closeConnection(conn.get());
        } else {
            idle_connections_.push(conn.get());
        }
    } else {
        // 连接已断开
        LOG_WARN("Released connection is disconnected, closing");
        closeConnection(conn.get());
    }
}

int ConnectionPool::activeConnections() const {
    return active_count_;
}

int ConnectionPool::idleConnections() const {
    return idle_connections_.size();
}

int ConnectionPool::totalConnections() const {
    return all_connections_.size();
}

void ConnectionPool::healthCheck() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    int healthy = 0;
    int unhealthy = 0;
    
    std::vector<DbConnection*> to_remove;
    
    for (DbConnection* conn : all_connections_) {
        if (validateConnection(conn)) {
            healthy++;
        } else {
            unhealthy++;
            to_remove.push_back(conn);
        }
    }
    
    // 移除不健康的连接
    for (DbConnection* conn : to_remove) {
        auto it = std::find(all_connections_.begin(), all_connections_.end(), conn);
        if (it != all_connections_.end()) {
            all_connections_.erase(it);
        }
        
        // 从idle队列中移除
        std::queue<DbConnection*> new_idle;
        while (!idle_connections_.empty()) {
            if (idle_connections_.front() != conn) {
                new_idle.push(idle_connections_.front());
            }
            idle_connections_.pop();
        }
        idle_connections_ = std::move(new_idle);
        
        closeConnection(conn);
        
        // 如果需要，可以尝试重新创建
        if (config_.auto_reconnect && 
            static_cast<int>(all_connections_.size()) < config_.min_connections) {
            DbConnection* new_conn = createConnection();
            if (new_conn) {
                all_connections_.push_back(new_conn);
                idle_connections_.push(new_conn);
            }
        }
    }
    
    LOG_DEBUG("Health check: healthy=" + std::to_string(healthy) + 
             ", unhealthy=" + std::to_string(unhealthy));
    
    emit healthCheckCompleted(healthy, unhealthy);
}

void ConnectionPool::reconnectAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    LOG_INFO("Reconnecting all connections...");
    
    // 关闭所有现有连接
    while (!idle_connections_.empty()) {
        DbConnection* conn = idle_connections_.front();
        idle_connections_.pop();
        closeConnection(conn);
    }
    
    for (DbConnection* conn : all_connections_) {
        closeConnection(conn);
    }
    
    all_connections_.clear();
    
    // 重新创建
    for (int i = 0; i < config_.min_connections; ++i) {
        DbConnection* conn = createConnection();
        if (conn) {
            idle_connections_.push(conn);
            all_connections_.push_back(conn);
        }
    }
    
    LOG_INFO("Reconnected: " + std::to_string(all_connections_.size()) + " connections");
}

// =============================================================================
// DataAccessObject实现
// =============================================================================

DataAccessObject::DataAccessObject(std::shared_ptr<ConnectionPool> pool)
    : pool_(pool)
{
}

std::shared_ptr<DbConnection> DataAccessObject::getConnection() {
    return pool_->getConnection();
}

void DataAccessObject::releaseConnection(std::shared_ptr<DbConnection> conn) {
    pool_->releaseConnection(conn);
}

// =============================================================================
// SqlBuilder实现
// =============================================================================

SqlBuilder& SqlBuilder::select(const std::string& columns) {
    sql_ << "SELECT " << columns;
    return *this;
}

SqlBuilder& SqlBuilder::from(const std::string& table) {
    sql_ << " FROM " << table;
    return *this;
}

SqlBuilder& SqlBuilder::where(const std::string& condition) {
    where_clause_ = " WHERE " + condition;
    return *this;
}

SqlBuilder& SqlBuilder::andWhere(const std::string& condition) {
    if (where_clause_.empty()) {
        return where(condition);
    }
    where_clause_ += " AND " + condition;
    return *this;
}

SqlBuilder& SqlBuilder::orWhere(const std::string& condition) {
    if (where_clause_.empty()) {
        return where(condition);
    }
    where_clause_ += " OR " + condition;
    return *this;
}

SqlBuilder& SqlBuilder::orderBy(const std::string& column, bool ascending) {
    order_clause_ = " ORDER BY " + column + (ascending ? " ASC" : " DESC");
    return *this;
}

SqlBuilder& SqlBuilder::limit(int count) {
    limit_clause_ = " LIMIT " + std::to_string(count);
    return *this;
}

SqlBuilder& SqlBuilder::offset(int start) {
    limit_clause_ += " OFFSET " + std::to_string(start);
    return *this;
}

SqlBuilder& SqlBuilder::insert(const std::string& table, const std::map<std::string, std::string>& values) {
    sql_ << "INSERT INTO " << table << " (";
    
    bool first = true;
    for (const auto& [key, value] : values) {
        if (!first) sql_ << ", ";
        sql_ << key;
        first = false;
    }
    
    sql_ << ") VALUES (";
    
    first = true;
    for (const auto& [key, value] : values) {
        if (!first) sql_ << ", ";
        sql_ << "'" << value << "'";
        first = false;
    }
    
    sql_ << ")";
    return *this;
}

SqlBuilder& SqlBuilder::update(const std::string& table, const std::map<std::string, std::string>& values) {
    sql_ << "UPDATE " << table << " SET ";
    
    bool first = true;
    for (const auto& [key, value] : values) {
        if (!first) sql_ << ", ";
        sql_ << key << " = '" << value << "'";
        first = false;
    }
    
    return *this;
}

SqlBuilder& SqlBuilder::remove(const std::string& table) {
    sql_ << "DELETE FROM " << table;
    return *this;
}

std::string SqlBuilder::build() {
    std::string result = sql_.str();
    result += where_clause_;
    result += order_clause_;
    result += limit_clause_;
    return result;
}

} // namespace server
} // namespace smartsched
