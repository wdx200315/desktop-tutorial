/**
 * @file security.h
 * @brief 安全防护模块 - SQL注入检测、XSS防护、请求限流
 */
#ifndef SMARTSCHED_SECURITY_H
#define SMARTSCHED_SECURITY_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include <regex>
#include <chrono>
#include <memory>
#include <functional>

namespace smartsched {

// ==================== 安全事件 ====================

/**
 * @brief 安全事件类型
 */
enum class SecurityEvent {
    SQLInjection,       // SQL注入
    XSSAttack,          // 跨站脚本攻击
    CSRFAttack,         // 跨站请求伪造
    BruteForce,         // 暴力破解
    RateLimit,          // 请求频率超限
    InvalidToken,       // 无效令牌
    PrivilegeEscalation,// 权限提升
    DataLeak,           // 数据泄露
    SuspiciousIP,       // 可疑IP
    SessionHijack       // 会话劫持
};

/**
 * @brief 安全事件记录
 */
struct SecurityEventRecord {
    int64_t timestamp;
    SecurityEvent event;
    std::string ip;
    std::string user_id;
    std::string details;
    std::string raw_data;
    bool blocked;
};

// ==================== SQL注入检测 ====================

/**
 * @brief SQL注入检测器
 */
class SQLInjectionDetector {
public:
    static SQLInjectionDetector& instance();
    
    // 检测
    bool isDangerous(const std::string& input) const;
    std::vector<std::string> findDangerousPatterns(const std::string& input) const;
    std::string sanitize(const std::string& input) const;
    
    // 管理规则
    void addPattern(const std::string& pattern);
    void removePattern(const std::string& pattern);
    void loadDefaultPatterns();
    
    // 白名单
    void addWhitelist(const std::string& value);
    bool isWhitelisted(const std::string& value) const;
    void clearWhitelist();

private:
    SQLInjectionDetector();
    
    std::vector<std::regex> m_patterns;
    std::set<std::string> m_whitelist;
    mutable std::mutex m_mutex;
};

// ==================== XSS防护 ====================

/**
 * @brief XSS防护器
 */
class XSSProtector {
public:
    static XSSProtector& instance();
    
    // 编码
    std::string encodeHTML(const std::string& input) const;
    std::string encodeJavaScript(const std::string& input) const;
    std::string encodeURL(const std::string& input) const;
    std::string encodeCSS(const std::string& input) const;
    
    // 检测
    bool containsXSS(const std::string& input) const;
    std::string sanitize(const std::string& input) const;
    
    // 配置
    void setAllowScripts(bool allow) { m_allowScripts = allow; }
    void setAllowStyles(bool allow) { m_allowStyles = allow; }

private:
    XSSProtector();
    
    bool m_allowScripts = false;
    bool m_allowStyles = false;
};

// ==================== 请求限流 ====================

/**
 * @brief 限流策略
 */
enum class RateLimitPolicy {
    TokenBucket,    // 令牌桶
    LeakyBucket,    // 漏桶
    FixedWindow,    // 固定窗口
    SlidingWindow   // 滑动窗口
};

/**
 * @brief 限流结果
 */
struct RateLimitResult {
    bool allowed;
    int64_t remaining;
    int64_t resetTime;
    int64_t retryAfter;  // 秒
    
    RateLimitResult() : allowed(true), remaining(0), resetTime(0), retryAfter(0) {}
};

/**
 * @brief 请求限流器
 */
class RateLimiter {
public:
    RateLimiter();
    
    // 配置
    void setPolicy(RateLimitPolicy policy);
    void setLimit(int maxRequests, int windowSeconds);
    void addExemption(const std::string& ip);
    void removeExemption(const std::string& ip);
    
    // 检查
    RateLimitResult check(const std::string& key);
    RateLimitResult check(const std::string& key, const std::string& ip);
    
    // 统计
    int64_t getRequestCount(const std::string& key) const;
    void reset(const std::string& key);
    void resetAll();
    
    // 回调
    using OnLimitExceeded = std::function<void(const std::string&, const std::string&, int)>;
    void setOnLimitExceeded(OnLimitExceeded callback);

private:
    RateLimitPolicy m_policy;
    int m_maxRequests;
    int m_windowSeconds;
    std::set<std::string> m_exemptions;
    
    struct Bucket {
        int64_t tokens;
        int64_t lastUpdate;
    };
    
    std::map<std::string, Bucket> m_buckets;
    std::map<std::string, std::pair<int64_t, int>> m_counters;  // key -> (window_start, count)
    std::mutex m_mutex;
    OnLimitExceeded m_callback;
};

// ==================== IP黑名单 ====================

/**
 * @brief IP黑名单管理器
 */
class IPBlacklist {
public:
    static IPBlacklist& instance();
    
    // 管理
    void block(const std::string& ip, const std::string& reason = "", int64_t duration = -1);
    void unblock(const std::string& ip);
    bool isBlocked(const std::string& ip) const;
    
    // 批量操作
    void blockRange(const std::string& cidr, const std::string& reason = "");
    void unblockRange(const std::string& cidr);
    
    // 持久化
    bool loadFromFile(const std::string& filepath);
    bool saveToFile(const std::string& filepath) const;
    
    // 统计
    size_t size() const;
    std::vector<std::string> getBlockedIPs() const;

private:
    IPBlacklist();
    
    struct BlockedIP {
        std::string ip;
        std::string reason;
        int64_t blockedAt;
        int64_t expiresAt;  // -1表示永久
    };
    
    std::map<std::string, BlockedIP> m_blocked;
    std::mutex m_mutex;
};

// ==================== CSRF防护 ====================

/**
 * @brief CSRF令牌管理器
 */
class CSRFProtector {
public:
    static CSRFProtector& instance();
    
    // 生成
    std::string generateToken(const std::string& sessionId);
    std::string generateTokenForUser(int userId);
    
    // 验证
    bool validateToken(const std::string& sessionId, const std::string& token);
    bool validateTokenForUser(int userId, const std::string& token);
    
    // 清除
    void invalidateToken(const std::string& sessionId);
    void invalidateUserTokens(int userId);
    
    // 配置
    void setTokenExpiry(int64_t seconds);

private:
    CSRFProtector();
    
    std::string generateRandomToken();
    int64_t hashToken(const std::string& token) const;
    
    struct TokenInfo {
        int64_t hash;
        int64_t expiresAt;
        int64_t createdAt;
    };
    
    std::map<std::string, std::vector<TokenInfo>> m_sessionTokens;
    std::map<int, std::vector<TokenInfo>> m_userTokens;
    std::mutex m_mutex;
    int64_t m_tokenExpiry;
};

// ==================== 安全审计日志 ====================

/**
 * @brief 安全审计日志
 */
class SecurityAudit {
public:
    static SecurityAudit& instance();
    
    // 记录
    void log(SecurityEvent event, const std::string& ip, 
             const std::string& userId = "", const std::string& details = "",
             bool blocked = false);
    void logSQLInjection(const std::string& ip, const std::string& sql);
    void logXSS(const std::string& ip, const std::string& content);
    void logBruteForce(const std::string& ip, const std::string& username);
    void logRateLimit(const std::string& ip, int count);
    
    // 查询
    std::vector<SecurityEventRecord> query(
        SecurityEvent event = SecurityEvent::SQLInjection,
        const std::string& ip = "",
        int64_t since = 0,
        int64_t until = 0,
        int limit = 100);
    
    // 统计
    struct EventStats {
        int64_t totalEvents;
        int64_t blockedEvents;
        std::map<std::string, int64_t> byIP;
        std::map<std::string, int64_t> byUser;
    };
    
    EventStats getStats(int64_t since = 0) const;
    
    // 配置
    void setMaxEventsPerMinute(int max);
    void setRetentionDays(int days);

private:
    SecurityAudit();
    
    std::string getEventName(SecurityEvent event) const;
    std::string sanitizeForLog(const std::string& input) const;
    
    std::vector<SecurityEventRecord> m_events;
    size_t m_maxEvents;
    int m_retentionDays;
    mutable std::mutex m_mutex;
};

// ==================== 综合安全服务 ====================

/**
 * @brief 安全服务 - 整合所有安全功能
 */
class SecurityService {
public:
    static SecurityService& instance();
    
    // 初始化
    void init();
    
    // 请求验证
    struct ValidationResult {
        bool valid;
        std::string error;
        bool shouldBlock;
        SecurityEvent event;
    };
    
    ValidationResult validateRequest(
        const std::string& ip,
        const std::string& userId,
        const std::map<std::string, std::string>& params,
        const std::string& csrfToken = "",
        const std::string& sessionId = "");
    
    // SQL参数验证
    ValidationResult validateSQLParams(
        const std::map<std::string, std::string>& params);
    
    // 输出编码
    std::string encodeOutput(const std::string& input, const std::string& context = "html");
    
    // IP检查
    bool isIPAllowed(const std::string& ip);
    
    // 配置
    void enableSQLProtection(bool enable);
    void enableXSSProtection(bool enable);
    void enableCSRFProtection(bool enable);
    void enableRateLimiting(bool enable);
    void setRateLimit(int maxRequests, int windowSeconds);

private:
    SecurityService();
    
    bool m_sqlProtectionEnabled = true;
    bool m_xssProtectionEnabled = true;
    bool m_csrfProtectionEnabled = true;
    bool m_rateLimitingEnabled = true;
};

} // namespace smartsched

#endif // SMARTSCHED_SECURITY_H
