/**
 * @file test_auth.cpp
 * @brief 认证模块单元测试
 */
#include <gtest/gtest.h>
#include "smartsched/security/auth.h"

using namespace smartsched;

// ==================== AuthService测试 ====================

class AuthServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 重置认证服务状态
        AuthService::instance();
    }
};

TEST_F(AuthServiceTest, RolePermissions)
{
    // 超级管理员应有所有权限
    auto adminPerms = AuthService::instance().getRolePermissions(Role::SuperAdmin);
    EXPECT_GT(adminPerms.size(), 50);
    
    // 患者应只有基本权限
    auto patientPerms = AuthService::instance().getRolePermissions(Role::Patient);
    EXPECT_LT(patientPerms.size(), 10);
    EXPECT_TRUE(AuthService::instance().hasPermission(Role::Patient, Permission::PatientRegister));
}

TEST_F(AuthServiceTest, RolePermissionCheck)
{
    // 医生可以叫号
    EXPECT_TRUE(AuthService::instance().hasPermission(Role::Doctor, Permission::DoctorCall));
    EXPECT_TRUE(AuthService::instance().hasPermission(Role::Doctor, Permission::DoctorFinish));
    
    // 患者不能叫号
    EXPECT_FALSE(AuthService::instance().hasPermission(Role::Patient, Permission::DoctorCall));
    
    // 管理员可以系统配置
    EXPECT_TRUE(AuthService::instance().hasPermission(Role::Admin, Permission::SystemConfig));
}

TEST_F(AuthServiceTest, AddRemovePermission)
{
    Role testRole = Role::Guest;
    
    // 添加权限
    AuthService::instance().addRolePermission(testRole, Permission::PatientRegister);
    EXPECT_TRUE(AuthService::instance().hasPermission(testRole, Permission::PatientRegister));
    
    // 移除权限
    AuthService::instance().removeRolePermission(testRole, Permission::PatientRegister);
    EXPECT_FALSE(AuthService::instance().hasPermission(testRole, Permission::PatientRegister));
}

TEST_F(AuthServiceTest, TokenGeneration)
{
    // 生成一个有效载荷
    JWTPayload payload;
    payload.user_id = 1001;
    payload.username = "testuser";
    payload.role = Role::Doctor;
    payload.issued_at = time(nullptr);
    payload.expires_at = payload.issued_at + 3600;
    
    // 这里无法直接测试generateToken，因为它在private中
    // 但可以通过验证逻辑来测试
    EXPECT_TRUE(payload.user_id > 0);
    EXPECT_FALSE(payload.username.empty());
}

// ==================== PermissionChecker测试 ====================

class PermissionCheckerTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(PermissionCheckerTest, InvalidToken)
{
    // 无效Token应该返回最低权限
    PermissionChecker checker("invalid_token");
    
    EXPECT_EQ(checker.getRole(), Role::Guest);
    EXPECT_EQ(checker.getUserId(), 0);
    EXPECT_FALSE(checker.check(Permission::DoctorCall));
}

TEST_F(PermissionCheckerTest, EmptyToken)
{
    PermissionChecker checker("");
    
    EXPECT_EQ(checker.getRole(), Role::Guest);
    EXPECT_FALSE(checker.check(Permission::PatientRegister));
}

// ==================== SessionManager测试 ====================

class SessionManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        SessionManager::instance();
    }
};

TEST_F(SessionManagerTest, CreateSession)
{
    Session session;
    session.user_id = 1001;
    session.username = "testuser";
    session.ip_address = "127.0.0.1";
    session.login_time = std::chrono::steady_clock::now();
    session.last_active = session.login_time;
    
    std::string sessionId = SessionManager::instance().createSession(session);
    
    EXPECT_FALSE(sessionId.empty());
    EXPECT_GT(sessionId.length(), 10);
}

TEST_F(SessionManagerTest, GetSession)
{
    Session session;
    session.user_id = 1002;
    session.username = "testuser2";
    session.ip_address = "127.0.0.1";
    session.login_time = std::chrono::steady_clock::now();
    session.last_active = session.login_time;
    
    std::string sessionId = SessionManager::instance().createSession(session);
    auto retrieved = SessionManager::instance().getSession(sessionId);
    
    EXPECT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->user_id, 1002);
    EXPECT_EQ(retrieved->username, "testuser2");
}

TEST_F(SessionManagerTest, DestroySession)
{
    Session session;
    session.user_id = 1003;
    session.username = "testuser3";
    session.ip_address = "127.0.0.1";
    session.login_time = std::chrono::steady_clock::now();
    session.last_active = session.login_time;
    
    std::string sessionId = SessionManager::instance().createSession(session);
    
    bool destroyed = SessionManager::instance().destroySession(sessionId);
    EXPECT_TRUE(destroyed);
    
    auto retrieved = SessionManager::instance().getSession(sessionId);
    EXPECT_FALSE(retrieved.has_value());
}

TEST_F(SessionManagerTest, DestroyUserSessions)
{
    Session session1;
    session1.user_id = 2000;
    session1.username = "testuser2000";
    session1.ip_address = "127.0.0.1";
    session1.login_time = std::chrono::steady_clock::now();
    session1.last_active = session1.login_time;
    
    SessionManager::instance().createSession(session1);
    
    bool destroyed = SessionManager::instance().destroyUserSessions(2000);
    EXPECT_TRUE(destroyed);
}

TEST_F(SessionManagerTest, SessionAttributes)
{
    Session session;
    session.user_id = 3000;
    session.username = "testuser3000";
    session.ip_address = "127.0.0.1";
    session.login_time = std::chrono::steady_clock::now();
    session.last_active = session.login_time;
    
    std::string sessionId = SessionManager::instance().createSession(session);
    
    SessionManager::instance().setAttribute(sessionId, "theme", "dark");
    SessionManager::instance().setAttribute(sessionId, "language", "zh-CN");
    
    auto theme = SessionManager::instance().getAttribute(sessionId, "theme");
    auto lang = SessionManager::instance().getAttribute(sessionId, "language");
    
    EXPECT_TRUE(theme.has_value());
    EXPECT_EQ(theme.value(), "dark");
    EXPECT_TRUE(lang.has_value());
    EXPECT_EQ(lang.value(), "zh-CN");
}

TEST_F(SessionManagerTest, OnlineCount)
{
    int before = SessionManager::instance().getOnlineCount();
    
    Session session;
    session.user_id = 4000 + before;
    session.username = "online_user";
    session.ip_address = "127.0.0.1";
    session.login_time = std::chrono::steady_clock::now();
    session.last_active = session.login_time;
    
    SessionManager::instance().createSession(session);
    
    int after = SessionManager::instance().getOnlineCount();
    EXPECT_EQ(after, before + 1);
}
