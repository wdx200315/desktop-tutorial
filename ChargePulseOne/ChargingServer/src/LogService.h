#pragma once
#include "common.h"
#include "ProtocolParser.h"
#include "TcpServer.h"

void handleLogQuery(SOCKET client, const json& data, const std::string& token);
