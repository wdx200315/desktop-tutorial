/**
 * @file cache.h
 * @brief LRU缓存与内存池管理
 */
#ifndef SMARTSCHED_CACHE_H
#define SMARTSCHED_CACHE_H

#include <string>
#include <map>
#include <list>
#include <memory>
#include <mutex>
#include <functional>
#include <chrono>

namespace smartsched {

/**
 * @brief LRU缓存条目
 */
template<typename K, typename V>
struct CacheEntry {
    K key;
    V value;
    int64_t expire_time;  // 过期时间戳，-1表示永不过期
    int access_count;
    
    CacheEntry() : expire_time(-1), access_count(0) {}
    CacheEntry(const K& k, const V& v, int64_t exp = -1) 
        : key(k), value(v), expire_time(exp), access_count(0) {}
};

/**
 * @brief LRU缓存 - 线程安全
 */
template<typename K, typename V>
class LRUCache {
public:
    using Entry = CacheEntry<K, V>;
    using Iterator = typename std::list<Entry>::iterator;
    
    /**
     * @brief 构造函数
     * @param capacity 最大容量
     * @param defaultTTL 默认过期时间(秒)，-1表示永不过期
     */
    LRUCache(size_t capacity, int64_t defaultTTL = 3600)
        : m_capacity(capacity), m_defaultTTL(defaultTTL) {}
    
    // 禁用拷贝
    LRUCache(const LRUCache&) = delete;
    LRUCache& operator=(const LRUCache&) = delete;
    
    // 基本操作
    void put(const K& key, const V& value, int64_t ttl = -1);
    bool get(const K& key, V& value);
    bool contains(const K& key);
    bool remove(const K& key);
    void clear();
    
    // 批量操作
    void putAll(const std::map<K, V>& items, int64_t ttl = -1);
    std::map<K, V> getAll(const std::vector<K>& keys);
    
    // 统计
    size_t size() const;
    size_t capacity() const { return m_capacity; }
    void stats(size_t& total, size_t& expired, size_t& memory) const;
    
    // 过期清理
    void cleanup();
    size_t cleanupExpired();
    
    // LFU特性
    V* getPtr(const K& key);  // 获取指针，不调整顺序
    
private:
    void moveToFront(Iterator it);
    void evictOne();

private:
    size_t m_capacity;
    int64_t m_defaultTTL;
    
    std::list<Entry> m_list;           // 双向链表：最近访问在front
    std::map<K, Iterator> m_map;        // 哈希表：key -> iterator
    mutable std::mutex m_mutex;
};

/**
 * @brief 缓存命中统计
 */
struct CacheStats {
    uint64_t hits = 0;
    uint64_t misses = 0;
    uint64_t evictions = 0;
    uint64_t expirations = 0;
    
    double hitRate() const {
        uint64_t total = hits + misses;
        return total > 0 ? (double)hits / total : 0;
    }
};

/**
 * @brief 全局缓存管理器
 */
class CacheManager {
public:
    static CacheManager& instance();
    
    // 命名缓存
    LRUCache<std::string, std::string>* getStringCache(const std::string& name);
    LRUCache<std::string, std::string>* getOrCreateStringCache(
        const std::string& name, size_t capacity, int64_t ttl = 3600);
    
    // 预定义缓存
    void initCaches();
    
    // 统计
    CacheStats getStats() const;
    void printStats() const;
    
    // 全局清理
    void cleanupAll();
    
    // 注册统计回调
    using StatsCallback = std::function<void(const CacheStats&)>;
    void setStatsCallback(StatsCallback cb);

private:
    CacheManager();
    ~CacheManager();
    
    std::map<std::string, std::unique_ptr<LRUCache<std::string, std::string>>> m_caches;
    CacheStats m_stats;
    mutable std::mutex m_mutex;
    StatsCallback m_statsCallback;
};

// ==================== 内存池 ====================

/**
 * @brief 固定大小内存池
 */
class MemoryPool {
public:
    MemoryPool(size_t blockSize, size_t poolSize);
    ~MemoryPool();
    
    void* allocate();
    void deallocate(void* ptr);
    
    size_t getBlockSize() const { return m_blockSize; }
    size_t getTotalBlocks() const { return m_totalBlocks; }
    size_t getFreeBlocks() const;
    size_t getUsedBlocks() const { return m_totalBlocks - getFreeBlocks(); }
    
private:
    struct Block {
        Block* next;
        char data[0];
    };
    
    size_t m_blockSize;
    size_t m_totalBlocks;
    Block* m_freeList;
    std::vector<Block*> m_allBlocks;
    
#ifdef _WIN32
    void* m_heap;
#else
    int m_heap;
#endif
};

// ==================== 对象池 ====================

/**
 * @brief 对象池模板
 */
template<typename T>
class ObjectPool {
public:
    ObjectPool(size_t initialSize = 16, size_t maxSize = 1024)
        : m_maxSize(maxSize) {
        m_factory = []() { return new T(); };
        grow(initialSize);
    }
    
    ObjectPool(size_t initialSize, size_t maxSize, std::function<T*()> factory)
        : m_maxSize(maxSize), m_factory(factory) {
        grow(initialSize);
    }
    
    ~ObjectPool() {
        clear();
    }
    
    T* acquire() {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (!m_freeList.empty()) {
            T* obj = m_freeList.back();
            m_freeList.pop_back();
            return obj;
        }
        
        if (m_totalCount < m_maxSize) {
            grow(std::min(m_maxSize - m_totalCount, size_t(16)));
            if (!m_freeList.empty()) {
                T* obj = m_freeList.back();
                m_freeList.pop_back();
                return obj;
            }
        }
        
        // 超过上限，返回新对象
        return m_factory();
    }
    
    void release(T* obj) {
        if (!obj) return;
        
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (m_totalCount <= m_maxSize) {
            m_freeList.push_back(obj);
        } else {
            delete obj;
        }
    }
    
    template<typename F>
    void release(T* obj, F&& resetFunc) {
        if (!obj) return;
        resetFunc(obj);
        release(obj);
    }
    
    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto obj : m_freeList) {
            delete obj;
        }
        m_freeList.clear();
        m_totalCount = 0;
    }
    
    size_t size() const { return m_freeList.size(); }
    size_t totalCount() const { return m_totalCount; }

private:
    void grow(size_t count) {
        for (size_t i = 0; i < count; ++i) {
            m_freeList.push_back(m_factory());
            ++m_totalCount;
        }
    }

private:
    size_t m_maxSize;
    size_t m_totalCount = 0;
    std::vector<T*> m_freeList;
    std::function<T*()> m_factory;
    mutable std::mutex m_mutex;
};

// ==================== 环形缓冲区 ====================

/**
 * @brief 线程安全的环形缓冲区
 */
template<typename T>
class RingBuffer {
public:
    RingBuffer(size_t capacity) : m_capacity(capacity), m_buffer(capacity) {
        m_head = m_tail = m_count = 0;
    }
    
    bool push(const T& item);
    bool pop(T& item);
    bool peek(T& item) const;
    
    size_t size() const { 
        std::lock_guard<std::mutex> lock(m_mutex); 
        return m_count; 
    }
    
    size_t capacity() const { return m_capacity; }
    bool isFull() const { return size() >= m_capacity; }
    bool isEmpty() const { return size() == 0; }
    
    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_head = m_tail = m_count = 0;
    }

private:
    size_t m_capacity;
    std::vector<T> m_buffer;
    size_t m_head;    // 读位置
    size_t m_tail;    // 写位置
    size_t m_count;   // 元素数量
    mutable std::mutex m_mutex;
};

// ==================== 模板实现 ====================

template<typename K, typename V>
void LRUCache<K, V>::put(const K& key, const V& value, int64_t ttl)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    int64_t expireTime = (ttl < 0) ? -1 : (time(nullptr) + ttl);
    
    auto it = m_map.find(key);
    if (it != m_map.end()) {
        // 更新现有条目
        it->second->value = value;
        it->second->expire_time = expireTime;
        it->second->access_count++;
        moveToFront(it->second);
    } else {
        // 新条目
        if (m_list.size() >= m_capacity) {
            evictOne();
        }
        
        m_list.push_front(Entry(key, value, expireTime));
        m_map[key] = m_list.begin();
    }
}

template<typename K, typename V>
bool LRUCache<K, V>::get(const K& key, V& value)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_map.find(key);
    if (it == m_map.end()) {
        return false;
    }
    
    Entry& entry = *it->second;
    
    // 检查过期
    if (entry.expire_time > 0 && time(nullptr) > entry.expire_time) {
        m_list.erase(it->second);
        m_map.erase(it);
        return false;
    }
    
    value = entry.value;
    entry.access_count++;
    moveToFront(it->second);
    
    return true;
}

template<typename K, typename V>
bool LRUCache<K, V>::contains(const K& key)
{
    V value;
    return get(key, value);
}

template<typename K, typename V>
bool LRUCache<K, V>::remove(const K& key)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_map.find(key);
    if (it != m_map.end()) {
        m_list.erase(it->second);
        m_map.erase(it);
        return true;
    }
    return false;
}

template<typename K, typename V>
void LRUCache<K, V>::clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_list.clear();
    m_map.clear();
}

template<typename K, typename V>
void LRUCache<K, V>::putAll(const std::map<K, V>& items, int64_t ttl)
{
    for (const auto& [k, v] : items) {
        put(k, v, ttl);
    }
}

template<typename K, typename V>
std::map<K, V> LRUCache<K, V>::getAll(const std::vector<K>& keys)
{
    std::map<K, V> result;
    for (const auto& k : keys) {
        V v;
        if (get(k, v)) {
            result[k] = v;
        }
    }
    return result;
}

template<typename K, typename V>
size_t LRUCache<K, V>::size() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_map.size();
}

template<typename K, typename V>
void LRUCache<K, V>::stats(size_t& total, size_t& expired, size_t& memory) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    total = m_map.size();
    expired = 0;
    memory = 0;
    int64_t now = time(nullptr);
    
    for (const auto& entry : m_list) {
        if (entry.expire_time > 0 && now > entry.expire_time) {
            expired++;
        }
        memory += sizeof(K) + sizeof(V) + sizeof(int64_t);
    }
}

template<typename K, typename V>
void LRUCache<K, V>::cleanup()
{
    cleanupExpired();
}

template<typename K, typename V>
size_t LRUCache<K, V>::cleanupExpired()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    int64_t now = time(nullptr);
    size_t removed = 0;
    
    auto it = m_list.begin();
    while (it != m_list.end()) {
        if (it->expire_time > 0 && now > it->expire_time) {
            m_map.erase(it->key);
            it = m_list.erase(it);
            removed++;
        } else {
            ++it;
        }
    }
    
    return removed;
}

template<typename K, typename V>
V* LRUCache<K, V>::getPtr(const K& key)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_map.find(key);
    if (it != m_map.end()) {
        return &it->second->value;
    }
    return nullptr;
}

template<typename K, typename V>
void LRUCache<K, V>::moveToFront(Iterator it)
{
    if (it != m_list.begin()) {
        m_list.splice(m_list.begin(), m_list, it);
    }
}

template<typename K, typename V>
void LRUCache<K, V>::evictOne()
{
    if (!m_list.empty()) {
        auto last = std::prev(m_list.end());
        m_map.erase(last->key);
        m_list.pop_back();
    }
}

// RingBuffer实现
template<typename T>
bool RingBuffer<T>::push(const T& item)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_count >= m_capacity) {
        return false;
    }
    
    m_buffer[m_tail] = item;
    m_tail = (m_tail + 1) % m_capacity;
    ++m_count;
    return true;
}

template<typename T>
bool RingBuffer<T>::pop(T& item)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_count == 0) {
        return false;
    }
    
    item = m_buffer[m_head];
    m_head = (m_head + 1) % m_capacity;
    --m_count;
    return true;
}

template<typename T>
bool RingBuffer<T>::peek(T& item) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_count == 0) {
        return false;
    }
    
    item = m_buffer[m_head];
    return true;
}

} // namespace smartsched

#endif // SMARTSCHED_CACHE_H
