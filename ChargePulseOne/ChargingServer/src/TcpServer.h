#pragma once
#include "common.h"
#include <functional>
#include <string>
#include <thread>
#include <map>
#include <mutex>
#include <atomic>
#include <chrono>
#include <condition_variable>

#ifdef _WIN32
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
    using socklen_t = int;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    using SOCKET = int;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
#endif

// 客户端连接信息
struct ClientInfo {
    SOCKET socket;
    std::string ip;
    int64_t lastHeartbeat;
    bool authenticated;
    int userId;
    std::thread* workerThread;
    
    ClientInfo() : socket(INVALID_SOCKET), lastHeartbeat(0), authenticated(false), userId(0), workerThread(nullptr) {}
};

class TcpServer {
public:
    using MessageCallback = std::function<void(SOCKET, const std::string&)>;
    using ConnectCallback = std::function<void(SOCKET)>;
    using DisconnectCallback = std::function<void(SOCKET)>;
    using HeartbeatCallback = std::function<void(SOCKET)>;

    TcpServer();
    ~TcpServer();
    bool start(int port);
    void stop();
    void setMessageCallback(MessageCallback cb);
    void setConnectCallback(ConnectCallback cb);
    void setDisconnectCallback(DisconnectCallback cb);
    void setHeartbeatCallback(HeartbeatCallback cb);
    
    // 发送消息（线程安全）
    void sendMessage(SOCKET client, const std::string& msg);
    void sendMessageToUser(int userId, const std::string& msg);
    void broadcastMessage(const std::string& msg);
    
    // 客户端管理
    size_t getClientCount() const;
    bool isClientConnected(SOCKET client) const;
    void disconnectClient(SOCKET client);
    void disconnectAllClients();
    
    // 心跳设置
    void setHeartbeatInterval(int seconds);
    void setHeartbeatTimeout(int seconds);
    
    // 获取客户端信息
    std::string getClientIP(SOCKET client) const;
    int64_t getClientLastHeartbeat(SOCKET client) const;

private:
    void acceptLoop();
    void clientThread(SOCKET client);
    void heartbeatChecker();
    void closeSocket(SOCKET s);
    
    SOCKET listenSocket;
    std::thread acceptThread;
    std::thread heartbeatThread;
    std::atomic<bool> running{false};
    std::atomic<bool> heartbeatRunning{false};
    
    MessageCallback onMessage;
    ConnectCallback onConnect;
    DisconnectCallback onDisconnect;
    HeartbeatCallback onHeartbeat;
    
    // 客户端管理
    std::map<SOCKET, ClientInfo> clients;
    mutable std::mutex clientsMutex;
    
    // 心跳配置
    int heartbeatIntervalSeconds = 30;  // 心跳间隔
    int heartbeatTimeoutSeconds = 90;     // 心跳超时（3倍间隔）
    
    // 发送锁
    std::mutex sendMutex;

    void addClient(SOCKET client, const std::string& ip);
    void removeClient(SOCKET client);
    void updateHeartbeat(SOCKET client);
};

// 心跳命令定义
namespace CMD {
    const std::string HEARTBEAT = "9999";
    const std::string HEARTBEAT_RESPONSE = "9998";
}
