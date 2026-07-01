/**
 * @file auth.h
 * @brief JWT认证与权限控制模块
 */
#ifndef SMARTSCHED_AUTH_H
#define SMARTSCHED_AUTH_H

#include <string>
#include <map>
#include <vector>
#include <set>
#include <chrono>
#include <memory>

namespace smartsched {

// ==================== 角色与权限定义 ====================

/**
 * @brief 系统角色枚举
 */
enum class Role : int {
    SuperAdmin = 0,    // 超级管理员
    Admin = 1,         // 管理员
    Doctor = 2,        // 医生
    Nurse = 3,         // 护士
    Patient = 4,       // 患者
    Guest = 5          // 访客
};

/**
 * @brief 权限枚举
 */
enum class Permission : int {
    // 系统管理
    SystemConfig = 1000,      // 系统配置
    UserManage = 1001,        // 用户管理
    RoleManage = 1002,        // 角色管理
    AuditView = 1003,         // 审计查看
    
    // 科室管理
    DeptManage = 2000,        // 科室管理
    DeptStatistics = 2001,    // 科室统计
    
    // 医生操作
    DoctorView = 3000,         // 查看排班
    DoctorCall = 3001,         // 叫号
    DoctorFinish = 3002,       // 完成诊疗
    DoctorTransfer = 3003,     // 转诊
    
    // 患者操作
    PatientRegister = 4000,    // 挂号
    PatientQuery = 4001,       // 查询排队
    PatientCancel = 4002,     // 取消挂号
    
    // B超室
    BUltraManage = 5000,      // B超管理
    BUltraCall = 5001,         // B超叫号
    BUltraFinish = 5002,      // B超完成
    
    // 数据操作
    DataExport = 6000,         // 数据导出
    DataView = 6001,           // 数据查看
    DataDelete = 6002          // 数据删除
};

/**
 * @brief 权限检查结果
 */
struct AuthResult {
    bool success;
    std::string message;
    std::string token;
    Role role;
    int user_id;
    
    AuthResult() : success(false), role(Role::Guest), user_id(0) {}
};

/**
 * @brief JWT Token结构
 */
struct JWTPayload {
    int user_id;
    std::string username;
    Role role;
    std::string device_id;
    std::vector<std::string> permissions;
    int64_t issued_at;      // 签发时间
    int64_t expires_at;     // 过期时间
    
    std::string toJson() const;
    bool fromJson(const std::string& json);
};

// ==================== 认证服务 ====================

/**
 * @brief 认证服务 - 单例模式
 */
class AuthService {
public:
    static AuthService& instance();
    
    // 认证
    AuthResult login(const std::string& username, const std::string& password, 
                     const std::string& device_id = "");
    bool logout(const std::string& token);
    bool refreshToken(const std::string& oldToken, std::string& newToken);
    
    // Token验证
    bool verifyToken(const std::string& token);
    bool verifyToken(const std::string& token, JWTPayload& payload);
    
    // 权限检查
    bool hasPermission(const std::string& token, Permission perm);
    bool hasPermission(Role role, Permission perm);
    bool hasAnyPermission(const std::string& token, const std::vector<Permission>& perms);
    bool hasAllPermissions(const std::string& token, const std::vector<Permission>& perms);
    
    // Token解析
    std::optional<JWTPayload> parseToken(const std::string& token);
    
    // 用户管理
    bool changePassword(int user_id, const std::string& old_password, 
                        const std::string& new_password);
    bool lockAccount(const std::string& username);
    bool unlockAccount(const std::string& username);
    
    // 角色权限配置
    void addRolePermission(Role role, Permission perm);
    void removeRolePermission(Role role, Permission perm);
    std::vector<Permission> getRolePermissions(Role role);
    
    // 安全配置
    void setTokenExpiry(int64_t seconds);
    void setMaxFailedAttempts(int count);
    void enableIPWhitelist(const std::vector<std::string>& ips);
    void disableIPWhitelist();

private:
    AuthService();
    ~AuthService();
    AuthService(const AuthService&) = delete;
    AuthService& operator=(const AuthService&) = delete;
    
    std::string generateToken(const JWTPayload& payload);
    std::string generateSignature(const std::string& header, const std::string& payload);
    bool verifySignature(const std::string& token);
    
    std::string base64Encode(const std::string& input);
    std::string base64Decode(const std::string& input);
    
    bool validateCredentials(const std::string& username, const std::string& password);
    std::string hashPassword(const std::string& password, const std::string& salt);
    
    // 内存中的Token缓存
    std::map<std::string, JWTPayload> m_tokenCache;
    std::mutex m_mutex;
    
    // 角色权限映射
    std::map<Role, std::set<Permission>> m_rolePermissions;
    
    // 安全配置
    std::string m_secretKey;
    int64_t m_tokenExpiry = 3600;        // 1小时
    int m_maxFailedAttempts = 5;
    std::vector<std::string> m_ipWhitelist;
    bool m_ipWhitelistEnabled = false;
    
    // 登录失败记录
    std::map<std::string, int> m_failedAttempts;
    std::map<std::string, std::chrono::steady_clock::time_point> m_lockedAccounts;
};

// ==================== 权限装饰器 ====================

/**
 * @brief 权限检查辅助类
 */
class PermissionChecker {
public:
    PermissionChecker(const std::string& token);
    ~PermissionChecker();
    
    bool check(Permission perm);
    bool checkAny(const std::vector<Permission>& perms);
    bool checkAll(const std::vector<Permission>& perms);
    
    Role getRole() const { return m_role; }
    int getUserId() const { return m_userId; }

private:
    std::string m_token;
    Role m_role;
    int m_userId;
};

// ==================== 会话管理 ====================

/**
 * @brief 用户会话信息
 */
struct Session {
    int user_id;
    std::string username;
    std::string token;
    std::string ip_address;
    std::string user_agent;
    std::chrono::steady_clock::time_point login_time;
    std::chrono::steady_clock::time_point last_active;
    std::map<std::string, std::string> attributes;
};

/**
 * @brief 会话管理器
 */
class SessionManager {
public:
    static SessionManager& instance();
    
    // 会话操作
    std::string createSession(const Session& session);
    bool destroySession(const std::string& session_id);
    bool destroyUserSessions(int user_id);
    std::optional<Session> getSession(const std::string& session_id);
    
    // 在线用户
    std::vector<Session> getOnlineUsers();
    int getOnlineCount();
    int getOnlineCountByRole(Role role);
    
    // 清理
    void cleanupExpiredSessions();
    void cleanupUserSessions(int user_id);
    
    // 属性管理
    void setAttribute(const std::string& session_id, const std::string& key, const std::string& value);
    std::optional<std::string> getAttribute(const std::string& session_id, const std::string& key);

private:
    SessionManager();
    ~SessionManager();
    
    std::string generateSessionId();
    
    std::map<std::string, Session> m_sessions;
    std::map<int, std::set<std::string>> m_userSessions;  // user_id -> session_ids
    std::mutex m_mutex;
};

} // namespace smartsched

#endif // SMARTSCHED_AUTH_H
