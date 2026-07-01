/**
 * @file test_protocol.cpp
 * @brief 协议层单元测试
 */
#include <gtest/gtest.h>
#include "smartsched/protocol/protocol.h"
#include "smartsched/protocol/commands.h"
#include "smartsched/protocol/message.h"

using namespace smartsched;

// ==================== 命令测试 ====================

class ProtocolTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(ProtocolTest, CommandEnumValues)
{
    // 验证命令码在有效范围内
    EXPECT_GT(CMD_PATIENT_REGISTER, 0);
    EXPECT_LT(CMD_PATIENT_REGISTER, 1000);
    
    EXPECT_GT(CMD_GET_QUEUE_STATUS, 0);
    EXPECT_LT(CMD_GET_QUEUE_STATUS, 1000);
}

TEST_F(ProtocolTest, CommandCategories)
{
    // 患者相关命令
    std::vector<int> patientCmds = {
        CMD_PATIENT_REGISTER,
        CMD_GET_QUEUE_STATUS,
        CMD_CANCEL_REGISTRATION,
        CMD_GET_MY_REGISTRATION
    };
    
    for (auto cmd : patientCmds) {
        EXPECT_GT(cmd, 0);
        EXPECT_LT(cmd, 100);
    }
    
    // 医生相关命令
    std::vector<int> doctorCmds = {
        CMD_DOCTOR_LOGIN,
        CMD_DOCTOR_CALL,
        CMD_DOCTOR_FINISH,
        CMD_GET_DOCTOR_QUEUE
    };
    
    for (auto cmd : doctorCmds) {
        EXPECT_GE(cmd, 100);
        EXPECT_LT(cmd, 200);
    }
}

TEST_F(ProtocolTest, ResultCodeMeanings)
{
    // 验证结果码定义
    EXPECT_EQ(RET_SUCCESS, 0);
    EXPECT_EQ(RET_INVALID_PARAMS, -1);
    EXPECT_EQ(RET_NOT_FOUND, -2);
    EXPECT_EQ(RET_SERVER_ERROR, -99);
}

// ==================== 消息构建测试 ====================

class MessageTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(MessageTest, BuildRequest)
{
    Message req;
    req.setCmd(CMD_PATIENT_REGISTER);
    req.setSeq(12345);
    req.setParam("patient_id", "1001");
    req.setParam("department_id", "1");
    req.setParam("patient_name", "张三");
    
    std::string json = req.toJson();
    
    EXPECT_TRUE(json.find("\"cmd\":") != std::string::npos);
    EXPECT_TRUE(json.find("\"seq\":") != std::string::npos);
    EXPECT_TRUE(json.find("1001") != std::string::npos);
}

TEST_F(MessageTest, BuildResponse)
{
    Message resp;
    resp.setCmd(CMD_PATIENT_REGISTER);
    resp.setSeq(12345);
    resp.setRet(RET_SUCCESS);
    resp.setData("registration_id", "R12345");
    resp.setData("queue_number", "15");
    
    std::string json = resp.toJson();
    
    EXPECT_TRUE(json.find("\"ret\":0") != std::string::npos);
    EXPECT_TRUE(json.find("R12345") != std::string::npos);
}

TEST_F(MessageTest, ParseMessage)
{
    std::string json = R"({
        "cmd": 1,
        "seq": 12345,
        "patient_id": "1001",
        "department_id": "1"
    })";
    
    Message msg;
    bool ok = msg.fromJson(json);
    
    EXPECT_TRUE(ok);
    EXPECT_EQ(msg.cmd(), CMD_PATIENT_REGISTER);
    EXPECT_EQ(msg.seq(), 12345);
    EXPECT_EQ(msg.getParam("patient_id"), "1001");
    EXPECT_EQ(msg.getParam("department_id"), "1");
}

TEST_F(MessageTest, MessageWithNoParams)
{
    Message msg;
    msg.setCmd(CMD_GET_QUEUE_STATUS);
    msg.setSeq(1);
    
    std::string json = msg.toJson();
    Message parsed;
    
    bool ok = parsed.fromJson(json);
    EXPECT_TRUE(ok);
    EXPECT_EQ(parsed.cmd(), CMD_GET_QUEUE_STATUS);
}

TEST_F(MessageTest, InvalidJson)
{
    Message msg;
    bool ok = msg.fromJson("invalid json");
    
    EXPECT_FALSE(ok);
}

// ==================== 协议完整性测试 ====================

class ProtocolIntegrityTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(ProtocolIntegrityTest, AllCommandsDefined)
{
    // 确保所有主要命令都有定义
    std::vector<std::pair<std::string, int>> commands = {
        {"注册", CMD_PATIENT_REGISTER},
        {"取消", CMD_CANCEL_REGISTRATION},
        {"医生登录", CMD_DOCTOR_LOGIN},
        {"医生叫号", CMD_DOCTOR_CALL},
        {"获取队列", CMD_GET_QUEUE_STATUS},
        {"获取排班", CMD_GET_SCHEDULE},
        {"管理登录", CMD_ADMIN_LOGIN}
    };
    
    for (const auto& [name, cmd] : commands) {
        EXPECT_GT(cmd, 0) << "命令 " << name << " 未定义";
    }
}

TEST_F(ProtocolIntegrityTest, ResultCodeRange)
{
    // 确保结果码在有效范围内
    EXPECT_EQ(RET_SUCCESS, 0);
    EXPECT_LT(RET_SUCCESS, 0);  // 错误码应为负数
    EXPECT_GT(RET_INVALID_PARAMS, -1000);
}
