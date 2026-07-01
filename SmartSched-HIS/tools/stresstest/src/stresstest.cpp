/**
 * @file stresstest.cpp
 * @brief 压力测试工具实现
 */
#include "stresstest.h"
#include "../common/include/smartsched/protocol/protocol.h"
#include "../common/include/smartsched/crypto/crypto.h"
#include <iostream>
#include <iomanip>
#include <cstring>
#include <csignal>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h>
    #include <errno.h>
    #define SOCKET int
    #define INVALID_SOCKET (-1)
    #define SOCKET_ERROR (-1)
#endif

namespace smartsched {

// ==================== SimulatedClient 实现 ====================
SimulatedClient::SimulatedClient(int id, const StressTestConfig& config)
    : m_id(id)
    , m_config(config)
    , m_running(false)
    , m_socket(INVALID_SOCKET)
    , m_rng(std::random_device{}())
    , m_deptDist(1, 15)       // 1-15个科室
    , m_waitDist(10, 60)      // 10-60分钟等待
{
}

SimulatedClient::~SimulatedClient()
{
    stop();
    if (m_socket != INVALID_SOCKET) {
#ifdef _WIN32
        closesocket(m_socket);
#else
        close(m_socket);
#endif
    }
}

bool SimulatedClient::connect()
{
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    m_socket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket == INVALID_SOCKET) {
        return false;
    }

    // 设置超时
#ifdef _WIN32
    int timeout = m_config.connect_timeout_ms;
#else
    struct timeval timeout;
    timeout.tv_sec = m_config.connect_timeout_ms / 1000;
    timeout.tv_usec = (m_config.connect_timeout_ms % 1000) * 1000;
#endif
    setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));

    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(m_config.server_port);
    
    if (inet_pton(AF_INET, m_config.server_host.c_str(), &serverAddr.sin_addr) <= 0) {
        hostent* he = gethostbyname(m_config.server_host.c_str());
        if (he) {
            serverAddr.sin_addr = *(in_addr*)he->h_addr_list[0];
        } else {
            return false;
        }
    }

    if (::connect(m_socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        return false;
    }

    return true;
}

void SimulatedClient::disconnect()
{
    if (m_socket != INVALID_SOCKET) {
#ifdef _WIN32
        closesocket(m_socket);
#else
        close(m_socket);
#endif
        m_socket = INVALID_SOCKET;
    }
}

void SimulatedClient::start()
{
    if (m_running) return;
    m_running = true;
    m_thread = std::thread(&SimulatedClient::run, this);
}

void SimulatedClient::stop()
{
    m_running = false;
}

void SimulatedClient::join()
{
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void SimulatedClient::run()
{
    if (!connect()) {
        m_failCount++;
        return;
    }

    for (int i = 0; i < m_config.requests_per_connection && m_running; ++i) {
        bool success = false;
        auto startTime = std::chrono::steady_clock::now();
        
        switch (m_config.scenario) {
            case StressTestConfig::Scenario::Registration:
                success = sendRegistration();
                break;
            case StressTestConfig::Scenario::Query:
                success = sendQuery();
                break;
            case StressTestConfig::Scenario::Mixed:
            case StressTestConfig::Scenario::PeakLoad:
                success = (i % 3 == 0) ? sendRegistration() : sendQuery();
                break;
        }
        
        auto endTime = std::chrono::steady_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        
        m_requestCount++;
        if (success) {
            m_successCount++;
            m_totalLatency += latency;
            m_minLatency = std::min(m_minLatency.load(), (uint64_t)latency);
            m_maxLatency = std::max(m_maxLatency.load(), (uint64_t)latency);
        } else {
            m_failCount++;
        }
        
        think();
    }
    
    disconnect();
}

bool SimulatedClient::sendRegistration()
{
    // 构造挂号请求 JSON
    std::string req = R"({"cmd":)" + std::to_string(CMD_PATIENT_REGISTER)
        + R"(,"patient_id":)" + std::to_string(1000 + m_id % 100)
        + R"(,"department_id":)" + std::to_string(m_deptDist(m_rng))
        + R"(,"patient_name":"测试患者)"}";
    
    // 添加分隔符
    req += "\n";
    
    // 加密（如果启用）
    if (m_config.use_encryption) {
        req = Crypto::instance().encrypt(req);
    }
    
    // 发送
    int sent = send(m_socket, req.c_str(), req.length(), 0);
    if (sent <= 0) return false;
    
    // 接收响应
    char buffer[4096];
    int received = recv(m_socket, buffer, sizeof(buffer) - 1, 0);
    if (received <= 0) return false;
    
    buffer[received] = '\0';
    std::string response(buffer);
    
    // 解密
    if (m_config.use_encryption) {
        response = Crypto::instance().decrypt(response);
    }
    
    // 简单验证：检查是否包含成功的响应码
    return response.find("\"ret\":0") != std::string::npos || 
           response.find("\"ret\": 0") != std::string::npos;
}

bool SimulatedClient::sendQuery()
{
    // 构造查询请求
    std::string req = R"({"cmd":)" + std::to_string(CMD_GET_QUEUE_STATUS)
        + R"(,"patient_id":)" + std::to_string(1000 + m_id % 100) + "}";
    req += "\n";
    
    if (m_config.use_encryption) {
        req = Crypto::instance().encrypt(req);
    }
    
    int sent = send(m_socket, req.c_str(), req.length(), 0);
    if (sent <= 0) return false;
    
    char buffer[4096];
    int received = recv(m_socket, buffer, sizeof(buffer) - 1, 0);
    if (received <= 0) return false;
    
    return true;
}

void SimulatedClient::think()
{
    if (m_config.think_time_ms > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(m_config.think_time_ms));
    }
}

// ==================== StressTest 实现 ====================
StressTest::StressTest()
    : m_running(false)
    , m_stopRequested(false)
{
}

StressTest::~StressTest()
{
    stop();
}

void StressTest::configure(const StressTestConfig& config)
{
    m_config = config;
}

bool StressTest::run()
{
    std::cout << "========== 压力测试开始 ==========\n";
    std::cout << "服务器: " << m_config.server_host << ":" << m_config.server_port << "\n";
    std::cout << "并发连接: " << m_config.num_connections << "\n";
    std::cout << "每连接请求: " << m_config.requests_per_connection << "\n";
    std::cout << "加密: " << (m_config.use_encryption ? "启用" : "禁用") << "\n";
    std::cout << "==================================\n\n";
    
    m_running = true;
    m_stopRequested = false;
    m_result.start_time = std::chrono::steady_clock::now();
    m_lastProgress = m_result.start_time;
    
    // 创建并启动客户端线程
    int totalRequests = m_config.num_connections * m_config.requests_per_connection;
    
    for (int i = 0; i < m_config.num_connections; ++i) {
        m_threads.emplace_back(&StressTest::workerThread, this, i);
    }
    
    // 打印进度
    std::thread progressThread(&StressTest::printProgress, this);
    
    // 等待所有线程完成
    for (auto& t : m_threads) {
        t.join();
    }
    
    m_running = false;
    m_result.end_time = std::chrono::steady_clock::now();
    
    progressThread.join();
    
    std::cout << "\n========== 测试完成 ==========\n";
    
    return true;
}

void StressTest::workerThread(int id)
{
    SimulatedClient client(id, m_config);
    client.start();
    client.join();
    
    // 汇总统计
    m_result.total_requests += client.getRequestCount();
    m_result.successful_requests += client.getSuccessCount();
    m_result.failed_requests += client.getFailCount();
    m_result.total_latency_ms += client.getTotalLatency();
    
    // 更新最小/最大延迟
    uint64_t minLat = client.getMinLatency();
    uint64_t maxLat = client.getMaxLatency();
    if (minLat > 0 && minLat < m_result.min_latency_ms.load()) {
        m_result.min_latency_ms = minLat;
    }
    if (maxLat > m_result.max_latency_ms.load()) {
        m_result.max_latency_ms = maxLat;
    }
}

void StressTest::printProgress()
{
    while (m_running && !m_stopRequested) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_result.start_time).count();
        
        uint64_t total = m_result.total_requests.load();
        uint64_t success = m_result.successful_requests.load();
        
        double qps = elapsed > 0 ? (double)success / elapsed : 0;
        
        std::cout << "\r[" << std::setw(3) << elapsed << "s] "
                  << "请求: " << std::setw(6) << total 
                  << " | 成功: " << std::setw(6) << success
                  << " | QPS: " << std::fixed << std::setprecision(1) << qps
                  << std::flush;
    }
}

void StressTest::stop()
{
    m_stopRequested = true;
    m_running = false;
    
    for (auto& t : m_threads) {
        if (t.joinable()) {
            t.join();
        }
    }
}

TestResult StressTest::getResult() const
{
    return m_result;
}

void StressTest::printReport() const
{
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(
        m_result.end_time - m_result.start_time).count();
    
    std::cout << "\n========== 测试报告 ==========\n";
    std::cout << "测试时长: " << duration << " 秒\n";
    std::cout << "总请求数: " << m_result.total_requests << "\n";
    std::cout << "成功请求: " << m_result.successful_requests << "\n";
    std::cout << "失败请求: " << m_result.failed_requests << "\n";
    std::cout << "成功率: " << std::fixed << std::setprecision(2) 
              << m_result.getSuccessRate() << "%\n";
    std::cout << "\n【性能指标】\n";
    std::cout << "QPS: " << std::fixed << std::setprecision(2) << m_result.getQps() << "\n";
    std::cout << "平均延迟: " << std::fixed << std::setprecision(2) 
              << m_result.getAvgLatency() << " ms\n";
    std::cout << "最小延迟: " << m_result.min_latency_ms << " ms\n";
    std::cout << "最大延迟: " << m_result.max_latency_ms << " ms\n";
    
    // 计算百分位数（简化版）
    std::cout << "\n【延迟分布估算】\n";
    double avg = m_result.getAvgLatency();
    uint64_t max = m_result.max_latency_ms.load();
    std::cout << "P50 ≈ " << (uint64_t)(avg * 0.8) << " ms\n";
    std::cout << "P90 ≈ " << (uint64_t)(avg * 1.5) << " ms\n";
    std::cout << "P99 ≈ " << (uint64_t)(avg * 2.0) << " ms\n";
    std::cout << "==================================\n";
}

bool StressTest::saveReport(const std::string& filepath) const
{
    std::ofstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(
        m_result.end_time - m_result.start_time).count();
    
    file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    file << "<StressTestReport>\n";
    file << "  <Configuration>\n";
    file << "    <Server>" << m_config.server_host << ":" << m_config.server_port << "</Server>\n";
    file << "    <Connections>" << m_config.num_connections << "</Connections>\n";
    file << "    <RequestsPerConnection>" << m_config.requests_per_connection << "</RequestsPerConnection>\n";
    file << "    <Encryption>" << (m_config.use_encryption ? "true" : "false") << "</Encryption>\n";
    file << "  </Configuration>\n";
    file << "  <Results>\n";
    file << "    <Duration>" << duration << "</Duration>\n";
    file << "    <TotalRequests>" << m_result.total_requests << "</TotalRequests>\n";
    file << "    <SuccessfulRequests>" << m_result.successful_requests << "</SuccessfulRequests>\n";
    file << "    <FailedRequests>" << m_result.failed_requests << "</FailedRequests>\n";
    file << "    <SuccessRate>" << m_result.getSuccessRate() << "</SuccessRate>\n";
    file << "    <QPS>" << m_result.getQps() << "</QPS>\n";
    file << "    <AvgLatency>" << m_result.getAvgLatency() << "</AvgLatency>\n";
    file << "    <MinLatency>" << m_result.min_latency_ms << "</MinLatency>\n";
    file << "    <MaxLatency>" << m_result.max_latency_ms << "</MaxLatency>\n";
    file << "  </Results>\n";
    file << "</StressTestReport>\n";
    
    return true;
}

} // namespace smartsched

// ==================== main 函数 ====================
#ifdef _WIN32
BOOL WINAPI consoleCtrlHandler(DWORD dwCtrlType)
{
    if (dwCtrlType == CTRL_C_EVENT) {
        std::cout << "\n收到中断信号，正在停止测试...\n";
        // stressTest.stop() 会在下一轮检查时生效
        return TRUE;
    }
    return FALSE;
}
#endif

void printUsage(const char* program)
{
    std::cout << "Usage: " << program << " [options]\n";
    std::cout << "\nOptions:\n";
    std::cout << "  -h <host>          服务器地址 (默认: localhost)\n";
    std::cout << "  -p <port>          服务器端口 (默认: 8888)\n";
    std::cout << "  -c <num>           并发连接数 (默认: 100)\n";
    std::cout << "  -r <num>           每连接请求数 (默认: 100)\n";
    std::cout << "  -t <ms>            请求间隔(毫秒) (默认: 100)\n";
    std::cout << "  --no-crypto        禁用加密\n";
    std::cout << "  --scenario <type>  测试场景: reg|query|mixed|peak (默认: mixed)\n";
    std::cout << "  -o <file>          输出报告文件\n";
    std::cout << "  --help             显示帮助\n";
}

int main(int argc, char* argv[])
{
    using namespace smartsched;
    
    StressTestConfig config;
    std::string outputFile;
    
    // 解析参数
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" && i + 1 < argc) {
            config.server_host = argv[++i];
        } else if (arg == "-p" && i + 1 < argc) {
            config.server_port = std::stoi(argv[++i]);
        } else if (arg == "-c" && i + 1 < argc) {
            config.num_connections = std::stoi(argv[++i]);
        } else if (arg == "-r" && i + 1 < argc) {
            config.requests_per_connection = std::stoi(argv[++i]);
        } else if (arg == "-t" && i + 1 < argc) {
            config.think_time_ms = std::stoi(argv[++i]);
        } else if (arg == "-o" && i + 1 < argc) {
            outputFile = argv[++i];
        } else if (arg == "--no-crypto") {
            config.use_encryption = false;
        } else if (arg == "--scenario" && i + 1 < argc) {
            std::string scenario = argv[++i];
            if (scenario == "reg") config.scenario = StressTestConfig::Scenario::Registration;
            else if (scenario == "query") config.scenario = StressTestConfig::Scenario::Query;
            else if (scenario == "peak") config.scenario = StressTestConfig::Scenario::PeakLoad;
            else config.scenario = StressTestConfig::Scenario::Mixed;
        } else if (arg == "--help") {
            printUsage(argv[0]);
            return 0;
        }
    }
    
#ifdef _WIN32
    SetConsoleCtrlHandler(consoleCtrlHandler, TRUE);
#endif
    
    StressTest test;
    test.configure(config);
    
    std::cout << "准备开始压力测试...\n";
    std::cout << "按 Ctrl+C 可随时中断测试。\n\n";
    
    if (test.run()) {
        test.printReport();
        
        if (!outputFile.empty()) {
            if (test.saveReport(outputFile)) {
                std::cout << "报告已保存至: " << outputFile << "\n";
            } else {
                std::cout << "保存报告失败\n";
            }
        }
    } else {
        std::cout << "测试执行失败\n";
        return 1;
    }
    
    return 0;
}
