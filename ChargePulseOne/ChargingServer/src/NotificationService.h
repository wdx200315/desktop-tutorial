#pragma once
#include "common.h"
#include "ProtocolParser.h"
#include "TcpServer.h"

void handleNotificationRegister(SOCKET client, const json& data, const std::string& token);
