/**
 * @file auth.cpp
 * @brief JWT认证与权限控制实现
 */
#include "auth.h"
#include "logger.h"
#include "../utils/json_helper.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>
#include <openssl/sha.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
    #include <sys/time.h>
#endif

namespace smartsched {

// ==================== JWTPayload 实现 ====================

std::string JWTPayload::toJson() const
{
    json::JSON obj;
    obj["user_id"] = user_id;
    obj["username"] = username;
    obj["role"] = (int)role;
    obj["device_id"] = device_id;
    obj["permissions"] = json::Array();
    for (const auto& p : permissions) {
        obj["permissions"].append(p);
    }
    obj["issued_at"] = issued_at;
    obj["expires_at"] = expires_at;
    return obj.dump();
}

bool JWTPayload::fromJson(const std::string& jsonStr)
{
    try {
        auto obj = json::parse(jsonStr);
        user_id = obj["user_id"].toInt();
        username = obj["username"].toString();
        role = (Role)obj["role"].toInt();
        device_id = obj["device_id"].toString();
        issued_at = obj["issued_at"].toInt64();
        expires_at = obj["expires_at"].toInt64();
        
        for (const auto& p : obj["permissions"].array()) {
            permissions.push_back(p.toString());
        }
        return true;
    } catch (...) {
        return false;
    }
}

// ==================== AuthService 实现 ====================

AuthService& AuthService::instance()
{
    static AuthService instance;
    return instance;
}

AuthService::AuthService()
{
    // 初始化密钥（生产环境应从配置文件读取）
    m_secretKey = "SmartSched-HIS-Secret-Key-2024";
    
    // 初始化角色权限
    // 超级管理员 - 全部权限
    for (int i = 1000; i < 7000; ++i) {
        m_rolePermissions[Role::SuperAdmin].insert((Permission)i);
    }
    
    // 管理员
    m_rolePermissions[Role::Admin] = {
        Permission::SystemConfig,
        Permission::UserManage,
        Permission::RoleManage,
        Permission::AuditView,
        Permission::DeptManage,
        Permission::DeptStatistics,
        Permission::DataExport,
        Permission::DataView,
    };
    
    // 医生
    m_rolePermissions[Role::Doctor] = {
        Permission::DoctorView,
        Permission::DoctorCall,
        Permission::DoctorFinish,
        Permission::DoctorTransfer,
        Permission::PatientQuery,
        Permission::DataView,
    };
    
    // 护士
    m_rolePermissions[Role::Nurse] = {
        Permission::DoctorView,
        Permission::DoctorCall,
        Permission::PatientQuery,
    };
    
    // 患者
    m_rolePermissions[Role::Patient] = {
        Permission::PatientRegister,
        Permission::PatientQuery,
        Permission::PatientCancel,
    };
    
    // 访客
    m_rolePermissions[Role::Guest] = {
        Permission::DataView,
    };
}

AuthService::~AuthService()
{
}

std::string AuthService::generateSignature(const std::string& header, const std::string& payload)
{
    std::string data = header + "." + payload;
    
    unsigned char* digest = HMAC(EVP_sha256(), 
                                  m_secretKey.c_str(), m_secretKey.length(),
                                  (unsigned char*)data.c_str(), data.length(),
                                  NULL, NULL);
    
    std::ostringstream oss;
    for (int i = 0; i < 32; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)digest[i];
    }
    return oss.str();
}

std::string AuthService::base64Encode(const std::string& input)
{
    static const char* base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";
    
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    int in_len = (int)input.size();
    int o = 0;
    const unsigned char* bytes = (const unsigned char*)input.c_str();
    
    while (in_len--) {
        char_array_3[i++] = *(bytes++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            
            for (i = 0; i < 4; i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }
    
    if (i) {
        for (j = i; j < 3; j++)
            char_array_3[j] = '\0';
        
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        
        for (j = 0; j < i + 1; j++)
            ret += base64_chars[char_array_4[j]];
        
        while ((i++ < 3))
            ret += '=';
    }
    
    // URL-safe base64
    std::replace(ret.begin(), ret.end(), '+', '-');
    std::replace(ret.begin(), ret.end(), '/', '_');
    ret.erase(std::remove(ret.begin(), ret.end(), '='), ret.end());
    
    return ret;
}

std::string AuthService::base64Decode(const std::string& input)
{
    std::string output;
    std::vector<int> T(256, -1);
    static const std::string src = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    
    for (int i = 0; i < 64; i++) T[src[i]] = i;
    
    int val = 0, bits = -8;
    for (char c : input) {
        if (T[(int)c] == -1) break;
        val = (val << 6) + T[(int)c];
        bits += 6;
        if (bits >= 0) {
            output.push_back(char((val >> bits) & 0xFF));
            bits -= 8;
        }
    }
    return output;
}

std::string AuthService::generateToken(const JWTPayload& payload)
{
    // Header
    json::JSON header;
    header["alg"] = "HS256";
    header["typ"] = "JWT";
    std::string headerJson = header.dump();
    std::string headerEncoded = base64Encode(headerJson);
    
    // Payload
    std::string payloadJson = payload.toJson();
    std::string payloadEncoded = base64Encode(payloadJson);
    
    // Signature
    std::string signature = generateSignature(headerEncoded, payloadEncoded);
    
    return headerEncoded + "." + payloadEncoded + "." + signature;
}

bool AuthService::verifySignature(const std::string& token)
{
    auto parts = split(token, '.');
    if (parts.size() != 3) return false;
    
    std::string expectedSig = generateSignature(parts[0], parts[1]);
    return expectedSig == parts[2];
}

std::string AuthService::hashPassword(const std::string& password, const std::string& salt)
{
    std::string data = salt + password + m_secretKey;
    
    unsigned char* digest = SHA256((unsigned char*)data.c_str(), data.length(), NULL);
    
    std::ostringstream oss;
    for (int i = 0; i < 32; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)digest[i];
    }
    return oss.str();
}

bool AuthService::validateCredentials(const std::string& username, const std::string& password)
{
    // 从数据库验证用户凭证
    // 这里简化处理，实际应查询数据库
    // SELECT * FROM user WHERE username = ? AND password_hash = ?
    // 这里需要实现数据库查询逻辑
    
    // TODO: 实际实现应连接数据库验证
    return !username.empty() && !password.empty();
}

AuthResult AuthService::login(const std::string& username, const std::string& password,
                              const std::string& device_id)
{
    AuthResult result;
    
    // 检查账户是否被锁定
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_lockedAccounts.find(username);
        if (it != m_lockedAccounts.end()) {
            auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(
                std::chrono::steady_clock::now() - it->second).count();
            if (elapsed < 30) {  // 锁定30分钟
                result.message = "账户已被锁定，请30分钟后再试";
                return result;
            } else {
                m_lockedAccounts.erase(it);
            }
        }
    }
    
    // 验证凭证
    if (!validateCredentials(username, password)) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_failedAttempts[username]++;
        
        if (m_failedAttempts[username] >= m_maxFailedAttempts) {
            m_lockedAccounts[username] = std::chrono::steady_clock::now();
            result.message = "连续登录失败次数过多，账户已被锁定";
            Logger::instance().warn("账户 {} 因连续登录失败被锁定", username);
            return result;
        }
        
        result.message = "用户名或密码错误";
        return result;
    }
    
    // 查询用户信息和角色
    // SELECT user_id, role FROM user WHERE username = ?
    // TODO: 从数据库获取
    
    JWTPayload payload;
    payload.user_id = 1;  // TODO: 实际从数据库获取
    payload.username = username;
    payload.role = Role::Admin;  // TODO: 实际从数据库获取
    payload.device_id = device_id;
    payload.issued_at = time(nullptr);
    payload.expires_at = payload.issued_at + m_tokenExpiry;
    
    // 添加权限
    auto perms = getRolePermissions(payload.role);
    for (auto p : perms) {
        payload.permissions.push_back(std::to_string((int)p));
    }
    
    // 生成Token
    std::string token = generateToken(payload);
    
    // 缓存Token
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_tokenCache[token] = payload;
        m_failedAttempts.erase(username);
    }
    
    result.success = true;
    result.message = "登录成功";
    result.token = token;
    result.role = payload.role;
    result.user_id = payload.user_id;
    
    Logger::instance().info("用户 {} 登录成功，角色: {}", username, (int)payload.role);
    
    return result;
}

bool AuthService::logout(const std::string& token)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_tokenCache.find(token);
    if (it != m_tokenCache.end()) {
        Logger::instance().info("用户 {} 登出", it->second.username);
        m_tokenCache.erase(it);
        return true;
    }
    return false;
}

bool AuthService::refreshToken(const std::string& oldToken, std::string& newToken)
{
    JWTPayload payload;
    if (!verifyToken(oldToken, payload)) {
        return false;
    }
    
    // 更新时间戳
    payload.issued_at = time(nullptr);
    payload.expires_at = payload.issued_at + m_tokenExpiry;
    
    newToken = generateToken(payload);
    
    std::lock_guard<std::mutex> lock(m_mutex);
    m_tokenCache.erase(oldToken);
    m_tokenCache[newToken] = payload;
    
    return true;
}

bool AuthService::verifyToken(const std::string& token)
{
    JWTPayload payload;
    return verifyToken(token, payload);
}

bool AuthService::verifyToken(const std::string& token, JWTPayload& payload)
{
    // 检查缓存
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_tokenCache.find(token);
        if (it != m_tokenCache.end()) {
            payload = it->second;
            
            // 检查是否过期
            if (time(nullptr) > payload.expires_at) {
                m_tokenCache.erase(it);
                return false;
            }
            return true;
        }
    }
    
    // 解析Token
    if (!verifySignature(token)) {
        return false;
    }
    
    auto parts = split(token, '.');
    if (parts.size() != 3) return false;
    
    std::string payloadJson = base64Decode(parts[1]);
    if (!payload.fromJson(payloadJson)) {
        return false;
    }
    
    // 检查是否过期
    if (time(nullptr) > payload.expires_at) {
        return false;
    }
    
    // 缓存
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_tokenCache[token] = payload;
    }
    
    return true;
}

std::optional<JWTPayload> AuthService::parseToken(const std::string& token)
{
    JWTPayload payload;
    if (verifyToken(token, payload)) {
        return payload;
    }
    return std::nullopt;
}

bool AuthService::hasPermission(const std::string& token, Permission perm)
{
    JWTPayload payload;
    if (!verifyToken(token, payload)) {
        return false;
    }
    return hasPermission(payload.role, perm);
}

bool AuthService::hasPermission(Role role, Permission perm)
{
    auto it = m_rolePermissions.find(role);
    if (it == m_rolePermissions.end()) {
        return false;
    }
    return it->second.count(perm) > 0;
}

bool AuthService::hasAnyPermission(const std::string& token, const std::vector<Permission>& perms)
{
    for (auto p : perms) {
        if (hasPermission(token, p)) return true;
    }
    return false;
}

bool AuthService::hasAllPermissions(const std::string& token, const std::vector<Permission>& perms)
{
    for (auto p : perms) {
        if (!hasPermission(token, p)) return false;
    }
    return true;
}

bool AuthService::changePassword(int user_id, const std::string& old_password,
                                  const std::string& new_password)
{
    // TODO: 实现密码修改逻辑
    // 1. 验证旧密码
    // 2. 加密新密码
    // 3. 更新数据库
    return true;
}

bool AuthService::lockAccount(const std::string& username)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lockedAccounts[username] = std::chrono::steady_clock::now();
    return true;
}

bool AuthService::unlockAccount(const std::string& username)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lockedAccounts.erase(username);
    m_failedAttempts.erase(username);
    return true;
}

void AuthService::addRolePermission(Role role, Permission perm)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_rolePermissions[role].insert(perm);
}

void AuthService::removeRolePermission(Role role, Permission perm)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_rolePermissions[role].erase(perm);
}

std::vector<Permission> AuthService::getRolePermissions(Role role)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<Permission> result;
    auto it = m_rolePermissions.find(role);
    if (it != m_rolePermissions.end()) {
        result.assign(it->second.begin(), it->second.end());
    }
    return result;
}

void AuthService::setTokenExpiry(int64_t seconds)
{
    m_tokenExpiry = seconds;
}

void AuthService::setMaxFailedAttempts(int count)
{
    m_maxFailedAttempts = count;
}

void AuthService::enableIPWhitelist(const std::vector<std::string>& ips)
{
    m_ipWhitelist = ips;
    m_ipWhitelistEnabled = true;
}

void AuthService::disableIPWhitelist()
{
    m_ipWhitelistEnabled = false;
}

// ==================== PermissionChecker 实现 ====================

PermissionChecker::PermissionChecker(const std::string& token)
    : m_token(token), m_role(Role::Guest), m_userId(0)
{
    auto payload = AuthService::instance().parseToken(token);
    if (payload) {
        m_role = payload->role;
        m_userId = payload->user_id;
    }
}

PermissionChecker::~PermissionChecker()
{
}

bool PermissionChecker::check(Permission perm)
{
    return AuthService::instance().hasPermission(m_token, perm);
}

bool PermissionChecker::checkAny(const std::vector<Permission>& perms)
{
    return AuthService::instance().hasAnyPermission(m_token, perms);
}

bool PermissionChecker::checkAll(const std::vector<Permission>& perms)
{
    return AuthService::instance().hasAllPermissions(m_token, perms);
}

// ==================== SessionManager 实现 ====================

SessionManager& SessionManager::instance()
{
    static SessionManager instance;
    return instance;
}

SessionManager::SessionManager()
{
}

SessionManager::~SessionManager()
{
}

std::string SessionManager::generateSessionId()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::ostringstream oss;
    for (int i = 0; i < 32; ++i) {
        oss << std::hex << dis(gen);
    }
    return oss.str();
}

std::string SessionManager::createSession(const Session& session)
{
    std::string sessionId = generateSessionId();
    
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sessions[sessionId] = session;
    m_userSessions[session.user_id].insert(sessionId);
    
    return sessionId;
}

bool SessionManager::destroySession(const std::string& session_id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_sessions.find(session_id);
    if (it != m_sessions.end()) {
        m_userSessions[it->second.user_id].erase(session_id);
        m_sessions.erase(it);
        return true;
    }
    return false;
}

bool SessionManager::destroyUserSessions(int user_id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_userSessions.find(user_id);
    if (it != m_userSessions.end()) {
        for (const auto& sid : it->second) {
            m_sessions.erase(sid);
        }
        m_userSessions.erase(it);
        return true;
    }
    return false;
}

std::optional<Session> SessionManager::getSession(const std::string& session_id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_sessions.find(session_id);
    if (it != m_sessions.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<Session> SessionManager::getOnlineUsers()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<Session> result;
    for (const auto& [id, session] : m_sessions) {
        result.push_back(session);
    }
    return result;
}

int SessionManager::getOnlineCount()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return (int)m_sessions.size();
}

int SessionManager::getOnlineCountByRole(Role role)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    int count = 0;
    for (const auto& [id, session] : m_sessions) {
        // 需要从AuthService获取用户角色
        // 这里简化处理
    }
    return count;
}

void SessionManager::cleanupExpiredSessions()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto now = std::chrono::steady_clock::now();
    auto timeout = std::chrono::minutes(30);
    
    std::vector<std::string> expired;
    for (const auto& [id, session] : m_sessions) {
        if (now - session.last_active > timeout) {
            expired.push_back(id);
        }
    }
    
    for (const auto& id : expired) {
        int uid = m_sessions[id].user_id;
        m_userSessions[uid].erase(id);
        m_sessions.erase(id);
    }
}

void SessionManager::cleanupUserSessions(int user_id)
{
    destroyUserSessions(user_id);
}

void SessionManager::setAttribute(const std::string& session_id, const std::string& key,
                                  const std::string& value)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_sessions.find(session_id);
    if (it != m_sessions.end()) {
        it->second.attributes[key] = value;
        it->second.last_active = std::chrono::steady_clock::now();
    }
}

std::optional<std::string> SessionManager::getAttribute(const std::string& session_id,
                                                        const std::string& key)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_sessions.find(session_id);
    if (it != m_sessions.end()) {
        auto attrIt = it->second.attributes.find(key);
        if (attrIt != it->second.attributes.end()) {
            return attrIt->second;
        }
    }
    return std::nullopt;
}

} // namespace smartsched
