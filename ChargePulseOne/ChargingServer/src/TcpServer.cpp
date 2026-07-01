#include "TcpServer.h"
#include "Logger.h"
#include <cstring>

TcpServer::TcpServer() {
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);
#endif
}

TcpServer::~TcpServer() {
    stop();
#ifdef _WIN32
    WSACleanup();
#endif
}

void TcpServer::closeSocket(SOCKET s) {
    if (s == INVALID_SOCKET) return;
#ifdef _WIN32
    shutdown(s, SD_BOTH);
    closesocket(s);
#else
    shutdown(s, SHUT_RDWR);
    close(s);
#endif
}

bool TcpServer::start(int port) {
    listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(listenSocket == INVALID_SOCKET) {
        Logger::instance().log(ERROR, "Failed to create socket");
        return false;
    }

    // 设置地址重用
    int opt = 1;
    setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    
    // 设置TCP保活
    setsockopt(listenSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(listenSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        Logger::instance().log(ERROR, "Bind failed on port " + std::to_string(port));
        closeSocket(listenSocket);
        return false;
    }

    if(listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        Logger::instance().log(ERROR, "Listen failed");
        closeSocket(listenSocket);
        return false;
    }

    running = true;
    heartbeatRunning = true;
    
    // 启动接受线程
    acceptThread = std::thread(&TcpServer::acceptLoop, this);
    
    // 启动心跳检测线程
    heartbeatThread = std::thread(&TcpServer::heartbeatChecker, this);
    
    Logger::instance().log(INFO, "TCP Server started on port " + std::to_string(port));
    Logger::instance().log(INFO, "Heartbeat interval: " + std::to_string(heartbeatIntervalSeconds) + "s, timeout: " + std::to_string(heartbeatTimeoutSeconds) + "s");
    return true;
}

void TcpServer::stop() {
    running = false;
    heartbeatRunning = false;
    
    // 关闭所有客户端
    disconnectAllClients();
    
    // 关闭监听socket
    closeSocket(listenSocket);
    
    // 等待线程结束
    if(acceptThread.joinable()) {
        acceptThread.join();
    }
    if(heartbeatThread.joinable()) {
        heartbeatThread.join();
    }
    
    Logger::instance().log(INFO, "TCP Server stopped");
}

void TcpServer::setMessageCallback(MessageCallback cb) { 
    onMessage = std::move(cb); 
}

void TcpServer::setConnectCallback(ConnectCallback cb) { 
    onConnect = std::move(cb); 
}

void TcpServer::setDisconnectCallback(DisconnectCallback cb) { 
    onDisconnect = std::move(cb); 
}

void TcpServer::setHeartbeatCallback(HeartbeatCallback cb) { 
    onHeartbeat = std::move(cb); 
}

void TcpServer::setHeartbeatInterval(int seconds) {
    heartbeatIntervalSeconds = seconds;
    heartbeatTimeoutSeconds = seconds * 3;
}

void TcpServer::setHeartbeatTimeout(int seconds) {
    heartbeatTimeoutSeconds = seconds;
}

void TcpServer::acceptLoop() {
    while(running) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(listenSocket, &readfds);
        
        timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        int activity = select(listenSocket + 1, &readfds, nullptr, nullptr, &timeout);
        
        if (activity < 0) {
            if (running) Logger::instance().log(ERROR, "Select failed in accept loop");
            continue;
        }
        
        if (activity == 0) continue;
        
        sockaddr_in clientAddr{};
        socklen_t addrLen = sizeof(clientAddr);
        SOCKET client = accept(listenSocket, (sockaddr*)&clientAddr, &addrLen);
        
        if(client == INVALID_SOCKET) {
            if(running) Logger::instance().log(ERROR, "Accept failed");
            continue;
        }
        
        // 设置TCP_NODELAY减少延迟
        int flag = 1;
        setsockopt(client, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));
        
        // 设置接收超时
#ifdef _WIN32
        int timeout = 30000; // 30秒
        setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
#else
        timeval tv;
        tv.tv_sec = 30;
        tv.tv_usec = 0;
        setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif
        
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, ip, sizeof(ip));
        
        // 添加到客户端列表
        addClient(client, std::string(ip));
        
        Logger::instance().log(INFO, "Client connected: " + std::string(ip) + ", total clients: " + std::to_string(getClientCount()));
        
        if(onConnect) onConnect(client);
        
        // 启动客户端处理线程
        std::thread t(&TcpServer::clientThread, this, client);
        
        // 将线程所有权转移到客户端信息中
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            if (clients.find(client) != clients.end()) {
                clients[client].workerThread = new std::thread(std::move(t));
            }
        }
    }
}

void TcpServer::clientThread(SOCKET client) {
    char buffer[8192];  // 增大缓冲区
    int bufferPos = 0;
    
    while(running) {
        memset(buffer + bufferPos, 0, sizeof(buffer) - bufferPos);
        int bytes = recv(client, buffer + bufferPos, sizeof(buffer) - bufferPos - 1, 0);
        
        if(bytes <= 0) {
            if (bytes == 0) {
                Logger::instance().log(INFO, "Client disconnected normally");
            } else {
                int err = errno;
                Logger::instance().log(INFO, "Client disconnected, error: " + std::to_string(err));
            }
            break;
        }
        
        bufferPos += bytes;
        
        // 处理可能存在的多个消息（以换行符分隔）
        char* start = buffer;
        char* end;
        while ((end = strchr(start, '\n')) != nullptr && bufferPos > 0) {
            *end = '\0';
            std::string msg(start, end - start);
            
            // 更新心跳时间
            updateHeartbeat(client);
            
            // 处理心跳命令
            if (msg.find("\"cmd\":\"9999\"") != std::string::npos || 
                msg.find("\"cmd\":\"9998\"") != std::string::npos) {
                // 发送心跳响应
                std::string response = "{\"cmd\":\"9998\",\"status\":\"ok\"}\n";
                send(client, response.c_str(), response.size(), 0);
                Logger::instance().log(DEBUG, "Heartbeat processed for client");
            } else {
                // 处理普通消息
                if(onMessage) onMessage(client, msg);
            }
            
            start = end + 1;
            bufferPos -= (end - start + 1);
        }
        
        // 移动剩余数据到缓冲区开头
        if (start != buffer && bufferPos > 0) {
            memmove(buffer, start, bufferPos);
        }
        
        // 如果缓冲区已满但没有换行符，清空
        if (bufferPos >= (int)sizeof(buffer) - 1) {
            Logger::instance().log(WARN, "Buffer overflow, clearing");
            bufferPos = 0;
        }
    }
    
    // 清理
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        if (clients.find(client) != clients.end() && clients[client].workerThread) {
            if (clients[client].workerThread->joinable()) {
                clients[client].workerThread->join();
            }
            delete clients[client].workerThread;
            clients[client].workerThread = nullptr;
        }
    }
    
    if(onDisconnect) onDisconnect(client);
    removeClient(client);
    closeSocket(client);
}

void TcpServer::heartbeatChecker() {
    while(heartbeatRunning) {
        // 每10秒检查一次
        std::this_thread::sleep_for(std::chrono::seconds(10));
        
        if (!running) break;
        
        int64_t now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        std::lock_guard<std::mutex> lock(clientsMutex);
        std::vector<SOCKET> timeoutClients;
        
        for (auto& pair : clients) {
            SOCKET client = pair.first;
            ClientInfo& info = pair.second;
            
            int64_t elapsed = now - info.lastHeartbeat;
            if (elapsed > heartbeatTimeoutSeconds) {
                timeoutClients.push_back(client);
                Logger::instance().log(WARN, "Client heartbeat timeout: " + info.ip + 
                                       ", idle: " + std::to_string(elapsed) + "s");
            }
        }
        
        // 断开超时的客户端
        for (SOCKET client : timeoutClients) {
            if (onDisconnect) onDisconnect(client);
            closeSocket(client);
            removeClient(client);
        }
    }
}

void TcpServer::sendMessage(SOCKET client, const std::string& msg) {
    std::lock_guard<std::mutex> lock(sendMutex);
    
    std::string sendData = msg + "\n";
    int result = send(client, sendData.c_str(), sendData.size(), 0);
    
    if (result == SOCKET_ERROR) {
        Logger::instance().log(ERROR, "Failed to send message to client");
    }
}

void TcpServer::sendMessageToUser(int userId, const std::string& msg) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    
    for (const auto& pair : clients) {
        if (pair.second.userId == userId && pair.second.authenticated) {
            sendMessage(pair.first, msg);
        }
    }
}

void TcpServer::broadcastMessage(const std::string& msg) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    
    for (const auto& pair : clients) {
        sendMessage(pair.first, msg);
    }
}

size_t TcpServer::getClientCount() const {
    std::lock_guard<std::mutex> lock(clientsMutex);
    return clients.size();
}

bool TcpServer::isClientConnected(SOCKET client) const {
    std::lock_guard<std::mutex> lock(clientsMutex);
    return clients.find(client) != clients.end();
}

void TcpServer::disconnectClient(SOCKET client) {
    Logger::instance().log(INFO, "Manually disconnecting client");
    if (onDisconnect) onDisconnect(client);
    closeSocket(client);
    removeClient(client);
}

void TcpServer::disconnectAllClients() {
    std::lock_guard<std::mutex> lock(clientsMutex);
    
    for (auto& pair : clients) {
        SOCKET client = pair.first;
        ClientInfo& info = pair.second;
        
        closeSocket(client);
        
        if (info.workerThread) {
            if (info.workerThread->joinable()) {
                info.workerThread->join();
            }
            delete info.workerThread;
            info.workerThread = nullptr;
        }
    }
    
    clients.clear();
    Logger::instance().log(INFO, "All clients disconnected");
}

std::string TcpServer::getClientIP(SOCKET client) const {
    std::lock_guard<std::mutex> lock(clientsMutex);
    auto it = clients.find(client);
    if (it != clients.end()) {
        return it->second.ip;
    }
    return "";
}

int64_t TcpServer::getClientLastHeartbeat(SOCKET client) const {
    std::lock_guard<std::mutex> lock(clientsMutex);
    auto it = clients.find(client);
    if (it != clients.end()) {
        return it->second.lastHeartbeat;
    }
    return 0;
}

void TcpServer::addClient(SOCKET client, const std::string& ip) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    
    ClientInfo info;
    info.socket = client;
    info.ip = ip;
    info.lastHeartbeat = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    info.authenticated = false;
    info.userId = 0;
    info.workerThread = nullptr;
    
    clients[client] = info;
}

void TcpServer::removeClient(SOCKET client) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    
    auto it = clients.find(client);
    if (it != clients.end()) {
        if (it->second.workerThread) {
            delete it->second.workerThread;
        }
        clients.erase(it);
    }
}

void TcpServer::updateHeartbeat(SOCKET client) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    
    auto it = clients.find(client);
    if (it != clients.end()) {
        it->second.lastHeartbeat = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
}
