/**
 * @file stresstest.h
 * @brief 压力测试工具 - 模拟大量并发连接和请求
 */
#ifndef SMARTSCHED_STRESSTEST_H
#define SMARTSCHED_STRESSTEST_H

#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <random>

namespace smartsched {

/**
 * @brief 测试结果统计
 */
struct TestResult {
    std::atomic<uint64_t> total_requests{0};
    std::atomic<uint64_t> successful_requests{0};
    std::atomic<uint64_t> failed_requests{0};
    std::atomic<uint64_t> total_latency_ms{0};  // 总延迟（毫秒）
    std::atomic<uint64_t> min_latency_ms{UINT64_MAX};
    std::atomic<uint64_t> max_latency_ms{0};
    
    // 时间戳
    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::time_point end_time;
    
    double getAvgLatency() const {
        uint64_t total = successful_requests.load();
        return total > 0 ? (double)total_latency_ms.load() / total : 0;
    }
    
    double getQps() const {
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(
            end_time - start_time).count();
        return duration > 0 ? (double)successful_requests.load() / duration : 0;
    }
    
    double getSuccessRate() const {
        uint64_t total = total_requests.load();
        return total > 0 ? (double)successful_requests.load() / total * 100 : 0;
    }
};

/**
 * @brief 压力测试配置
 */
struct StressTestConfig {
    std::string server_host = "localhost";
    int server_port = 8888;
    bool use_encryption = true;
    
    int num_connections = 100;          // 并发连接数
    int requests_per_connection = 100;  // 每个连接的请求数
    int think_time_ms = 100;            // 请求间隔（毫秒）
    
    // 速率控制
    bool rate_limited = false;
    int target_qps = 1000;               // 目标QPS
    
    // 测试场景
    enum class Scenario {
        Registration,   // 挂号
        Query,          // 查询
        Mixed,          // 混合
        PeakLoad        // 峰值负载
    };
    Scenario scenario = Scenario::Mixed;
    
    // 超时配置
    int connect_timeout_ms = 5000;
    int request_timeout_ms = 10000;
};

/**
 * @brief 单个模拟客户端
 */
class SimulatedClient {
public:
    SimulatedClient(int id, const StressTestConfig& config);
    ~SimulatedClient();
    
    void start();
    void stop();
    void join();
    
    uint64_t getRequestCount() const { return m_requestCount; }
    uint64_t getSuccessCount() const { return m_successCount; }
    uint64_t getFailCount() const { return m_failCount; }
    uint64_t getTotalLatency() const { return m_totalLatency; }
    uint64_t getMinLatency() const { return m_minLatency; }
    uint64_t getMaxLatency() const { return m_maxLatency; }

private:
    void run();
    bool connect();
    void disconnect();
    bool sendRegistration();
    bool sendQuery();
    void think();

private:
    int m_id;
    StressTestConfig m_config;
    bool m_running;
    std::thread m_thread;
    int m_socket;
    
    // 统计
    std::atomic<uint64_t> m_requestCount{0};
    std::atomic<uint64_t> m_successCount{0};
    std::atomic<uint64_t> m_failCount{0};
    std::atomic<uint64_t> m_totalLatency{0};
    std::atomic<uint64_t> m_minLatency{UINT64_MAX};
    std::atomic<uint64_t> m_maxLatency{0};
    
    // 随机数生成
    std::mt19937 m_rng;
    std::uniform_int_distribution<int> m_deptDist;
    std::uniform_int_distribution<int> m_waitDist;
};

/**
 * @brief 压力测试管理器
 */
class StressTest {
public:
    StressTest();
    ~StressTest();
    
    void configure(const StressTestConfig& config);
    bool run();
    void stop();
    
    TestResult getResult() const;
    void printReport() const;
    bool saveReport(const std::string& filepath) const;

private:
    void workerThread(int id);
    void printProgress();

private:
    StressTestConfig m_config;
    std::vector<SimulatedClient*> m_clients;
    std::vector<std::thread> m_threads;
    TestResult m_result;
    std::atomic<bool> m_running;
    std::atomic<bool> m_stopRequested;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::chrono::steady_clock::time_point m_lastProgress;
};

} // namespace smartsched

#endif // SMARTSCHED_STRESSTEST_H
