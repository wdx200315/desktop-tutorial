#include "PaymentService.h"
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

// 统一分发
void handlePaymentDispatch(SOCKET client, const json& data, const std::string& token) {
    std::string action = data.value("action", "create");
    if(action == "status") handlePaymentStatus(client, data, token);
    else handlePaymentCreate(client, data, token);
}

void handlePaymentCreate(SOCKET client, const json& data, const std::string& token) {
    Session sess;
    if(!SessionManager::instance().validate(token, sess)) {
        error(client, CMD::PAYMENT, "Authentication required");
        return;
    }
    int orderId = data.value("order_id", 0);
    double amount = data.value("amount", 0.0);
    if(orderId <= 0 || amount <= 0) {
        error(client, CMD::PAYMENT, "Invalid order_id or amount");
        return;
    }
    auto ord = DatabaseManager::instance().query("SELECT user_id, status FROM charging_orders WHERE id=" + std::to_string(orderId));
    if(ord.empty() || std::stoi(ord[0]["user_id"]) != sess.userId) {
        error(client, CMD::PAYMENT, "Order not found or permission denied");
        return;
    }
    DatabaseManager::instance().execute(
        "INSERT INTO payments (order_id, user_id, amount, status) VALUES (" +
        std::to_string(orderId) + "," + std::to_string(sess.userId) + "," + std::to_string(amount) + ",'pending')"
    );
    int payId = DatabaseManager::instance().lastInsertId();
    DatabaseManager::instance().execute("UPDATE payments SET status='paid', paid_at=NOW() WHERE id=" + std::to_string(payId));
    DatabaseManager::instance().execute("UPDATE charging_orders SET status='completed', amount=" + std::to_string(amount) + " WHERE id=" + std::to_string(orderId));
    json resp;
    resp["payment_id"] = payId;
    resp["status"] = "paid";
    resp["message"] = "Payment successful (simulated)";
    response(client, CMD::PAYMENT, resp, token);
}

void handlePaymentStatus(SOCKET client, const json& data, const std::string& token) {
    Session sess;
    if(!SessionManager::instance().validate(token, sess)) {
        error(client, CMD::PAYMENT, "Authentication required");
        return;
    }
    int paymentId = data.value("payment_id", 0);
    auto rows = DatabaseManager::instance().query("SELECT * FROM payments WHERE id=" + std::to_string(paymentId));
    if(rows.empty()) { error(client, CMD::PAYMENT, "Payment not found"); return; }
    response(client, CMD::PAYMENT, rows[0], token);
}
