#pragma once
#include "common.h"
#include "ProtocolParser.h"
#include "TcpServer.h"

void handleMessageList(SOCKET client, const json& data, const std::string& token);
void handleMessageSend(SOCKET client, const json& data, const std::string& token);
void handleMessageRead(SOCKET client, const json& data, const std::string& token);
void handleMessageDelete(SOCKET client, const json& data, const std::string& token);
