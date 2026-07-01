#pragma once
#include "common.h"
#include <map>
#include <mutex>
#include <ctime>

struct Session {
    int userId;
    std::string username;
    std::string role;      // driver, operator, admin
    std::time_t loginTime;
    std::time_t lastActive;
};

class SessionManager {
public:
    static SessionManager& instance();
    std::string createSession(int userId, const std::string& username, const std::string& role);
    bool validate(const std::string& token, Session& sess);
    void updateActivity(const std::string& token);
    void remove(const std::string& token);
    void cleanExpired();
    int getActiveCount();
    int getExpireSeconds() { return 1800; } // 30分钟

private:
    SessionManager() = default;
    std::map<std::string, Session> sessions;
    std::mutex mtx;
    std::string generateToken();
};

