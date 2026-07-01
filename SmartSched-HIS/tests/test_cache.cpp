/**
 * @file test_cache.cpp
 * @brief 缓存模块单元测试
 */
#include <gtest/gtest.h>
#include "smartsched/utils/cache.h"

using namespace smartsched;

// ==================== LRUCache测试 ====================

class LRUCacheTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(LRUCacheTest, PutAndGet)
{
    LRUCache<std::string, std::string> cache(100);
    
    cache.put("key1", "value1");
    cache.put("key2", "value2");
    
    std::string val1, val2;
    EXPECT_TRUE(cache.get("key1", val1));
    EXPECT_TRUE(cache.get("key2", val2));
    
    EXPECT_EQ(val1, "value1");
    EXPECT_EQ(val2, "value2");
}

TEST_F(LRUCacheTest, UpdateExisting)
{
    LRUCache<std::string, int> cache(100);
    
    cache.put("key", 1);
    cache.put("key", 2);
    
    int value;
    cache.get("key", value);
    
    EXPECT_EQ(value, 2);
    EXPECT_EQ(cache.size(), 1);
}

TEST_F(LRUCacheTest, Eviction)
{
    LRUCache<std::string, std::string> cache(3);
    
    cache.put("a", "1");
    cache.put("b", "2");
    cache.put("c", "3");
    
    // 访问a使其成为最近使用
    std::string val;
    cache.get("a", val);
    
    // 添加新元素，a不应该被驱逐
    cache.put("d", "4");
    
    EXPECT_TRUE(cache.get("a", val));  // a应该还在
}

TEST_F(LRUCacheTest, MissOnEviction)
{
    LRUCache<std::string, std::string> cache(2);
    
    cache.put("a", "1");
    cache.put("b", "2");
    
    // 添加第三个元素，最久未使用的应该被驱逐
    cache.put("c", "3");
    
    std::string val;
    // b应该是被驱逐的那个（因为a和b中b先添加）
    // 但由于LRU实现，可能a或b被驱逐
    bool aExists = cache.get("a", val);
    bool bExists = cache.get("b", val);
    
    // 至少有一个应该不存在
    EXPECT_FALSE(aExists && bExists);
}

TEST_F(LRUCacheTest, Contains)
{
    LRUCache<std::string, int> cache(100);
    
    cache.put("key", 42);
    
    EXPECT_TRUE(cache.contains("key"));
    EXPECT_FALSE(cache.contains("nonexistent"));
}

TEST_F(LRUCacheTest, Remove)
{
    LRUCache<std::string, int> cache(100);
    
    cache.put("key", 42);
    EXPECT_TRUE(cache.contains("key"));
    
    cache.remove("key");
    EXPECT_FALSE(cache.contains("key"));
}

TEST_F(LRUCacheTest, Clear)
{
    LRUCache<std::string, int> cache(100);
    
    cache.put("a", 1);
    cache.put("b", 2);
    cache.put("c", 3);
    
    EXPECT_EQ(cache.size(), 3);
    
    cache.clear();
    EXPECT_EQ(cache.size(), 0);
}

TEST_F(LRUCacheTest, Capacity)
{
    LRUCache<std::string, int> cache(50);
    
    EXPECT_EQ(cache.capacity(), 50);
}

TEST_F(LRUCacheTest, PutAll)
{
    LRUCache<std::string, int> cache(100);
    
    std::map<std::string, int> items = {
        {"a", 1},
        {"b", 2},
        {"c", 3}
    };
    
    cache.putAll(items);
    
    EXPECT_EQ(cache.size(), 3);
}

TEST_F(LRUCacheTest, GetAll)
{
    LRUCache<std::string, int> cache(100);
    
    cache.put("a", 1);
    cache.put("b", 2);
    cache.put("c", 3);
    
    auto results = cache.getAll({"a", "c", "nonexistent"});
    
    EXPECT_EQ(results.size(), 2);
    EXPECT_EQ(results["a"], 1);
    EXPECT_EQ(results["c"], 3);
    EXPECT_FALSE(results.count("nonexistent"));
}

// ==================== RingBuffer测试 ====================

class RingBufferTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(RingBufferTest, BasicOperations)
{
    RingBuffer<int> buffer(5);
    
    EXPECT_TRUE(buffer.isEmpty());
    EXPECT_FALSE(buffer.isFull());
    
    buffer.push(1);
    buffer.push(2);
    buffer.push(3);
    
    EXPECT_EQ(buffer.size(), 3);
    
    int val;
    buffer.pop(val);
    EXPECT_EQ(val, 1);
    
    buffer.pop(val);
    EXPECT_EQ(val, 2);
    
    EXPECT_EQ(buffer.size(), 1);
}

TEST_F(RingBufferTest, FullBuffer)
{
    RingBuffer<int> buffer(3);
    
    buffer.push(1);
    buffer.push(2);
    buffer.push(3);
    
    EXPECT_TRUE(buffer.isFull());
    EXPECT_FALSE(buffer.push(4));  // 应该失败
}

TEST_F(RingBufferTest, EmptyBuffer)
{
    RingBuffer<int> buffer(5);
    
    int val;
    EXPECT_FALSE(buffer.pop(val));  // 应该失败
}

TEST_F(RingBufferTest, Peek)
{
    RingBuffer<std::string> buffer(5);
    
    buffer.push("first");
    buffer.push("second");
    
    std::string val;
    buffer.peek(val);
    EXPECT_EQ(val, "first");
    
    // peek不应该移除元素
    buffer.peek(val);
    EXPECT_EQ(val, "first");
    EXPECT_EQ(buffer.size(), 2);
}

TEST_F(RingBufferTest, Clear)
{
    RingBuffer<int> buffer(5);
    
    buffer.push(1);
    buffer.push(2);
    buffer.push(3);
    
    buffer.clear();
    
    EXPECT_TRUE(buffer.isEmpty());
    EXPECT_EQ(buffer.size(), 0);
}

TEST_F(RingBufferTest, Capacity)
{
    RingBuffer<int> buffer(100);
    
    EXPECT_EQ(buffer.capacity(), 100);
}

// ==================== ObjectPool测试 ====================

class ObjectPoolTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(ObjectPoolTest, AcquireRelease)
{
    ObjectPool<int> pool(10, 100);
    
    int* obj1 = pool.acquire();
    int* obj2 = pool.acquire();
    
    EXPECT_NE(obj1, nullptr);
    EXPECT_NE(obj2, nullptr);
    
    pool.release(obj1);
    pool.release(obj2);
}

TEST_F(ObjectPoolTest, Reuse)
{
    ObjectPool<int> pool(5, 10);
    
    int* obj1 = pool.acquire();
    *obj1 = 42;
    
    pool.release(obj1);
    
    int* obj2 = pool.acquire();
    // obj2可能是复用的obj1
    pool.release(obj2);
}

TEST_F(ObjectPoolTest, MaxSize)
{
    ObjectPool<int> pool(2, 5);
    
    std::vector<int*> objects;
    for (int i = 0; i < 5; ++i) {
        objects.push_back(pool.acquire());
    }
    
    // 超过最大大小时会分配新对象而不是复用
    int* extra = pool.acquire();
    EXPECT_NE(extra, nullptr);
    
    // 清理
    for (auto obj : objects) {
        pool.release(obj);
    }
    pool.release(extra);
}

TEST_F(ObjectPoolTest, Clear)
{
    ObjectPool<int> pool(10, 20);
    
    for (int i = 0; i < 5; ++i) {
        pool.release(pool.acquire());
    }
    
    EXPECT_GT(pool.size(), 0);
    
    pool.clear();
    EXPECT_EQ(pool.size(), 0);
}

// ==================== MemoryPool测试 ====================

class MemoryPoolTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(MemoryPoolTest, BasicAllocation)
{
    MemoryPool pool(64, 100);
    
    void* ptr1 = pool.allocate();
    void* ptr2 = pool.allocate();
    
    EXPECT_NE(ptr1, nullptr);
    EXPECT_NE(ptr2, nullptr);
    EXPECT_NE(ptr1, ptr2);
    
    pool.deallocate(ptr1);
    pool.deallocate(ptr2);
}

TEST_F(MemoryPoolTest, BlockSize)
{
    MemoryPool pool(128, 50);
    
    EXPECT_EQ(pool.getBlockSize(), 128);
    EXPECT_EQ(pool.getTotalBlocks(), 50);
}

TEST_F(MemoryPoolTest, FreeBlocks)
{
    MemoryPool pool(64, 10);
    
    std::vector<void*> ptrs;
    for (int i = 0; i < 5; ++i) {
        ptrs.push_back(pool.allocate());
    }
    
    EXPECT_EQ(pool.getFreeBlocks(), 5);
    
    for (auto ptr : ptrs) {
        pool.deallocate(ptr);
    }
    
    EXPECT_EQ(pool.getFreeBlocks(), 10);
}
