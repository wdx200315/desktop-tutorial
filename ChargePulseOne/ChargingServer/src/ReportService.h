#pragma once
#include "common.h"
#include "ProtocolParser.h"
#include "TcpServer.h"

void handleReportDaily(SOCKET client, const json& data, const std::string& token);
void handleReportMonthly(SOCKET client, const json& data, const std::string& token);
