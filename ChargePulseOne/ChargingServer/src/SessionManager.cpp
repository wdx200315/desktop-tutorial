#include "SessionManager.h"
#include "Logger.h"

SessionManager& SessionManager::instance() {
    static SessionManager inst;
    return inst;
}

std::string SessionManager::generateToken() {
    return random_str(32);
}

std::string SessionManager::createSession(int userId, const std::string& username, const std::string& role) {
    std::lock_guard<std::mutex> lock(mtx);
    std::string token = generateToken();
    std::time_t now = std::time(nullptr);
    sessions[token] = {userId, username, role, now, now};
    Logger::instance().log(INFO, "Session created for user: " + username);
    return token;
}

bool SessionManager::validate(const std::string& token, Session& sess) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = sessions.find(token);
    if(it == sessions.end()) return false;
    // 检查是否过期（24小时总有效期）
    if(std::time(nullptr) - it->second.loginTime > 86400) {
        sessions.erase(it);
        return false;
    }
    // 检查30分钟无操作超时
    if(std::time(nullptr) - it->second.lastActive > getExpireSeconds()) {
        sessions.erase(it);
        return false;
    }
    sess = it->second;
    return true;
}

void SessionManager::updateActivity(const std::string& token) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = sessions.find(token);
    if(it != sessions.end()) {
        it->second.lastActive = std::time(nullptr);
    }
}

void SessionManager::remove(const std::string& token) {
    std::lock_guard<std::mutex> lock(mtx);
    sessions.erase(token);
}

void SessionManager::cleanExpired() {
    std::lock_guard<std::mutex> lock(mtx);
    std::time_t now = std::time(nullptr);
    for(auto it = sessions.begin(); it != sessions.end(); ) {
        if(now - it->second.loginTime > 86400 || now - it->second.lastActive > getExpireSeconds()) {
            it = sessions.erase(it);
        } else ++it;
    }
}

int SessionManager::getActiveCount() {
    std::lock_guard<std::mutex> lock(mtx);
    return (int)sessions.size();
}
