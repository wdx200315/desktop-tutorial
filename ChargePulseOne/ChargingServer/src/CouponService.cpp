#include "CouponService.h"
#include "Logger.h"
#include "SessionManager.h"
#include "DatabaseManager.h"

#ifdef _WIN32
    #include <winsock2.h>
#else
    #include <sys/socket.h>
    #include <unistd.h>
    using SOCKET = int;
#endif

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

// ---------- 管理员接口 ----------
void handleCouponList(SOCKET client, const json& data, const std::string& token) {
    Session sess;
    if(!SessionManager::instance().validate(token, sess) || (sess.role != "admin" && sess.role != "operator")) {
        error(client, CMD::COUPON_LIST, "Permission denied"); return;
    }
    auto rows = DatabaseManager::instance().query("SELECT * FROM coupons ORDER BY id DESC");
    json resp = json::array();
    for(auto& r: rows) resp.push_back(r);
    response(client, CMD::COUPON_LIST, resp, token);
}

void handleCouponAdd(SOCKET client, const json& data, const std::string& token) {
    Session sess;
    if(!SessionManager::instance().validate(token, sess) || sess.role != "admin") {
        error(client, CMD::COUPON_ADD, "Permission denied"); return;
    }
    std::string code = data.value("code", "");
    std::string type = data.value("type", "fixed");
    double value = data.value("value", 5.0);
    double minOrder = data.value("min_order", 0);
    std::string validFrom = data.value("valid_from", "");
    std::string validTo = data.value("valid_to", "");
    int quantity = data.value("quantity", 100);
    if(code.empty() || validFrom.empty() || validTo.empty()) {
        error(client, CMD::COUPON_ADD, "code, valid_from, valid_to required");
        return;
    }
    DatabaseManager::instance().execute(
        "INSERT INTO coupons (code, type, value, min_order_amount, valid_from, valid_to, quantity) VALUES ('" +
        code + "','" + type + "'," + std::to_string(value) + "," + std::to_string(minOrder) + ",'" +
        validFrom + "','" + validTo + "'," + std::to_string(quantity) + ")"
    );
    response(client, CMD::COUPON_ADD, {{"message","Coupon added"}}, token);
}

void handleCouponEdit(SOCKET client, const json& data, const std::string& token) {
    Session sess;
    if(!SessionManager::instance().validate(token, sess) || sess.role != "admin") {
        error(client, CMD::COUPON_EDIT, "Permission denied"); return;
    }
    int id = data.value("id", 0);
    std::string sql = "UPDATE coupons SET ";
    if(data.contains("value")) sql += "value=" + std::to_string(data["value"].get<double>()) + ",";
    if(data.contains("min_order_amount")) sql += "min_order_amount=" + std::to_string(data["min_order_amount"].get<double>()) + ",";
    if(data.contains("valid_to")) sql += "valid_to='" + data["valid_to"].get<std::string>() + "',";
    if(data.contains("quantity")) sql += "quantity=" + std::to_string(data["quantity"].get<int>()) + ",";
    if(sql.back() == ',') sql.pop_back();
    sql += " WHERE id=" + std::to_string(id);
    DatabaseManager::instance().execute(sql);
    response(client, CMD::COUPON_EDIT, {{"message","Updated"}}, token);
}

void handleCouponDelete(SOCKET client, const json& data, const std::string& token) {
    Session sess;
    if(!SessionManager::instance().validate(token, sess) || sess.role != "admin") {
        error(client, CMD::COUPON_DELETE, "Permission denied"); return;
    }
    int id = data.value("id", 0);
    DatabaseManager::instance().execute("DELETE FROM coupons WHERE id=" + std::to_string(id));
    response(client, CMD::COUPON_DELETE, {{"message","Deleted"}}, token);
}

// ---------- 用户接口 ----------
void handleCouponClaim(SOCKET client, const json& data, const std::string& token) {
    Session sess;
    if(!SessionManager::instance().validate(token, sess)) {
        error(client, CMD::COUPON_CLAIM, "Auth required"); return;
    }
    int couponId = data.value("coupon_id", 0);
    auto cp = DatabaseManager::instance().query("SELECT * FROM coupons WHERE id=" + std::to_string(couponId) + " AND status='active'");
    if(cp.empty()) { error(client, CMD::COUPON_CLAIM, "Coupon not available"); return; }
    // 检查是否已领取
    auto owned = DatabaseManager::instance().query(
        "SELECT id FROM user_coupons WHERE user_id=" + std::to_string(sess.userId) + " AND coupon_id=" + std::to_string(couponId)
    );
    if(!owned.empty()) { error(client, CMD::COUPON_CLAIM, "Already claimed"); return; }
    // 检查库存
    int usedCount = std::stoi(DatabaseManager::instance().query(
        "SELECT COUNT(*) as cnt FROM user_coupons WHERE coupon_id=" + std::to_string(couponId)
    )[0]["cnt"]);
    int quantity = std::stoi(cp[0]["quantity"]);
    if(usedCount >= quantity) { error(client, CMD::COUPON_CLAIM, "Out of stock"); return; }
    DatabaseManager::instance().execute(
        "INSERT INTO user_coupons (user_id, coupon_id) VALUES (" + std::to_string(sess.userId) + "," + std::to_string(couponId) + ")"
    );
    response(client, CMD::COUPON_CLAIM, {{"message","Claimed"}}, token);
}

void handleCouponUserList(SOCKET client, const json& data, const std::string& token) {
    Session sess;
    if(!SessionManager::instance().validate(token, sess)) {
        error(client, CMD::COUPON_USER_LIST, "Auth required"); return;
    }
    auto rows = DatabaseManager::instance().query(
        "SELECT uc.id, c.code, c.type, c.value, c.min_order_amount, c.valid_from, c.valid_to, uc.status, uc.used_at "
        "FROM user_coupons uc JOIN coupons c ON uc.coupon_id=c.id WHERE uc.user_id=" + std::to_string(sess.userId)
    );
    json resp = json::array();
    for(auto& r: rows) resp.push_back(r);
    response(client, CMD::COUPON_USER_LIST, resp, token);
}

void handleCouponUse(SOCKET client, const json& data, const std::string& token) {
    // 支付时使用券（需传入 user_coupon_id 和订单金额）
    Session sess;
    if(!SessionManager::instance().validate(token, sess)) {
        error(client, CMD::COUPON_USE, "Auth required"); return;
    }
    int userCouponId = data.value("user_coupon_id", 0);
    double orderAmount = data.value("order_amount", 0.0);
    auto uc = DatabaseManager::instance().query("SELECT * FROM user_coupons WHERE id=" + std::to_string(userCouponId) + " AND user_id=" + std::to_string(sess.userId));
    if(uc.empty() || uc[0]["status"] != "unused") { error(client, CMD::COUPON_USE, "Coupon not available"); return; }
    // 可在此校验券是否满足满减条件（需要查询 coupons 表）
    DatabaseManager::instance().execute("UPDATE user_coupons SET status='used', used_at=NOW() WHERE id=" + std::to_string(userCouponId));
    // 返回减免金额（实际需计算，这里简化为券面值）
    double discount = 0.0;
    response(client, CMD::COUPON_USE, {{"discount", discount}}, token);
}
