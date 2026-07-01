#pragma once
#include <string>
#include "json.hpp"

#ifdef _WIN32
    #include <winsock2.h>
#else
    #include <sys/socket.h>
#endif

using json = nlohmann::json;

// Analytics command codes
namespace CMD {
    const std::string ANALYTICS_OVERVIEW = "8001";
    const std::string ANALYTICS_REVENUE = "8002";
    const std::string ANALYTICS_CHARGER = "8003";
    const std::string USER_STATS = "8004";
}

void handleAnalyticsOverview(SOCKET client, const json& data, const std::string& token);
void handleAnalyticsRevenue(SOCKET client, const json& data, const std::string& token);
void handleAnalyticsChargerStats(SOCKET client, const json& data, const std::string& token);
void handleUserStatistics(SOCKET client, const json& data, const std::string& token);
