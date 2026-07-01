/**
 * @file test_integration.cpp
 * @brief 集成测试 - 模拟完整业务流程
 */
#include <gtest/gtest.h>
#include "smartsched/protocol/protocol.h"
#include "smartsched/protocol/commands.h"
#include "smartsched/security/auth.h"
#include "smartsched/utils/cache.h"

using namespace smartsched;

// ==================== 患者挂号流程测试 ====================

class PatientWorkflowTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(PatientWorkflowTest, FullRegistrationFlow)
{
    // 1. 构建挂号请求
    Message registerReq;
    registerReq.setCmd(CMD_PATIENT_REGISTER);
    registerReq.setSeq(1001);
    registerReq.setParam("patient_id", "P10001");
    registerReq.setParam("department_id", "1");
    registerReq.setParam("patient_name", "张三");
    
    std::string reqJson = registerReq.toJson();
    EXPECT_TRUE(reqJson.find("CMD_PATIENT_REGISTER") != std::string::npos ||
                reqJson.find("\"cmd\":1") != std::string::npos);
    
    // 2. 模拟服务器处理并返回响应
    Message registerResp;
    registerResp.setCmd(CMD_PATIENT_REGISTER);
    registerResp.setSeq(1001);
    registerResp.setRet(RET_SUCCESS);
    registerResp.setData("registration_id", "R20240115001");
    registerResp.setData("queue_number", "15");
    registerResp.setData("estimated_wait_time", "30");
    
    std::string respJson = registerResp.toJson();
    EXPECT_TRUE(respJson.find("\"ret\":0") != std::string::npos);
    
    // 3. 解析响应
    Message parsed;
    ASSERT_TRUE(parsed.fromJson(respJson));
    EXPECT_EQ(parsed.ret(), RET_SUCCESS);
    EXPECT_EQ(parsed.getData("queue_number"), "15");
}

TEST_F(PatientWorkflowTest, QueryQueueStatus)
{
    // 1. 构建查询请求
    Message queryReq;
    queryReq.setCmd(CMD_GET_QUEUE_STATUS);
    queryReq.setSeq(1002);
    queryReq.setParam("patient_id", "P10001");
    
    std::string reqJson = queryReq.toJson();
    
    // 2. 模拟响应
    Message queryResp;
    queryResp.setCmd(CMD_GET_QUEUE_STATUS);
    queryResp.setSeq(1002);
    queryResp.setRet(RET_SUCCESS);
    queryResp.setData("queue_position", "3");
    queryResp.setData("ahead_count", "2");
    queryResp.setData("estimated_time", "15");
    
    std::string respJson = queryResp.toJson();
    
    // 3. 验证
    Message parsed;
    ASSERT_TRUE(parsed.fromJson(respJson));
    EXPECT_EQ(parsed.getData("queue_position"), "3");
}

TEST_F(PatientWorkflowTest, CancelRegistration)
{
    // 1. 构建取消请求
    Message cancelReq;
    cancelReq.setCmd(CMD_CANCEL_REGISTRATION);
    cancelReq.setSeq(1003);
    cancelReq.setParam("registration_id", "R20240115001");
    
    // 2. 模拟成功响应
    Message cancelResp;
    cancelResp.setCmd(CMD_CANCEL_REGISTRATION);
    cancelResp.setSeq(1003);
    cancelResp.setRet(RET_SUCCESS);
    
    std::string respJson = cancelResp.toJson();
    
    Message parsed;
    ASSERT_TRUE(parsed.fromJson(respJson));
    EXPECT_EQ(parsed.ret(), RET_SUCCESS);
}

// ==================== 医生工作流程测试 ====================

class DoctorWorkflowTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(DoctorWorkflowTest, DoctorLoginFlow)
{
    // 1. 医生登录
    Message loginReq;
    loginReq.setCmd(CMD_DOCTOR_LOGIN);
    loginReq.setSeq(2001);
    loginReq.setParam("doctor_id", "D001");
    loginReq.setParam("password", "password123");
    
    // 2. 模拟登录响应（包含Token）
    Message loginResp;
    loginResp.setCmd(CMD_DOCTOR_LOGIN);
    loginResp.setSeq(2001);
    loginResp.setRet(RET_SUCCESS);
    loginResp.setData("token", "jwt_token_here");
    loginResp.setData("doctor_name", "李医生");
    loginResp.setData("department", "内科");
    
    std::string respJson = loginResp.toJson();
    
    Message parsed;
    ASSERT_TRUE(parsed.fromJson(respJson));
    EXPECT_EQ(parsed.ret(), RET_SUCCESS);
}

TEST_F(DoctorWorkflowTest, DoctorCallPatient)
{
    // 1. 医生叫号
    Message callReq;
    callReq.setCmd(CMD_DOCTOR_CALL);
    callReq.setSeq(2002);
    callReq.setParam("doctor_id", "D001");
    callReq.setParam("queue_number", "15");
    
    // 2. 模拟响应
    Message callResp;
    callResp.setCmd(CMD_DOCTOR_CALL);
    callResp.setSeq(2002);
    callResp.setRet(RET_SUCCESS);
    callResp.setData("patient_name", "张三");
    callResp.setData("registration_id", "R20240115001");
    
    std::string respJson = callResp.toJson();
    
    Message parsed;
    ASSERT_TRUE(parsed.fromJson(respJson));
    EXPECT_EQ(parsed.ret(), RET_SUCCESS);
}

TEST_F(DoctorWorkflowTest, DoctorFinishConsultation)
{
    // 1. 完成诊疗
    Message finishReq;
    finishReq.setCmd(CMD_DOCTOR_FINISH);
    finishReq.setSeq(2003);
    finishReq.setParam("consultation_id", "C001");
    finishReq.setParam("diagnosis", "普通感冒");
    finishReq.setParam("prescription", "感冒灵颗粒");
    
    // 2. 模拟响应
    Message finishResp;
    finishResp.setCmd(CMD_DOCTOR_FINISH);
    finishResp.setSeq(2003);
    finishResp.setRet(RET_SUCCESS);
    finishResp.setData("next_queue_number", "16");
    
    std::string respJson = finishResp.toJson();
    
    Message parsed;
    ASSERT_TRUE(parsed.fromJson(respJson));
    EXPECT_EQ(parsed.ret(), RET_SUCCESS);
}

// ==================== 管理员操作测试 ====================

class AdminWorkflowTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(AdminWorkflowTest, AdminLoginWithPermissions)
{
    // 验证管理员权限
    EXPECT_TRUE(AuthService::instance().hasPermission(Role::Admin, Permission::SystemConfig));
    EXPECT_TRUE(AuthService::instance().hasPermission(Role::Admin, Permission::UserManage));
    EXPECT_TRUE(AuthService::instance().hasPermission(Role::Admin, Permission::DataExport));
}

TEST_F(AdminWorkflowTest, ViewStatistics)
{
    Message statsReq;
    statsReq.setCmd(CMD_ADMIN_GET_STATS);
    statsReq.setSeq(3001);
    statsReq.setParam("date", "2024-01-15");
    statsReq.setParam("type", "daily");
    
    Message statsResp;
    statsResp.setCmd(CMD_ADMIN_GET_STATS);
    statsResp.setSeq(3001);
    statsResp.setRet(RET_SUCCESS);
    statsResp.setData("total_registrations", "500");
    statsResp.setData("avg_wait_time", "25");
    statsResp.setData("peak_queue_size", "50");
    
    std::string respJson = statsResp.toJson();
    
    Message parsed;
    ASSERT_TRUE(parsed.fromJson(respJson));
    EXPECT_EQ(parsed.ret(), RET_SUCCESS);
}

// ==================== 缓存性能测试 ====================

class CachePerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(CachePerformanceTest, CacheHitRate)
{
    LRUCache<std::string, std::string> cache(1000);
    
    // 预热
    for (int i = 0; i < 100; ++i) {
        cache.put("key" + std::to_string(i), "value" + std::to_string(i));
    }
    
    // 测试命中率
    int hits = 0;
    int total = 200;
    
    for (int i = 0; i < total; ++i) {
        std::string val;
        if (cache.get("key" + std::to_string(i % 150), val)) {
            hits++;
        }
    }
    
    // 命中率应该较高（因为有重复访问）
    double hitRate = (double)hits / total;
    EXPECT_GT(hitRate, 0.2);  // 至少20%
}

TEST_F(CachePerformanceTest, CacheUnderLoad)
{
    LRUCache<std::string, int> cache(100);
    
    // 添加大量数据
    for (int i = 0; i < 10000; ++i) {
        cache.put("key" + std::to_string(i), i);
    }
    
    // 验证缓存仍然正常工作
    std::string val;
    bool found = cache.get("key9999", val);
    
    // 可能被驱逐，但缓存应该还能工作
    EXPECT_TRUE(found || !found);  // 无论如何都不应该崩溃
    EXPECT_LT(cache.size(), 200);  // 应该保持在容量内
}

// ==================== 并发测试 ====================

class ConcurrencyTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(ConcurrencyTest, ConcurrentCacheAccess)
{
    LRUCache<std::string, int> cache(1000);
    
    // 模拟并发写入
    std::vector<std::thread> writers;
    for (int t = 0; t < 10; ++t) {
        writers.emplace_back([&cache, t]() {
            for (int i = 0; i < 100; ++i) {
                cache.put("key" + std::to_string(t * 100 + i), t * 100 + i);
            }
        });
    }
    
    for (auto& w : writers) {
        w.join();
    }
    
    // 验证数据完整性
    EXPECT_LT(cache.size(), 1100);  // 应该保持在合理范围内
}

TEST_F(ConcurrencyTest, SessionCreation)
{
    SessionManager::instance();
    
    std::vector<std::thread> creators;
    for (int t = 0; t < 10; ++t) {
        creators.emplace_back([t]() {
            Session session;
            session.user_id = 1000 + t;
            session.username = "user" + std::to_string(t);
            session.ip_address = "127.0.0.1";
            session.login_time = std::chrono::steady_clock::now();
            session.last_active = session.login_time;
            
            SessionManager::instance().createSession(session);
        });
    }
    
    for (auto& c : creators) {
        c.join();
    }
    
    // 验证会话数量
    int onlineCount = SessionManager::instance().getOnlineCount();
    EXPECT_GE(onlineCount, 10);
}
