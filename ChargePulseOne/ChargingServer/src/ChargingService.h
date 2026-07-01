#pragma once
#include <string>

void handleStartCharge(int client, const nlohmann::json& data, const std::string& token, int serverClient = 0);
void handleStopCharge(int client, const nlohmann::json& data, const std::string& token);
void handleChargeMonitor(int client, const nlohmann::json& data, const std::string& token);
void updateChargeSimulation();
