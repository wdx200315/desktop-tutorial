#pragma once
#include "common.h"
#include "ProtocolParser.h"
#include "TcpServer.h"

void handleConfigGet(SOCKET client, const json& data, const std::string& token);
void handleConfigSet(SOCKET client, const json& data, const std::string& token);
