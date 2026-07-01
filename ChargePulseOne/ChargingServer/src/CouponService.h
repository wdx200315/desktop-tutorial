#pragma once
#include "common.h"
#include "ProtocolParser.h"
#include "TcpServer.h"

void handleCouponList(SOCKET client, const json& data, const std::string& token);
void handleCouponAdd(SOCKET client, const json& data, const std::string& token);
void handleCouponEdit(SOCKET client, const json& data, const std::string& token);
void handleCouponDelete(SOCKET client, const json& data, const std::string& token);
void handleCouponClaim(SOCKET client, const json& data, const std::string& token);
void handleCouponUserList(SOCKET client, const json& data, const std::string& token);
void handleCouponUse(SOCKET client, const json& data, const std::string& token);
