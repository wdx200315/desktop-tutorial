#include "DatabaseManager.h"
#include "ConnectionPool.h"
#include "Logger.h"
#include "ConfigManager.h"
#include <regex>

DatabaseManager& DatabaseManager::instance() {
    static DatabaseManager inst;
    return inst;
}

ResultSet DatabaseManager::query(const std::string& sql) {
    DBConnection db;
    MYSQL* conn = db.get();
    if(!conn) {
        Logger::instance().log(ERROR, "No available DB connection");
        return {};
    }
    if(mysql_query(conn, sql.c_str()) != 0) {
        Logger::instance().log(ERROR, "Query error: " + std::string(mysql_error(conn)) + " SQL: " + sql);
        return {};
    }
    MYSQL_RES* res = mysql_store_result(conn);
    if(!res) return {};
    
    ResultSet result;
    MYSQL_ROW row;
    unsigned int num_fields = mysql_num_fields(res);
    MYSQL_FIELD* fields = mysql_fetch_fields(res);
    
    while((row = mysql_fetch_row(res))) {
        Row r;
        for(unsigned int i=0;i<num_fields;++i) {
            r[fields[i].name] = row[i] ? row[i] : "";
        }
        result.push_back(r);
    }
    mysql_free_result(res);
    return result;
}

int DatabaseManager::execute(const std::string& sql) {
    DBConnection db;
    MYSQL* conn = db.get();
    if(!conn) return -1;
    if(mysql_query(conn, sql.c_str()) != 0) {
        Logger::instance().log(ERROR, "Execute error: " + std::string(mysql_error(conn)) + " SQL: " + sql);
        return -1;
    }
    return mysql_affected_rows(conn);
}

// ========== SQL安全方法 ==========

std::string DatabaseManager::escape(const std::string& input) {
    DBConnection db;
    MYSQL* conn = db.get();
    if(!conn) return input;
    
    std::vector<char> buffer(input.length() * 2 + 1);
    mysql_real_escape_string(conn, buffer.data(), input.c_str(), input.length());
    return std::string(buffer.data());
}

bool DatabaseManager::isSafeInput(const std::string& input) {
    // 检查是否只包含字母数字和下划线
    static std::regex safePattern("^[a-zA-Z0-9_]+$");
    if (!std::regex_match(input, safePattern)) {
        // 检查是否有SQL注入风险
        static const char* sqlKeywords[] = {
            "SELECT", "INSERT", "UPDATE", "DELETE", "DROP", "CREATE",
            "ALTER", "TRUNCATE", "EXEC", "EXECUTE", "UNION", "--",
            "/*", "*/", ";", "'", "\"", " OR ", " AND ", "1=1", "1=2"
        };
        std::string lowerInput = input;
        std::transform(lowerInput.begin(), lowerInput.end(), lowerInput.begin(), ::tolower);
        
        for (const char* keyword : sqlKeywords) {
            std::string lowerKeyword = keyword;
            std::transform(lowerKeyword.begin(), lowerKeyword.end(), lowerKeyword.begin(), ::tolower);
            if (lowerInput.find(lowerKeyword) != std::string::npos) {
                Logger::instance().log(WARN, "Potential SQL injection detected: " + input);
                return false;
            }
        }
    }
    return true;
}

ResultSet DatabaseManager::queryWithParams(const std::string& sql, const std::vector<SQLBinding>& params) {
    std::string escapedSql = sql;
    
    // 对每个参数进行转义并替换占位符
    for (const auto& param : params) {
        std::string safeValue = escape(param.value);
        std::string placeholder = ":" + param.column;
        
        // 替换占位符
        size_t pos;
        while ((pos = escapedSql.find(placeholder)) != std::string::npos) {
            if (param.isString) {
                escapedSql.replace(pos, placeholder.length(), "'" + safeValue + "'");
            } else {
                escapedSql.replace(pos, placeholder.length(), safeValue);
            }
        }
    }
    
    return query(escapedSql);
}

int DatabaseManager::executeWithParams(const std::string& sql, const std::vector<SQLBinding>& params) {
    std::string escapedSql = sql;
    
    // 对每个参数进行转义并替换占位符
    for (const auto& param : params) {
        std::string safeValue = escape(param.value);
        std::string placeholder = ":" + param.column;
        
        // 替换占位符
        size_t pos;
        while ((pos = escapedSql.find(placeholder)) != std::string::npos) {
            if (param.isString) {
                escapedSql.replace(pos, placeholder.length(), "'" + safeValue + "'");
            } else {
                escapedSql.replace(pos, placeholder.length(), safeValue);
            }
        }
    }
    
    return execute(escapedSql);
}

uint64_t DatabaseManager::lastInsertId() {
    DBConnection db;
    return mysql_insert_id(db.get());
}

void DatabaseManager::begin() {
    DBConnection db;
    mysql_query(db.get(), "START TRANSACTION");
}

void DatabaseManager::commit() {
    DBConnection db;
    mysql_query(db.get(), "COMMIT");
}

void DatabaseManager::rollback() {
    DBConnection db;
    mysql_query(db.get(), "ROLLBACK");
}
