#pragma once
#include "common.h"
#include "ProtocolParser.h"
#include "TcpServer.h"

// 认证
void handleLogin(SOCKET client, const json& data, const std::string& token);
void handleRegister(SOCKET client, const json& data, const std::string& token);
void handleChangePassword(SOCKET client, const json& data, const std::string& token);
void handleResetPassword(SOCKET client, const json& data, const std::string& token);

// 个人信息
void handleUserInfo(SOCKET client, const json& data, const std::string& token);
void handleUpdateUser(SOCKET client, const json& data, const std::string& token);

// 管理员功能
void handleUserList(SOCKET client, const json& data, const std::string& token);
void handleUserEdit(SOCKET client, const json& data, const std::string& token);  // 禁用/启用/改角色/重置密码

// 黑名单
void handleBlacklistAdd(SOCKET client, const json& data, const std::string& token);
void handleBlacklistRemove(SOCKET client, const json& data, const std::string& token);

// 车辆管理
void handleVehicleAdd(SOCKET client, const json& data, const std::string& token);
void handleVehicleList(SOCKET client, const json& data, const std::string& token);
void handleVehicleDelete(SOCKET client, const json& data, const std::string& token);
