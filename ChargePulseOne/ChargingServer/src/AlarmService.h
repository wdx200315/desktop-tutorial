#pragma once
#include "common.h"
#include "ProtocolParser.h"
#include "TcpServer.h"

void handleAlarmList(SOCKET client, const json& data, const std::string& token);
void handleAlarmHandle(SOCKET client, const json& data, const std::string& token);
void handleAlarmAutoRecover(SOCKET client, const json& data, const std::string& token);
