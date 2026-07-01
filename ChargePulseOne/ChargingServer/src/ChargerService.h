#pragma once
#include "common.h"
#include "ProtocolParser.h"
#include "TcpServer.h"

void handleChargerList(SOCKET client, const json& data, const std::string& token);
void handleChargerDetail(SOCKET client, const json& data, const std::string& token);
void handleChargerControl(SOCKET client, const json& data, const std::string& token); // 远程控制
void handleChargerAdd(SOCKET client, const json& data, const std::string& token);
void handleChargerUpdate(SOCKET client, const json& data, const std::string& token);
void handleChargerDelete(SOCKET client, const json& data, const std::string& token);
