#pragma once
#include "common.h"
#include "ProtocolParser.h"
#include "TcpServer.h"

void handlePaymentDispatch(SOCKET client, const json& data, const std::string& token);
void handlePaymentCreate(SOCKET client, const json& data, const std::string& token);
void handlePaymentStatus(SOCKET client, const json& data, const std::string& token);
