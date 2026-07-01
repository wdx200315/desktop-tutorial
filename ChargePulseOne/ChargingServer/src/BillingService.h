#pragma once
#include "common.h"
#include "ProtocolParser.h"
#include "TcpServer.h"

void handleRateList(SOCKET client, const json& data, const std::string& token);
void handleRateAdd(SOCKET client, const json& data, const std::string& token);
void handleRateUpdate(SOCKET client, const json& data, const std::string& token);
void handleRateDelete(SOCKET client, const json& data, const std::string& token);
void handleFeeEstimate(SOCKET client, const json& data, const std::string& token);
void handleRealBilling(SOCKET client, const json& data, const std::string& token);
