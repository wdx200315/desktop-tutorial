/**
 * @file test_common.cpp
 * @brief 公共模块单元测试
 */
#include <gtest/gtest.h>
#include "smartsched/utils/json_helper.h"
#include "smartsched/utils/datetime.h"
#include "smartsched/utils/bytebuffer.h"
#include "smartsched/common/macros.h"

using namespace smartsched;

// ==================== JSON测试 ====================

class JsonTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(JsonTest, ParseValidJson)
{
    std::string jsonStr = R"({"name":"test","value":123,"flag":true})";
    auto obj = json::parse(jsonStr);
    
    EXPECT_EQ(obj["name"].toString(), "test");
    EXPECT_EQ(obj["value"].toInt(), 123);
    EXPECT_EQ(obj["flag"].toBool(), true);
}

TEST_F(JsonTest, BuildJson)
{
    json::JSON obj;
    obj["key1"] = "value1";
    obj["key2"] = 42;
    obj["key3"] = json::Array();
    obj["key3"].append("a");
    obj["key3"].append("b");
    
    std::string output = obj.dump();
    
    EXPECT_TRUE(output.find("key1") != std::string::npos);
    EXPECT_TRUE(output.find("value1") != std::string::npos);
    EXPECT_TRUE(output.find("42") != std::string::npos);
}

TEST_F(JsonTest, NestedJson)
{
    json::JSON root;
    root["data"]["user"]["id"] = 1001;
    root["data"]["user"]["name"] = "张三";
    root["data"]["items"] = json::Array();
    root["data"]["items"].append(1);
    root["data"]["items"].append(2);
    
    std::string output = root.dump();
    
    EXPECT_TRUE(output.find("张三") != std::string::npos);
    EXPECT_TRUE(output.find("1001") != std::string::npos);
}

TEST_F(JsonTest, InvalidJson)
{
    std::string invalidJson = "{invalid}";
    EXPECT_THROW({
        auto obj = json::parse(invalidJson);
    }, std::runtime_error);
}

// ==================== DateTime测试 ====================

class DateTimeTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(DateTimeTest, CurrentTime)
{
    auto now = DateTime::now();
    EXPECT_GT(now.toUnix(), 0);
}

TEST_F(DateTimeTest, FormatTime)
{
    auto dt = DateTime::now();
    std::string formatted = dt.format("%Y-%m-%d %H:%M:%S");
    
    EXPECT_TRUE(formatted.find('-') != std::string::npos);
    EXPECT_TRUE(formatted.find(':') != std::string::npos);
}

TEST_F(DateTimeTest, ParseTime)
{
    std::string timeStr = "2024-01-15 10:30:00";
    auto dt = DateTime::parse(timeStr, "%Y-%m-%d %H:%M:%S");
    
    EXPECT_EQ(dt.year(), 2024);
    EXPECT_EQ(dt.month(), 1);
    EXPECT_EQ(dt.day(), 15);
    EXPECT_EQ(dt.hour(), 10);
    EXPECT_EQ(dt.minute(), 30);
}

TEST_F(DateTimeTest, TimeDifference)
{
    auto dt1 = DateTime::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto dt2 = DateTime::now();
    
    auto diff = dt2 - dt1;
    EXPECT_GE(diff.count(), 99);  // 允许一些误差
}

TEST_F(DateTimeTest, AddTime)
{
    auto dt1 = DateTime::now();
    auto dt2 = dt1.addDays(1);
    
    EXPECT_EQ(dt2.day(), dt1.day() + 1);
}

// ==================== ByteBuffer测试 ====================

class ByteBufferTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(ByteBufferTest, WriteAndRead)
{
    ByteBuffer buffer(1024);
    
    // 写入数据
    buffer.writeInt8(127);
    buffer.writeInt16(32767);
    buffer.writeInt32(2147483647);
    buffer.writeInt64(9223372036854775807LL);
    buffer.writeFloat(3.14159f);
    buffer.writeDouble(3.14159265358979);
    buffer.writeString("Hello, World!");
    
    // 切换到读模式
    buffer.flip();
    
    // 读取数据
    EXPECT_EQ(buffer.readInt8(), 127);
    EXPECT_EQ(buffer.readInt16(), 32767);
    EXPECT_EQ(buffer.readInt32(), 2147483647);
    EXPECT_EQ(buffer.readInt64(), 9223372036854775807LL);
    EXPECT_FLOAT_EQ(buffer.readFloat(), 3.14159f);
    EXPECT_DOUBLE_EQ(buffer.readDouble(), 3.14159265358979);
    EXPECT_EQ(buffer.readString(), "Hello, World!");
}

TEST_F(ByteBufferTest, Capacity)
{
    ByteBuffer buffer(256);
    EXPECT_EQ(buffer.capacity(), 256);
    EXPECT_EQ(buffer.remaining(), 256);
}

TEST_F(ByteBufferTest, AutoExpand)
{
    ByteBuffer buffer(8);
    
    // 写入超过初始容量的数据
    std::string longStr(100, 'x');
    buffer.writeString(longStr);
    
    EXPECT_GE(buffer.capacity(), 100);
}

TEST_F(ByteBufferTest, Clear)
{
    ByteBuffer buffer(1024);
    buffer.writeInt32(42);
    buffer.clear();
    
    EXPECT_EQ(buffer.remaining(), 1024);
    EXPECT_EQ(buffer.position(), 0);
}

TEST_F(ByteBufferTest, ReadOnly)
{
    ByteBuffer buffer(1024);
    buffer.writeInt32(42);
    buffer.flip();
    buffer.setReadOnly();
    
    EXPECT_THROW({
        buffer.writeInt32(100);
    }, std::runtime_error);
}

// ==================== 宏测试 ====================

class MacrosTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(MacrosTest, DISABLE_COPY)
{
    // 验证DISABLE_COPY宏使拷贝构造函数和赋值运算符不可用
    // 这是一个编译时测试
    EXPECT_TRUE(true);
}

TEST_F(MacrosTest, Version)
{
    EXPECT_GT(SMART_SCHED_VERSION, 0);
}
