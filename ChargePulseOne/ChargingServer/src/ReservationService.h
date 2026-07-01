#pragma once
#include "common.h"
#include "ProtocolParser.h"
#include "TcpServer.h"

void handleReserveDispatch(SOCKET client, const json& data, const std::string& token);
void handleReserveCreate(SOCKET client, const json& data, const std::string& token);
void handleReserveList(SOCKET client, const json& data, const std::string& token);
void handleReserveCancel(SOCKET client, const json& data, const std::string& token);
