/**
 * @file security.cpp
 * @brief 安全防护模块实现
 */
#include "security.h"
#include "logger.h"
#include <algorithm>
#include <random>
#include <sstream>

namespace smartsched {

// ==================== SQLInjectionDetector 实现 ====================

SQLInjectionDetector& SQLInjectionDetector::instance()
{
    static SQLInjectionDetector instance;
    return instance;
}

SQLInjectionDetector::SQLInjectionDetector()
{
    loadDefaultPatterns();
}

void SQLInjectionDetector::loadDefaultPatterns()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // SQL关键字
    m_patterns.push_back(std::regex(R"(union\s+select)", std::regex::icase));
    m_patterns.push_back(std::regex(R"(union\s+all\s+select)", std::regex::icase));
    m_patterns.push_back(std::regex(R"(select\s+.*\s+from)", std::regex::icase));
    m_patterns.push_back(std::regex(R"(insert\s+into)", std::regex::icase));
    m_patterns.push_back(std::regex(R"(delete\s+from)", std::regex::icase));
    m_patterns.push_back(std::regex(R"(update\s+.*\s+set)", std::regex::icase));
    m_patterns.push_back(std::regex(R"(drop\s+table)", std::regex::icase));
    m_patterns.push_back(std::regex(R"(drop\s+database)", std::regex::icase));
    m_patterns.push_back(std::regex(R"(alter\s+table)", std::regex::icase));
    m_patterns.push_back(std::regex(R"(create\s+table)", std::regex::icase));
    m_patterns.push_back(std::regex(R"(exec\s*\()", std::regex::icase));
    m_patterns.push_back(std::regex(R"(execute\s*\()", std::regex::icase));
    m_patterns.push_back(std::regex(R"(xp_cmdshell)", std::regex::icase));
    m_patterns.push_back(std::regex(R"(sp_executesql)", std::regex::icase));
    
    // 注释和字符串终止
    m_patterns.push_back(std::regex(R"(--)"));
    m_patterns.push_back(std::regex(R"(#)"));
    m_patterns.push_back(std::regex(R"(/\*.*\*/)", std::regex::icase));
    m_patterns.push_back(std::regex(R"(;)", std::regex::icase));
    
    // 布尔注入
    m_patterns.push_back(std::regex(R"(\bor\b.*=.*)", std::regex::icase));
    m_patterns.push_back(std::regex(R"(\band\b.*=.*)", std::regex::icase));
    m_patterns.push_back(std::regex(R"('\s*or\s*')", std::regex::icase));
    m_patterns.push_back(std::regex(R"('\s*and\s*')", std::regex::icase));
    
    // 堆叠查询
    m_patterns.push_back(std::regex(R"(;\s*select)", std::regex::icase));
    m_patterns.push_back(std::regex(R"(;\s*insert)", std::regex::icase));
    m_patterns.push_back(std::regex(R"(;\s*delete)", std::regex::icase));
    m_patterns.push_back(std::regex(R"(;\s*update)", std::regex::icase));
    
    // 十六进制编码
    m_patterns.push_back(std::regex(R"(0x[0-9a-f]+)", std::regex::icase));
    
    // 常见攻击向量
    m_patterns.push_back(std::regex(R"(<script)", std::regex::icase));
    m_patterns.push_back(std::regex(R"(javascript:)", std::regex::icase));
    m_patterns.push_back(std::regex(R"(onerror\s*=)", std::regex::icase));
    m_patterns.push_back(std::regex(R"(onload\s*=)", std::regex::icase));
}

bool SQLInjectionDetector::isDangerous(const std::string& input) const
{
    if (input.empty()) return false;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // 白名单检查
    if (m_whitelist.count(input)) {
        return false;
    }
    
    // 模式匹配
    for (const auto& pattern : m_patterns) {
        if (std::regex_search(input, pattern)) {
            return true;
        }
    }
    
    return false;
}

std::vector<std::string> SQLInjectionDetector::findDangerousPatterns(
    const std::string& input) const
{
    std::vector<std::string> found;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    for (const auto& pattern : m_patterns) {
        std::smatch match;
        if (std::regex_search(input, match, pattern)) {
            found.push_back(match.str());
        }
    }
    
    return found;
}

std::string SQLInjectionDetector::sanitize(const std::string& input) const
{
    std::string result = input;
    
    // 转义单引号
    size_t pos = 0;
    while ((pos = result.find('\'', pos)) != std::string::npos) {
        result.insert(pos, "'");
        pos += 2;
    }
    
    // 转义反斜杠
    pos = 0;
    while ((pos = result.find('\\', pos)) != std::string::npos) {
        result.insert(pos, "\\");
        pos += 2;
    }
    
    return result;
}

void SQLInjectionDetector::addPattern(const std::string& pattern)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_patterns.push_back(std::regex(pattern));
}

void SQLInjectionDetector::removePattern(const std::string& pattern)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto it = m_patterns.begin(); it != m_patterns.end(); ) {
        std::string p = it->str();
        if (p == pattern) {
            it = m_patterns.erase(it);
        } else {
            ++it;
        }
    }
}

void SQLInjectionDetector::addWhitelist(const std::string& value)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_whitelist.insert(value);
}

bool SQLInjectionDetector::isWhitelisted(const std::string& value) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_whitelist.count(value) > 0;
}

void SQLInjectionDetector::clearWhitelist()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_whitelist.clear();
}

// ==================== XSSProtector 实现 ====================

XSSProtector& XSSProtector::instance()
{
    static XSSProtector instance;
    return instance;
}

XSSProtector::XSSProtector()
{
}

std::string XSSProtector::encodeHTML(const std::string& input) const
{
    std::string result;
    for (char c : input) {
        switch (c) {
            case '&': result += "&amp;"; break;
            case '<': result += "&lt;"; break;
            case '>': result += "&gt;"; break;
            case '"': result += "&quot;"; break;
            case '\'': result += "&#x27;"; break;
            case '/': result += "&#x2F;"; break;
            default: result += c; break;
        }
    }
    return result;
}

std::string XSSProtector::encodeJavaScript(const std::string& input) const
{
    std::string result;
    for (char c : input) {
        if (c == '\\') result += "\\\\";
        else if (c == '"') result += "\\\"";
        else if (c == '\'') result += "\\'";
        else if (c == '<') result += "\\x3C";
        else if (c == '>') result += "\\x3E";
        else if (c == '&') result += "\\x26";
        else result += c;
    }
    return result;
}

std::string XSSProtector::encodeURL(const std::string& input) const
{
    std::string result;
    for (char c : input) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            result += c;
        } else {
            char buf[4];
            snprintf(buf, sizeof(buf), "%%%02X", (unsigned char)c);
            result += buf;
        }
    }
    return result;
}

std::string XSSProtector::encodeCSS(const std::string& input) const
{
    std::string result;
    for (char c : input) {
        if (isalnum(c)) {
            result += c;
        } else {
            char buf[8];
            snprintf(buf, sizeof(buf), "\\%02X", (unsigned char)c);
            result += buf;
        }
    }
    return result;
}

bool XSSProtector::containsXSS(const std::string& input) const
{
    static const std::regex patterns[] = {
        std::regex(R"(<script[^>]*>.*?</script>)", std::regex::icase),
        std::regex(R"(<script[^>]*>)", std::regex::icase),
        std::regex(R"(javascript:)", std::regex::icase),
        std::regex(R"(onerror\s*=)", std::regex::icase),
        std::regex(R"(onload\s*=)", std::regex::icase),
        std::regex(R"(onclick\s*=)", std::regex::icase),
        std::regex(R"(onmouseover\s*=)", std::regex::icase),
        std::regex(R"(<iframe[^>]*>.*?</iframe>)", std::regex::icase),
        std::regex(R"(<embed[^>]*>)", std::regex::icase),
        std::regex(R"(<object[^>]*>)", std::regex::icase),
        std::regex(R"(expression\s*\()", std::regex::icase),
        std::regex(R"(url\s*\()", std::regex::icase),
    };
    
    for (const auto& pattern : patterns) {
        if (std::regex_search(input, pattern)) {
            return true;
        }
    }
    return false;
}

std::string XSSProtector::sanitize(const std::string& input) const
{
    std::string result = input;
    
    // 移除script标签
    result = std::regex_replace(result, 
        std::regex(R"(<script[^>]*>.*?</script>)", std::regex::icase), "");
    result = std::regex_replace(result, 
        std::regex(R"(<script[^>]*>)", std::regex::icase), "");
    
    // 移除事件处理器
    result = std::regex_replace(result, 
        std::regex(R"(\bon\w+\s*=)", std::regex::icase), "data-removed-");
    
    // 移除javascript:协议
    result = std::regex_replace(result, 
        std::regex(R"(javascript:)", std::regex::icase), "removed:");
    
    // 移除data:协议
    result = std::regex_replace(result, 
        std::regex(R"(data:)", std::regex::icase), "removed:");
    
    return encodeHTML(result);
}

// ==================== RateLimiter 实现 ====================

RateLimiter::RateLimiter()
    : m_policy(RateLimitPolicy::SlidingWindow)
    , m_maxRequests(100)
    , m_windowSeconds(60)
{
}

void RateLimiter::setPolicy(RateLimitPolicy policy)
{
    m_policy = policy;
}

void RateLimiter::setLimit(int maxRequests, int windowSeconds)
{
    m_maxRequests = maxRequests;
    m_windowSeconds = windowSeconds;
}

void RateLimiter::addExemption(const std::string& ip)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_exemptions.insert(ip);
}

void RateLimiter::removeExemption(const std::string& ip)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_exemptions.erase(ip);
}

RateLimitResult RateLimiter::check(const std::string& key)
{
    return check(key, "");
}

RateLimitResult RateLimiter::check(const std::string& key, const std::string& ip)
{
    RateLimitResult result;
    
    // 检查豁免
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!ip.empty() && m_exemptions.count(ip)) {
            result.allowed = true;
            result.remaining = -1;
            return result;
        }
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    int64_t now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    switch (m_policy) {
        case RateLimitPolicy::TokenBucket: {
            auto& bucket = m_buckets[key];
            int64_t elapsed = now - bucket.lastUpdate;
            bucket.tokens = std::min(m_maxRequests, 
                (int)(bucket.tokens + elapsed * m_maxRequests / m_windowSeconds));
            bucket.lastUpdate = now;
            
            if (bucket.tokens > 0) {
                bucket.tokens--;
                result.allowed = true;
                result.remaining = bucket.tokens;
            } else {
                result.allowed = false;
                result.remaining = 0;
                result.retryAfter = m_windowSeconds / m_maxRequests;
            }
            break;
        }
        
        case RateLimitPolicy::SlidingWindow:
        case RateLimitPolicy::FixedWindow: {
            int64_t windowStart = now - m_windowSeconds;
            auto& counter = m_counters[key];
            
            if (counter.first < windowStart) {
                counter.first = now;
                counter.second = 1;
            } else {
                counter.second++;
            }
            
            if (counter.second <= m_maxRequests) {
                result.allowed = true;
                result.remaining = m_maxRequests - counter.second;
            } else {
                result.allowed = false;
                result.remaining = 0;
                result.retryAfter = m_windowSeconds;
                
                if (m_callback) {
                    lock.unlock();
                    m_callback(key, ip, counter.second);
                    lock.lock();
                }
            }
            result.resetTime = counter.first + m_windowSeconds;
            break;
        }
        
        default:
            result.allowed = true;
    }
    
    return result;
}

int64_t RateLimiter::getRequestCount(const std::string& key) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_counters.find(key);
    if (it != m_counters.end()) {
        return it->second.second;
    }
    return 0;
}

void RateLimiter::reset(const std::string& key)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_counters.erase(key);
    m_buckets.erase(key);
}

void RateLimiter::resetAll()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_counters.clear();
    m_buckets.clear();
}

void RateLimiter::setOnLimitExceeded(OnLimitExceeded callback)
{
    m_callback = callback;
}

// ==================== IPBlacklist 实现 ====================

IPBlacklist& IPBlacklist::instance()
{
    static IPBlacklist instance;
    return instance;
}

IPBlacklist::IPBlacklist()
{
}

void IPBlacklist::block(const std::string& ip, const std::string& reason, int64_t duration)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    BlockedIP entry;
    entry.ip = ip;
    entry.reason = reason;
    entry.blockedAt = time(nullptr);
    entry.expiresAt = duration > 0 ? entry.blockedAt + duration : -1;
    
    m_blocked[ip] = entry;
    
    Logger::instance().warn("IP {} 被加入黑名单: {}", ip, reason);
}

void IPBlacklist::unblock(const std::string& ip)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_blocked.find(ip);
    if (it != m_blocked.end()) {
        Logger::instance().info("IP {} 从黑名单移除", ip);
        m_blocked.erase(it);
    }
}

bool IPBlacklist::isBlocked(const std::string& ip) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_blocked.find(ip);
    if (it == m_blocked.end()) {
        return false;
    }
    
    // 检查是否过期
    if (it->second.expiresAt > 0 && time(nullptr) > it->second.expiresAt) {
        return false;
    }
    
    return true;
}

size_t IPBlacklist::size() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_blocked.size();
}

std::vector<std::string> IPBlacklist::getBlockedIPs() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<std::string> result;
    for (const auto& [ip, entry] : m_blocked) {
        result.push_back(ip);
    }
    return result;
}

bool IPBlacklist::loadFromFile(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        // 解析格式: IP,reason,duration
        auto parts = split(line, ',');
        if (parts.size() >= 1) {
            BlockedIP entry;
            entry.ip = parts[0];
            entry.reason = parts.size() > 1 ? parts[1] : "";
            entry.expiresAt = parts.size() > 2 ? std::stoll(parts[2]) : -1;
            entry.blockedAt = time(nullptr);
            m_blocked[entry.ip] = entry;
        }
    }
    
    return true;
}

bool IPBlacklist::saveToFile(const std::string& filepath) const
{
    std::ofstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    file << "# IP Blacklist\n";
    file << "# Format: IP,reason,duration_seconds(-1 for permanent)\n\n";
    
    for (const auto& [ip, entry] : m_blocked) {
        file << ip << "," << entry.reason << "," << entry.expiresAt << "\n";
    }
    
    return true;
}

// ==================== CSRFProtector 实现 ====================

CSRFProtector& CSRFProtector::instance()
{
    static CSRFProtector instance;
    return instance;
}

CSRFProtector::CSRFProtector()
    : m_tokenExpiry(3600)
{
}

std::string CSRFProtector::generateRandomToken()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    std::ostringstream oss;
    for (int i = 0; i < 32; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << dis(gen);
    }
    return oss.str();
}

std::string CSRFProtector::generateToken(const std::string& sessionId)
{
    std::string token = generateRandomToken();
    int64_t hash = hashToken(token);
    int64_t now = time(nullptr);
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    TokenInfo info;
    info.hash = hash;
    info.expiresAt = now + m_tokenExpiry;
    info.createdAt = now;
    
    m_sessionTokens[sessionId].push_back(info);
    
    return token;
}

std::string CSRFProtector::generateTokenForUser(int userId)
{
    std::string token = generateRandomToken();
    int64_t hash = hashToken(token);
    int64_t now = time(nullptr);
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    TokenInfo info;
    info.hash = hash;
    info.expiresAt = now + m_tokenExpiry;
    info.createdAt = now;
    
    m_userTokens[userId].push_back(info);
    
    return token;
}

int64_t CSRFProtector::hashToken(const std::string& token) const
{
    std::hash<std::string> hasher;
    return hasher(token + m_tokenExpiry);
}

bool CSRFProtector::validateToken(const std::string& sessionId, const std::string& token)
{
    int64_t hash = hashToken(token);
    int64_t now = time(nullptr);
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto sessionIt = m_sessionTokens.find(sessionId);
    if (sessionIt == m_sessionTokens.end()) {
        return false;
    }
    
    for (auto it = sessionIt->second.begin(); it != sessionIt->second.end(); ) {
        if (it->expiresAt < now) {
            it = sessionIt->second.erase(it);
        } else if (it->hash == hash) {
            return true;
        } else {
            ++it;
        }
    }
    
    return false;
}

bool CSRFProtector::validateTokenForUser(int userId, const std::string& token)
{
    int64_t hash = hashToken(token);
    int64_t now = time(nullptr);
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto userIt = m_userTokens.find(userId);
    if (userIt == m_userTokens.end()) {
        return false;
    }
    
    for (auto it = userIt->second.begin(); it != userIt->second.end(); ) {
        if (it->expiresAt < now) {
            it = userIt->second.erase(it);
        } else if (it->hash == hash) {
            return true;
        } else {
            ++it;
        }
    }
    
    return false;
}

void CSRFProtector::invalidateToken(const std::string& sessionId)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sessionTokens.erase(sessionId);
}

void CSRFProtector::invalidateUserTokens(int userId)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_userTokens.erase(userId);
}

void CSRFProtector::setTokenExpiry(int64_t seconds)
{
    m_tokenExpiry = seconds;
}

// ==================== SecurityAudit 实现 ====================

SecurityAudit& SecurityAudit::instance()
{
    static SecurityAudit instance;
    return instance;
}

SecurityAudit::SecurityAudit()
    : m_maxEvents(10000)
    , m_retentionDays(90)
{
}

void SecurityAudit::log(SecurityEvent event, const std::string& ip,
                        const std::string& userId, const std::string& details,
                        bool blocked)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    SecurityEventRecord record;
    record.timestamp = time(nullptr);
    record.event = event;
    record.ip = ip;
    record.user_id = userId;
    record.details = details;
    record.blocked = blocked;
    
    m_events.push_back(record);
    
    // 清理旧事件
    if (m_events.size() > m_maxEvents) {
        m_events.erase(m_events.begin());
    }
    
    // 记录到日志
    Logger::instance().warn("安全事件: {} from {} by {} - {} (blocked: {})",
        getEventName(event), ip, userId, details, blocked);
}

void SecurityAudit::logSQLInjection(const std::string& ip, const std::string& sql)
{
    log(SecurityEvent::SQLInjection, ip, "", "SQL: " + sql.substr(0, 100), true);
}

void SecurityAudit::logXSS(const std::string& ip, const std::string& content)
{
    log(SecurityEvent::XSSAttack, ip, "", "XSS: " + content.substr(0, 100), true);
}

void SecurityAudit::logBruteForce(const std::string& ip, const std::string& username)
{
    log(SecurityEvent::BruteForce, ip, username, "暴力破解尝试", true);
}

void SecurityAudit::logRateLimit(const std::string& ip, int count)
{
    log(SecurityEvent::RateLimit, ip, "", 
        std::to_string(count) + " requests", true);
}

std::vector<SecurityEventRecord> SecurityAudit::query(
    SecurityEvent event, const std::string& ip,
    int64_t since, int64_t until, int limit)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<SecurityEventRecord> result;
    
    for (const auto& record : m_events) {
        if (since > 0 && record.timestamp < since) continue;
        if (until > 0 && record.timestamp > until) continue;
        if (!ip.empty() && record.ip != ip) continue;
        
        result.push_back(record);
        
        if ((int)result.size() >= limit) break;
    }
    
    return result;
}

SecurityAudit::EventStats SecurityAudit::getStats(int64_t since) const
{
    EventStats stats;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    for (const auto& record : m_events) {
        if (since > 0 && record.timestamp < since) continue;
        
        stats.totalEvents++;
        if (record.blocked) stats.blockedEvents++;
        stats.byIP[record.ip]++;
        if (!record.user_id.empty()) {
            stats.byUser[record.user_id]++;
        }
    }
    
    return stats;
}

void SecurityAudit::setMaxEventsPerMinute(int max)
{
    m_maxEvents = max * 60;
}

void SecurityAudit::setRetentionDays(int days)
{
    m_retentionDays = days;
}

std::string SecurityAudit::getEventName(SecurityEvent event) const
{
    switch (event) {
        case SecurityEvent::SQLInjection: return "SQL注入";
        case SecurityEvent::XSSAttack: return "XSS攻击";
        case SecurityEvent::CSRFAttack: return "CSRF攻击";
        case SecurityEvent::BruteForce: return "暴力破解";
        case SecurityEvent::RateLimit: return "频率超限";
        case SecurityEvent::InvalidToken: return "无效令牌";
        case SecurityEvent::PrivilegeEscalation: return "权限提升";
        case SecurityEvent::DataLeak: return "数据泄露";
        case SecurityEvent::SuspiciousIP: return "可疑IP";
        case SecurityEvent::SessionHijack: return "会话劫持";
        default: return "未知";
    }
}

std::string SecurityAudit::sanitizeForLog(const std::string& input) const
{
    std::string result = input;
    // 移除敏感信息
    result = std::regex_replace(result, std::regex(R"(\b\d{11,}\b)"), "****");
    result = std::regex_replace(result, std::regex(R"(password\s*=\s*\S+)"), "password=****");
    result = std::regex_replace(result, std::regex(R"(token\s*=\s*\S+)"), "token=****");
    return result;
}

// ==================== SecurityService 实现 ====================

SecurityService& SecurityService::instance()
{
    static SecurityService instance;
    return instance;
}

SecurityService::SecurityService()
{
}

void SecurityService::init()
{
    SQLInjectionDetector::instance();
    XSSProtector::instance();
    CSRFProtector::instance();
    SecurityAudit::instance();
    Logger::instance().info("安全服务初始化完成");
}

SecurityService::ValidationResult SecurityService::validateRequest(
    const std::string& ip, const std::string& userId,
    const std::map<std::string, std::string>& params,
    const std::string& csrfToken, const std::string& sessionId)
{
    ValidationResult result;
    result.valid = true;
    
    // IP黑名单检查
    if (IPBlacklist::instance().isBlocked(ip)) {
        result.valid = false;
        result.error = "IP已被禁止";
        result.shouldBlock = true;
        result.event = SecurityEvent::SuspiciousIP;
        return result;
    }
    
    // 频率限制检查
    if (m_rateLimitingEnabled) {
        RateLimiter limiter;
        limiter.setLimit(100, 60);
        auto rateResult = limiter.check(userId.empty() ? ip : userId, ip);
        
        if (!rateResult.allowed) {
            SecurityAudit::instance().logRateLimit(ip, 100);
            
            if (rateResult.retryAfter > 30) {
                IPBlacklist::instance().block(ip, "频率超限", 300);
            }
            
            result.valid = false;
            result.error = "请求过于频繁";
            result.shouldBlock = true;
            result.event = SecurityEvent::RateLimit;
            return result;
        }
    }
    
    // SQL注入检查
    if (m_sqlProtectionEnabled) {
        auto& detector = SQLInjectionDetector::instance();
        
        for (const auto& [key, value] : params) {
            if (detector.isDangerous(value)) {
                SecurityAudit::instance().logSQLInjection(ip, value);
                
                result.valid = false;
                result.error = "检测到非法输入";
                result.shouldBlock = true;
                result.event = SecurityEvent::SQLInjection;
                return result;
            }
        }
    }
    
    // CSRF检查
    if (m_csrfProtectionEnabled && !csrfToken.empty()) {
        if (!sessionId.empty() && 
            !CSRFProtector::instance().validateToken(sessionId, csrfToken)) {
            SecurityAudit::instance().log(SecurityEvent::CSRFAttack, ip, userId);
            
            result.valid = false;
            result.error = "CSRF令牌无效";
            result.shouldBlock = true;
            result.event = SecurityEvent::CSRFAttack;
            return result;
        }
    }
    
    return result;
}

SecurityService::ValidationResult SecurityService::validateSQLParams(
    const std::map<std::string, std::string>& params)
{
    ValidationResult result;
    result.valid = true;
    
    auto& detector = SQLInjectionDetector::instance();
    
    for (const auto& [key, value] : params) {
        if (detector.isDangerous(value)) {
            result.valid = false;
            result.error = "检测到SQL注入特征";
            result.shouldBlock = true;
            result.event = SecurityEvent::SQLInjection;
            return result;
        }
    }
    
    return result;
}

std::string SecurityService::encodeOutput(const std::string& input, 
                                            const std::string& context)
{
    auto& protector = XSSProtector::instance();
    
    if (context == "html") {
        return protector.encodeHTML(input);
    } else if (context == "js") {
        return protector.encodeJavaScript(input);
    } else if (context == "url") {
        return protector.encodeURL(input);
    } else if (context == "css") {
        return protector.encodeCSS(input);
    }
    
    return protector.encodeHTML(input);
}

bool SecurityService::isIPAllowed(const std::string& ip)
{
    return !IPBlacklist::instance().isBlocked(ip);
}

void SecurityService::enableSQLProtection(bool enable)
{
    m_sqlProtectionEnabled = enable;
}

void SecurityService::enableXSSProtection(bool enable)
{
    m_xssProtectionEnabled = enable;
}

void SecurityService::enableCSRFProtection(bool enable)
{
    m_csrfProtectionEnabled = enable;
}

void SecurityService::enableRateLimiting(bool enable)
{
    m_rateLimitingEnabled = enable;
}

void SecurityService::setRateLimit(int maxRequests, int windowSeconds)
{
    // 配置将在check时生效
}

} // namespace smartsched
