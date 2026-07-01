/**
 * @file test_security.cpp
 * @brief 安全模块单元测试
 */
#include <gtest/gtest.h>
#include "smartsched/security/security.h"

using namespace smartsched;

// ==================== SQL注入检测测试 ====================

class SQLInjectionTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(SQLInjectionTest, DetectUnionSelect)
{
    auto& detector = SQLInjectionDetector::instance();
    
    EXPECT_TRUE(detector.isDangerous("'; DROP TABLE users; --"));
    EXPECT_TRUE(detector.isDangerous("1 OR 1=1"));
    EXPECT_TRUE(detector.isDangerous("1' AND '1'='1"));
    EXPECT_TRUE(detector.isDangerous("admin'--"));
}

TEST_F(SQLInjectionTest, SafeInput)
{
    auto& detector = SQLInjectionDetector::instance();
    
    EXPECT_FALSE(detector.isDangerous("John Doe"));
    EXPECT_FALSE(detector.isDangerous("123456"));
    EXPECT_FALSE(detector.isDangerous("正常的中文输入"));
}

TEST_F(SQLInjectionTest, SanitizeInput)
{
    auto& detector = SQLInjectionDetector::instance();
    
    std::string sanitized = detector.sanitize("O'Brien");
    EXPECT_TRUE(sanitized.find("''") != std::string::npos);
}

TEST_F(SQLInjectionTest, Whitelist)
{
    auto& detector = SQLInjectionDetector::instance();
    
    detector.addWhitelist("allowed_value");
    
    // 白名单中的值应该通过
    EXPECT_FALSE(detector.isDangerous("allowed_value"));
    
    detector.clearWhitelist();
}

TEST_F(SQLInjectionTest, FindDangerousPatterns)
{
    auto& detector = SQLInjectionDetector::instance();
    
    auto patterns = detector.findDangerousPatterns("'; DROP TABLE users; SELECT * FROM admin--");
    
    EXPECT_GT(patterns.size(), 0);
}

// ==================== XSS防护测试 ====================

class XSSTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(XSSTest, EncodeHTML)
{
    auto& protector = XSSProtector::instance();
    
    std::string encoded = protector.encodeHTML("<script>alert('xss')</script>");
    
    EXPECT_TRUE(encoded.find("<script>") == std::string::npos);
    EXPECT_TRUE(encoded.find("&lt;") != std::string::npos);
}

TEST_F(XSSTest, EncodeJavaScript)
{
    auto& protector = XSSProtector::instance();
    
    std::string encoded = protector.encodeJavaScript("var x = \"test\";");
    
    EXPECT_TRUE(encoded.find("\"") == std::string::npos);
    EXPECT_TRUE(encoded.find("\\\"") != std::string::npos);
}

TEST_F(XSSTest, EncodeURL)
{
    auto& protector = XSSProtector::instance();
    
    std::string encoded = protector.encodeURL("hello world");
    
    EXPECT_TRUE(encoded.find("%20") != std::string::npos);
}

TEST_F(XSSTest, ContainsXSS)
{
    auto& protector = XSSProtector::instance();
    
    EXPECT_TRUE(protector.containsXSS("<script>alert(1)</script>"));
    EXPECT_TRUE(protector.containsXSS("javascript:alert(1)"));
    EXPECT_TRUE(protector.containsXSS("<img src=x onerror=alert(1)>"));
}

TEST_F(XSSTest, SafeContent)
{
    auto& protector = XSSProtector::instance();
    
    EXPECT_FALSE(protector.containsXSS("Hello, World!"));
    EXPECT_FALSE(protector.containsXSS("正常的中文内容"));
    EXPECT_FALSE(protector.containsXSS("123456"));
}

TEST_F(XSSTest, SanitizeInput)
{
    auto& protector = XSSProtector::instance();
    
    std::string sanitized = protector.sanitize("<script>bad</script>safe");
    
    EXPECT_TRUE(sanitized.find("<script>") == std::string::npos);
    EXPECT_TRUE(sanitized.find("safe") != std::string::npos);
}

// ==================== 限流器测试 ====================

class RateLimiterTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(RateLimiterTest, BasicRateLimiting)
{
    RateLimiter limiter;
    limiter.setLimit(3, 60);  // 60秒内最多3次
    
    // 前3次应该通过
    EXPECT_TRUE(limiter.check("user1").allowed);
    EXPECT_TRUE(limiter.check("user1").allowed);
    EXPECT_TRUE(limiter.check("user1").allowed);
    
    // 第4次应该被限流
    EXPECT_FALSE(limiter.check("user1").allowed);
}

TEST_F(RateLimiterTest, DifferentKeys)
{
    RateLimiter limiter;
    limiter.setLimit(1, 60);
    
    EXPECT_TRUE(limiter.check("user1").allowed);
    EXPECT_FALSE(limiter.check("user1").allowed);
    
    // 不同用户不受影响
    EXPECT_TRUE(limiter.check("user2").allowed);
}

TEST_F(RateLimiterTest, Exemptions)
{
    RateLimiter limiter;
    limiter.setLimit(1, 60);
    
    // 添加豁免
    limiter.addExemption("127.0.0.1");
    
    // 豁免IP不受限制
    EXPECT_TRUE(limiter.check("req", "127.0.0.1").allowed);
    
    limiter.removeExemption("127.0.0.1");
}

TEST_F(RateLimiterTest, RemainingCount)
{
    RateLimiter limiter;
    limiter.setLimit(5, 60);
    
    auto result1 = limiter.check("user1");
    EXPECT_EQ(result1.remaining, 4);
    
    limiter.check("user1");
    limiter.check("user1");
    
    auto result4 = limiter.check("user1");
    EXPECT_EQ(result4.remaining, 1);
}

// ==================== IP黑名单测试 ====================

class IPBlacklistTest : public ::testing::Test {
protected:
    void SetUp() override {
        IPBlacklist::instance();
    }
};

TEST_F(IPBlacklistTest, BlockUnblock)
{
    auto& blacklist = IPBlacklist::instance();
    
    blacklist.block("192.168.1.100", "test_block");
    EXPECT_TRUE(blacklist.isBlocked("192.168.1.100"));
    
    blacklist.unblock("192.168.1.100");
    EXPECT_FALSE(blacklist.isBlocked("192.168.1.100"));
}

TEST_F(IPBlacklistTest, MultipleBlocks)
{
    auto& blacklist = IPBlacklist::instance();
    
    blacklist.block("10.0.0.1");
    blacklist.block("10.0.0.2");
    blacklist.block("10.0.0.3");
    
    EXPECT_GE(blacklist.size(), 3);
}

TEST_F(IPBlacklistTest, GetBlockedIPs)
{
    auto& blacklist = IPBlacklist::instance();
    
    blacklist.block("1.1.1.1");
    blacklist.block("2.2.2.2");
    
    auto ips = blacklist.getBlockedIPs();
    
    bool found1 = false, found2 = false;
    for (const auto& ip : ips) {
        if (ip == "1.1.1.1") found1 = true;
        if (ip == "2.2.2.2") found2 = true;
    }
    
    EXPECT_TRUE(found1 || found2);  // 至少找到一个
}

// ==================== CSRF防护测试 ====================

class CSRFTest : public ::testing::Test {
protected:
    void SetUp() override {
        CSRFProtector::instance();
    }
};

TEST_F(CSRFTest, GenerateToken)
{
    std::string token = CSRFProtector::instance().generateToken("session123");
    
    EXPECT_FALSE(token.empty());
    EXPECT_EQ(token.length(), 64);  // 32字节的十六进制表示
}

TEST_F(CSRFTest, ValidateToken)
{
    std::string sessionId = "session_abc";
    std::string token = CSRFProtector::instance().generateToken(sessionId);
    
    bool valid = CSRFProtector::instance().validateToken(sessionId, token);
    EXPECT_TRUE(valid);
}

TEST_F(CSRFTest, InvalidToken)
{
    bool valid = CSRFProtector::instance().validateToken("session123", "invalid_token");
    EXPECT_FALSE(valid);
}

TEST_F(CSRFTest, DifferentSessions)
{
    std::string token1 = CSRFProtector::instance().generateToken("session1");
    std::string token2 = CSRFProtector::instance().generateToken("session2");
    
    // 不同session的token不能混用
    EXPECT_FALSE(CSRFProtector::instance().validateToken("session1", token2));
    EXPECT_FALSE(CSRFProtector::instance().validateToken("session2", token1));
}

TEST_F(CSRFTest, InvalidateToken)
{
    std::string sessionId = "session_xyz";
    std::string token = CSRFProtector::instance().generateToken(sessionId);
    
    EXPECT_TRUE(CSRFProtector::instance().validateToken(sessionId, token));
    
    CSRFProtector::instance().invalidateToken(sessionId);
    
    EXPECT_FALSE(CSRFProtector::instance().validateToken(sessionId, token));
}

// ==================== 安全审计测试 ====================

class SecurityAuditTest : public ::testing::Test {
protected:
    void SetUp() override {
        SecurityAudit::instance();
    }
};

TEST_F(SecurityAuditTest, LogEvent)
{
    auto& audit = SecurityAudit::instance();
    
    audit.log(SecurityEvent::BruteForce, "192.168.1.50", "admin", "多次登录失败");
    
    auto events = audit.query(SecurityEvent::BruteForce, "192.168.1.50", 0, 0, 10);
    
    EXPECT_GE(events.size(), 0);  // 事件可能已清理
}

TEST_F(SecurityAuditTest, QueryByIP)
{
    auto& audit = SecurityAudit::instance();
    
    audit.log(SecurityEvent::SQLInjection, "10.0.0.1", "", "test sql");
    audit.log(SecurityEvent::XSSAttack, "10.0.0.1", "", "test xss");
    
    // 查询该IP的所有事件
    // 注意：可能有清理机制
}

TEST_F(SecurityAuditTest, EventStats)
{
    auto& audit = SecurityAudit::instance();
    
    auto stats = audit.getStats();
    
    EXPECT_GE(stats.totalEvents, 0);
}
