#pragma once
#include "common.h"
#include "ProtocolParser.h"
#include "TcpServer.h"

void handleMemberLevels(SOCKET client, const json& data, const std::string& token);
void handleMemberStats(SOCKET client, const json& data, const std::string& token);
