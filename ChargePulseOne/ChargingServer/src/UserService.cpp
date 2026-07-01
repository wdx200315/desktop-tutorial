#include "UserService.h"
#include "Logger.h"
#include "SessionManager.h"
#include "DatabaseManager.h"
#include "CryptoUtils.h"

#ifdef _WIN32
    #include <winsock2.h>
#else
    #include <sys/socket.h>
    #include <unistd.h>
    using SOCKET = int;
#endif

// ========== 常量定义 ==========
static const int MAX_USERNAME_LEN = 50;
static const int MAX_PASSWORD_LEN = 128;
static const int MAX_PHONE_LEN = 20;
static const int MAX_PLATE_LEN = 20;
static const int MAX_PAGE_SIZE = 100;
static const int DEFAULT_PAGE_SIZE = 20;

static void response(SOCKET client, const std::string& cmd, const json& data, const std::string& token = "") {
    json msg; msg["cmd"]=cmd; msg["status"]="ok"; msg["data"]=data;
    if(!token.empty()) msg["token"]=token;
    std::string raw = msg.dump();
    send(client, (raw+"\n").c_str(), raw.size()+1, 0);
}
static void error(SOCKET client, const std::string& cmd, const std::string& message) {
    json msg; msg["cmd"]=cmd; msg["status"]="error"; msg["message"]=message;
    std::string raw = msg.dump();
    send(client, (raw+"\n").c_str(), raw.size()+1, 0);
}

// ========== 输入验证函数 ==========

// 验证用户名格式（字母数字下划线，4-50位）
static bool validUsername(const std::string& username) {
    if (username.length() < 4 || username.length() > MAX_USERNAME_LEN) {
        return false;
    }
    for (char c : username) {
        if (!isalnum(c) && c != '_') {
            return false;
        }
    }
    return true;
}

// 验证手机号格式（中国大陆11位手机号）
static bool validPhone(const std::string& phone) {
    if (phone.empty()) return true;  // 手机号可选
    if (phone.length() != 11) return false;
    for (char c : phone) {
        if (!isdigit(c)) return false;
    }
    // 验证号段
    std::string prefixes[] = {"13", "14", "15", "16", "17", "18", "19"};
    for (const auto& prefix : prefixes) {
        if (phone.substr(0, 2) == prefix) return true;
    }
    return false;
}

// 验证车牌号格式
static bool validPlateNumber(const std::string& plate) {
    if (plate.empty()) return true;  // 车牌可选
    if (plate.length() > MAX_PLATE_LEN) return false;
    // 普通车牌：省份简称 + 字母数字组合
    return true;  // 简化验证
}

// 密码强度检查
static bool strongPassword(const std::string& pwd) {
    if(pwd.length() < 8 || pwd.length() > MAX_PASSWORD_LEN) return false;
    bool hasAlpha = false, hasDigit = false;
    for(char c : pwd) {
        if(isalpha(c)) hasAlpha = true;
        if(isdigit(c)) hasDigit = true;
    }
    return hasAlpha && hasDigit;
}

// 安全的SQL查询（使用转义）
static std::string safeWhere(const std::string& field, const std::string& value) {
    std::string escaped = DatabaseManager::instance().escape(value);
    return field + "='" + escaped + "'";
}

// ========== 会员等级计算 ==========

static std::string computeMemberLevel(int userId) {
    try {
        auto res = DatabaseManager::instance().query(
            "SELECT COALESCE(SUM(amount),0) as total FROM charging_orders WHERE user_id=" + 
            std::to_string(userId) + " AND status='completed'"
        );
        double total = res.empty() ? 0 : std::stod(res[0]["total"]);
        if(total >= 1000) return "gold";
        if(total >= 500)  return "silver";
        return "normal";
    } catch (const std::exception& e) {
        Logger::instance().log(ERROR, "computeMemberLevel error: " + std::string(e.what()));
        return "normal";
    }
}

// ================= 认证 =================

void handleLogin(SOCKET client, const json& data, const std::string& token) {
    std::string username = data.value("username", "");
    std::string password = data.value("password", "");
    
    // 输入验证
    if(username.empty() || password.empty()) {
        error(client, CMD::LOGIN, "Username and password required");
        return;
    }
    
    if(username.length() > MAX_USERNAME_LEN || password.length() > MAX_PASSWORD_LEN) {
        error(client, CMD::LOGIN, "Invalid input length");
        return;
    }
    
    std::string hashed = CryptoUtils::sha256(password);
    
    // 使用安全的参数化查询
    std::vector<SQLBinding> params = {
        {"username", username, true},
        {"password", hashed, true}
    };
    auto rows = DatabaseManager::instance().queryWithParams(
        "SELECT id, username, role, status FROM users WHERE username=:username AND password=:password",
        params
    );
    
    if(rows.empty()) {
        Logger::instance().log(WARN, "Login failed for user: " + username);
        error(client, CMD::LOGIN, "Invalid credentials");
        return;
    }
    
    auto& user = rows[0];
    if(user["status"] == "disabled") {
        error(client, CMD::LOGIN, "Account is disabled");
        return;
    }
    if(user["status"] == "blacklisted") {
        error(client, CMD::LOGIN, "Account is blacklisted");
        return;
    }
    
    try {
        int uid = std::stoi(user["id"]);
        std::string newToken = SessionManager::instance().createSession(uid, user["username"], user["role"]);
        
        // 记录登录日志
        DatabaseManager::instance().execute(
            "INSERT INTO login_logs (user_id, ip) VALUES (" + std::to_string(uid) + ", 'client')"
        );
        
        json resp;
        resp["user_id"] = uid;
        resp["username"] = user["username"];
        resp["role"] = user["role"];
        resp["member_level"] = computeMemberLevel(uid);
        
        Logger::instance().log(INFO, "User logged in: " + username);
        response(client, CMD::LOGIN, resp, newToken);
    } catch (const std::exception& e) {
        Logger::instance().log(ERROR, "Login error: " + std::string(e.what()));
        error(client, CMD::LOGIN, "Login processing error");
    }
}

void handleRegister(SOCKET client, const json& data, const std::string& token) {
    std::string username = data.value("username", "");
    std::string password = data.value("password", "");
    std::string phone = data.value("phone", "");
    std::string plate = data.value("plate_number", "");
    
    // 输入验证
    if(username.empty() || password.empty()) {
        error(client, CMD::REGISTER, "Username and password required");
        return;
    }
    
    // 详细验证
    if (!validUsername(username)) {
        error(client, CMD::REGISTER, "Username must be 4-50 chars, alphanumeric and underscore only");
        return;
    }
    
    if (!strongPassword(password)) {
        error(client, CMD::REGISTER, "Password too weak (min 8 chars, letters+digits required)");
        return;
    }
    
    if (!validPhone(phone)) {
        error(client, CMD::REGISTER, "Invalid phone number format");
        return;
    }
    
    if (!validPlateNumber(plate)) {
        error(client, CMD::REGISTER, "Invalid plate number format");
        return;
    }
    
    try {
        // 检查用户名唯一性
        auto existUser = DatabaseManager::instance().query(
            "SELECT id FROM users WHERE username='" + DatabaseManager::instance().escape(username) + "'"
        );
        if(!existUser.empty()) {
            error(client, CMD::REGISTER, "Username already exists");
            return;
        }
        
        // 检查手机号唯一性（如果提供）
        if(!phone.empty()) {
            auto existPhone = DatabaseManager::instance().query(
                "SELECT id FROM users WHERE phone='" + DatabaseManager::instance().escape(phone) + "'"
            );
            if(!existPhone.empty()) {
                error(client, CMD::REGISTER, "Phone already registered");
                return;
            }
        }
        
        std::string hashed = CryptoUtils::sha256(password);
        
        // 安全插入
        std::vector<SQLBinding> params = {
            {"username", username, true},
            {"password", hashed, true},
            {"phone", phone, true},
            {"plate_number", plate, true}
        };
        
        int affected = DatabaseManager::instance().executeWithParams(
            "INSERT INTO users (username, password, role, phone, plate_number) VALUES (:username, :password, 'driver', :phone, :plate_number)",
            params
        );
        
        if(affected <= 0) {
            error(client, CMD::REGISTER, "Registration failed");
            return;
        }
        
        int uid = DatabaseManager::instance().lastInsertId();
        std::string newToken = SessionManager::instance().createSession(uid, username, "driver");
        
        Logger::instance().log(INFO, "New user registered: " + username);
        
        json resp;
        resp["user_id"] = uid;
        resp["username"] = username;
        resp["member_level"] = "normal";
        response(client, CMD::REGISTER, resp, newToken);
        
    } catch (const std::exception& e) {
        Logger::instance().log(ERROR, "Registration error: " + std::string(e.what()));
        error(client, CMD::REGISTER, "Registration failed");
    }
}

void handleChangePassword(SOCKET client, const json& data, const std::string& token) {
    Session sess;
    if(!SessionManager::instance().validate(token, sess)) {
        error(client, CMD::CHANGE_PWD, "Authentication required");
        return;
    }
    
    std::string oldPwd = data.value("old_password", "");
    std::string newPwd = data.value("new_password", "");
    
    if(oldPwd.empty() || newPwd.empty()) {
        error(client, CMD::CHANGE_PWD, "Old and new password required");
        return;
    }
    
    if(!strongPassword(newPwd)) {
        error(client, CMD::CHANGE_PWD, "New password too weak (min 8 chars, letters+digits)");
        return;
    }
    
    // 防止新密码与旧密码相同
    if(oldPwd == newPwd) {
        error(client, CMD::CHANGE_PWD, "New password must be different from old password");
        return;
    }
    
    try {
        std::string oldHashed = CryptoUtils::sha256(oldPwd);
        auto rows = DatabaseManager::instance().query(
            "SELECT id FROM users WHERE id=" + std::to_string(sess.userId) + " AND password='" + oldHashed + "'"
        );
        
        if(rows.empty()) {
            error(client, CMD::CHANGE_PWD, "Old password incorrect");
            return;
        }
        
        std::string newHashed = CryptoUtils::sha256(newPwd);
        DatabaseManager::instance().execute(
            "UPDATE users SET password='" + newHashed + "' WHERE id=" + std::to_string(sess.userId)
        );
        
        Logger::instance().log(INFO, "Password changed for user: " + std::to_string(sess.userId));
        response(client, CMD::CHANGE_PWD, {{"message", "Password changed successfully"}}, token);
        
    } catch (const std::exception& e) {
        Logger::instance().log(ERROR, "Change password error: " + std::string(e.what()));
        error(client, CMD::CHANGE_PWD, "Password change failed");
    }
}

void handleResetPassword(SOCKET client, const json& data, const std::string& token) {
    std::string username = data.value("username", "");
    std::string code = data.value("code", "");
    std::string newPwd = data.value("new_password", "");
    
    if(username.empty() || code.empty() || newPwd.empty()) {
        error(client, CMD::RESET_PWD, "Username, code and new password required");
        return;
    }
    
    // 验证码验证（实际应发送短信验证码，这里简化）
    if(code != "123456") {
        error(client, CMD::RESET_PWD, "Invalid verification code");
        return;
    }
    
    if(!validUsername(username)) {
        error(client, CMD::RESET_PWD, "Invalid username");
        return;
    }
    
    if(!strongPassword(newPwd)) {
        error(client, CMD::RESET_PWD, "New password too weak (min 8 chars, letters+digits)");
        return;
    }
    
    try {
        std::string newHashed = CryptoUtils::sha256(newPwd);
        int affected = DatabaseManager::instance().execute(
            "UPDATE users SET password='" + newHashed + "' WHERE username='" + 
            DatabaseManager::instance().escape(username) + "'"
        );
        
        if(affected <= 0) {
            error(client, CMD::RESET_PWD, "User not found");
            return;
        }
        
        Logger::instance().log(INFO, "Password reset for user: " + username);
        response(client, CMD::RESET_PWD, {{"message", "Password reset successful"}});
        
    } catch (const std::exception& e) {
        Logger::instance().log(ERROR, "Reset password error: " + std::string(e.what()));
        error(client, CMD::RESET_PWD, "Password reset failed");
    }
}

// ================= 个人信息 =================

void handleUserInfo(SOCKET client, const json& data, const std::string& token) {
    Session sess;
    if(!SessionManager::instance().validate(token, sess)) {
        error(client, CMD::USER_INFO, "Authentication required");
        return;
    }
    
    try {
        auto rows = DatabaseManager::instance().query(
            "SELECT id, username, role, phone, plate_number, balance, status FROM users WHERE id=" + 
            std::to_string(sess.userId)
        );
        
        if(rows.empty()) {
            error(client, CMD::USER_INFO, "User not found");
            return;
        }
        
        json resp = rows[0];
        resp["member_level"] = computeMemberLevel(sess.userId);
        response(client, CMD::USER_INFO, resp, token);
        
    } catch (const std::exception& e) {
        Logger::instance().log(ERROR, "Get user info error: " + std::string(e.what()));
        error(client, CMD::USER_INFO, "Failed to get user info");
    }
}

void handleUpdateUser(SOCKET client, const json& data, const std::string& token) {
    Session sess;
    if(!SessionManager::instance().validate(token, sess)) {
        error(client, CMD::UPDATE_USER, "Authentication required");
        return;
    }
    
    std::string sql = "UPDATE users SET ";
    bool hasUpdate = false;
    
    try {
        if(data.contains("phone")) {
            std::string newPhone = data["phone"].get<std::string>();
            if (!validPhone(newPhone)) {
                error(client, CMD::UPDATE_USER, "Invalid phone number");
                return;
            }
            
            // 检查手机号唯一性
            auto exist = DatabaseManager::instance().query(
                "SELECT id FROM users WHERE phone='" + DatabaseManager::instance().escape(newPhone) + 
                "' AND id<>" + std::to_string(sess.userId)
            );
            if(!exist.empty()) {
                error(client, CMD::UPDATE_USER, "Phone already in use");
                return;
            }
            sql += "phone='" + DatabaseManager::instance().escape(newPhone) + "',";
            hasUpdate = true;
        }
        
        if(data.contains("plate_number")) {
            std::string newPlate = data["plate_number"].get<std::string>();
            if (!validPlateNumber(newPlate)) {
                error(client, CMD::UPDATE_USER, "Invalid plate number");
                return;
            }
            sql += "plate_number='" + DatabaseManager::instance().escape(newPlate) + "',";
            hasUpdate = true;
        }
        
        if(!hasUpdate) {
            error(client, CMD::UPDATE_USER, "No valid fields to update");
            return;
        }
        
        sql.pop_back();
        sql += " WHERE id=" + std::to_string(sess.userId);
        
        DatabaseManager::instance().execute(sql);
        Logger::instance().log(INFO, "User info updated: " + std::to_string(sess.userId));
        response(client, CMD::UPDATE_USER, {{"message", "Updated successfully"}}, token);
        
    } catch (const std::exception& e) {
        Logger::instance().log(ERROR, "Update user error: " + std::string(e.what()));
        error(client, CMD::UPDATE_USER, "Update failed");
    }
}

// ================= 管理员功能 =================

void handleUserList(SOCKET client, const json& data, const std::string& token) {
    Session sess;
    if(!SessionManager::instance().validate(token, sess) || (sess.role != "admin" && sess.role != "operator")) {
        error(client, CMD::USER_LIST, "Permission denied");
        return;
    }
    
    // 分页参数验证
    int page = std::max(1, data.value("page", 1));
    int size = std::min(MAX_PAGE_SIZE, std::max(1, data.value("size", DEFAULT_PAGE_SIZE)));
    std::string keyword = data.value("keyword", "");
    std::string role = data.value("role", "");
    std::string status = data.value("status", "");
    
    try {
        std::string where = "WHERE 1=1";
        if(!keyword.empty()) {
            where += " AND (username LIKE '%" + DatabaseManager::instance().escape(keyword) + 
                     "%' OR phone LIKE '%" + DatabaseManager::instance().escape(keyword) + "%')";
        }
        if(!role.empty()) where += " AND role='" + DatabaseManager::instance().escape(role) + "'";
        if(!status.empty()) where += " AND status='" + DatabaseManager::instance().escape(status) + "'";
        
        auto totalRes = DatabaseManager::instance().query("SELECT COUNT(*) as cnt FROM users " + where);
        int total = totalRes.empty() ? 0 : std::stoi(totalRes[0]["cnt"]);
        int offset = (page-1)*size;
        
        auto rows = DatabaseManager::instance().query(
            "SELECT id, username, role, phone, plate_number, balance, status FROM users " +
            where + " ORDER BY id LIMIT " + std::to_string(size) + " OFFSET " + std::to_string(offset)
        );
        
        json resp;
        resp["total"] = total;
        resp["page"] = page;
        resp["size"] = size;
        resp["list"] = json::array();
        for(auto& r : rows) resp["list"].push_back(r);
        
        response(client, CMD::USER_LIST, resp, token);
        
    } catch (const std::exception& e) {
        Logger::instance().log(ERROR, "Get user list error: " + std::string(e.what()));
        error(client, CMD::USER_LIST, "Failed to get user list");
    }
}

void handleUserEdit(SOCKET client, const json& data, const std::string& token) {
    Session sess;
    if(!SessionManager::instance().validate(token, sess) || (sess.role != "admin" && sess.role != "operator")) {
        error(client, CMD::USER_EDIT, "Permission denied");
        return;
    }
    
    int userId = data.value("user_id", 0);
    if(userId <= 0) {
        error(client, CMD::USER_EDIT, "Valid user_id required");
        return;
    }
    
    bool isAdmin = (sess.role == "admin");
    std::string sql = "UPDATE users SET ";
    bool changed = false;
    
    try {
        if(data.contains("status")) {
            std::string newStatus = data["status"].get<std::string>();
            if (newStatus == "active" || newStatus == "disabled" || newStatus == "blacklisted") {
                sql += "status='" + DatabaseManager::instance().escape(newStatus) + "',";
                changed = true;
            }
        }
        
        if(data.contains("role") && isAdmin) {
            std::string newRole = data["role"].get<std::string>();
            if (newRole == "driver" || newRole == "operator" || newRole == "admin") {
                sql += "role='" + DatabaseManager::instance().escape(newRole) + "',";
                changed = true;
            }
        }
        
        if(data.contains("new_password")) {
            std::string npwd = data["new_password"].get<std::string>();
            if(!strongPassword(npwd)) {
                error(client, CMD::USER_EDIT, "New password too weak");
                return;
            }
            sql += "password='" + CryptoUtils::sha256(npwd) + "',";
            changed = true;
        }
        
        if(!changed) {
            error(client, CMD::USER_EDIT, "No valid changes");
            return;
        }
        
        sql.pop_back();
        sql += " WHERE id=" + std::to_string(userId);
        
        DatabaseManager::instance().execute(sql);
        Logger::instance().log(INFO, "User edited: " + std::to_string(userId) + " by " + std::to_string(sess.userId));
        response(client, CMD::USER_EDIT, {{"message", "User updated successfully"}}, token);
        
    } catch (const std::exception& e) {
        Logger::instance().log(ERROR, "Edit user error: " + std::string(e.what()));
        error(client, CMD::USER_EDIT, "Update failed");
    }
}

// ================= 车辆管理 =================

void handleVehicleAdd(SOCKET client, const json& data, const std::string& token) {
    Session sess;
    if(!SessionManager::instance().validate(token, sess)) {
        error(client, CMD::VEHICLE_ADD, "Authentication required");
        return;
    }
    
    std::string plate = data.value("plate_number", "");
    std::string brand = data.value("brand", "");
    std::string model = data.value("model", "");
    std::string vin = data.value("vin", "");
    
    if(plate.empty()) {
        error(client, CMD::VEHICLE_ADD, "Plate number required");
        return;
    }
    
    if(!validPlateNumber(plate)) {
        error(client, CMD::VEHICLE_ADD, "Invalid plate number format");
        return;
    }
    
    try {
        // 检查车牌是否已关联该用户
        auto exist = DatabaseManager::instance().query(
            "SELECT id FROM vehicles WHERE plate_number='" + DatabaseManager::instance().escape(plate) + 
            "' AND user_id=" + std::to_string(sess.userId)
        );
        if(!exist.empty()) {
            error(client, CMD::VEHICLE_ADD, "Vehicle plate already added");
            return;
        }
        
        DatabaseManager::instance().execute(
            "INSERT INTO vehicles (user_id, plate_number, brand, model, vin) VALUES (" +
            std::to_string(sess.userId) + ",'" + DatabaseManager::instance().escape(plate) + "','" +
            DatabaseManager::instance().escape(brand) + "','" + DatabaseManager::instance().escape(model) + 
            "','" + DatabaseManager::instance().escape(vin) + "')"
        );
        
        Logger::instance().log(INFO, "Vehicle added for user: " + std::to_string(sess.userId));
        response(client, CMD::VEHICLE_ADD, {{"message", "Vehicle added successfully"}}, token);
        
    } catch (const std::exception& e) {
        Logger::instance().log(ERROR, "Add vehicle error: " + std::string(e.what()));
        error(client, CMD::VEHICLE_ADD, "Failed to add vehicle");
    }
}

void handleVehicleList(SOCKET client, const json& data, const std::string& token) {
    Session sess;
    if(!SessionManager::instance().validate(token, sess)) {
        error(client, CMD::VEHICLE_LIST, "Authentication required");
        return;
    }
    
    try {
        auto rows = DatabaseManager::instance().query(
            "SELECT * FROM vehicles WHERE user_id=" + std::to_string(sess.userId)
        );
        json resp = json::array();
        for(auto& r : rows) resp.push_back(r);
        response(client, CMD::VEHICLE_LIST, resp, token);
        
    } catch (const std::exception& e) {
        Logger::instance().log(ERROR, "Get vehicle list error: " + std::string(e.what()));
        error(client, CMD::VEHICLE_LIST, "Failed to get vehicle list");
    }
}

void handleVehicleDelete(SOCKET client, const json& data, const std::string& token) {
    Session sess;
    if(!SessionManager::instance().validate(token, sess)) {
        error(client, CMD::VEHICLE_DEL, "Authentication required");
        return;
    }
    
    int vehicleId = data.value("id", 0);
    if(vehicleId <= 0) {
        error(client, CMD::VEHICLE_DEL, "Valid vehicle id required");
        return;
    }
    
    try {
        auto chk = DatabaseManager::instance().query(
            "SELECT id FROM vehicles WHERE id=" + std::to_string(vehicleId) + 
            " AND user_id=" + std::to_string(sess.userId)
        );
        if(chk.empty()) {
            error(client, CMD::VEHICLE_DEL, "Vehicle not found");
            return;
        }
        
        DatabaseManager::instance().execute(
            "DELETE FROM vehicles WHERE id=" + std::to_string(vehicleId)
        );
        
        Logger::instance().log(INFO, "Vehicle deleted: " + std::to_string(vehicleId));
        response(client, CMD::VEHICLE_DEL, {{"message", "Vehicle deleted successfully"}}, token);
        
    } catch (const std::exception& e) {
        Logger::instance().log(ERROR, "Delete vehicle error: " + std::string(e.what()));
        error(client, CMD::VEHICLE_DEL, "Failed to delete vehicle");
    }
}
