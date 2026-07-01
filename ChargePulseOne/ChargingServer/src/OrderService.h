#pragma once
#include "common.h"
#include "ProtocolParser.h"
#include "TcpServer.h"

void handleOrderList(SOCKET client, const json& data, const std::string& token);
void handleOrderDetail(SOCKET client, const json& data, const std::string& token);
void handleOrderExport(SOCKET client, const json& data, const std::string& token);
