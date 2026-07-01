#pragma once
#include "common.h"
#include "TcpServer.h"
#include "ProtocolParser.h"
#include <functional>
#include <unordered_map>

class ClientHandler {
public:
    ClientHandler(TcpServer& server);
    void onMessage(SOCKET client, const std::string& raw);
    void onConnect(SOCKET client);
    void onDisconnect(SOCKET client);
    
private:
    TcpServer& server;
    // 命令处理函数映射
    using HandlerFunc = std::function<void(SOCKET, const json&, const std::string&)>;
    std::unordered_map<std::string, HandlerFunc> handlers;
    
    void registerHandlers();
    void response(SOCKET client, const std::string& cmd, const json& data, const std::string& token="");
    void errorResponse(SOCKET client, const std::string& cmd, const std::string& errMsg);
    bool checkAuth(SOCKET client, const std::string& token);
};
