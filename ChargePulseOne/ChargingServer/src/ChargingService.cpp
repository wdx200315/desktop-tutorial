#include "ChargingService.h"
#include "Logger.h"
#include "SessionManager.h"
#include "DatabaseManager.h"
#include "TcpServer.h"
#include "json.hpp"
#include <unordered_map>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <random>

#ifdef _WIN32
    #include <winsock2.h>
#else
    #include <sys/socket.h>
    #include <unistd.h>
    using SOCKET = int;
#endif

// 充电模拟器实例
std::unordered_map<int, ChargeSimulator> g_chargeSimulators;
std::unordered_map<int, SOCKET> g_orderClients;
std::unordered_map<int, SOCKET> g_userClients;

static void sendToClient(SOCKET client, const std::string& data) {
    if (client > 0) {
        send(client, (data + "\n").c_str(), data.size() + 1, 0);
    }
}

static void response(SOCKET client, const std::string& cmd, const json& data, const std::string& token = "") {
    json msg; msg["cmd"] = cmd; msg["status"] = "ok"; msg["data"] = data;
    if (!token.empty()) msg["token"] = token;
    std::string raw = msg.dump();
    send(client, (raw + "\n").c_str(), raw.size() + 1, 0);
}

static void error(SOCKET client, const std::string& cmd, const std::string& msg) {
    json m; m["cmd"] = cmd; m["status"] = "error"; m["message"] = msg;
    std::string raw = m.dump();
    send(client, (raw + "\n").c_str(), raw.size() + 1, 0);
}

void updateChargeSimulation() {
    auto it = g_chargeSimulators.begin();
    while (it != g_chargeSimulators.end()) {
        ChargeSimulator& sim = it->second;
        if (sim.running) {
            double currentTime = clock() / (double)CLOCKS_PER_SEC;
            double elapsed = currentTime - sim.lastUpdateTime;
            sim.lastUpdateTime = currentTime;
            
            // 模拟充电过程 (每更新周期增加电量)
            double deltaHours = elapsed / 3600.0;
            double deltaEnergy = sim.power * deltaHours; // kWh
            sim.currentEnergy = sim.initialEnergy + deltaEnergy;
            
            // 模拟电压波动 (360-400V)
            sim.voltage = 380.0 + (rand() % 40 - 20) + sin(currentTime / 10.0) * 5;
            if (sim.voltage < 350) sim.voltage = 350;
            if (sim.voltage > 410) sim.voltage = 410;
            
            // 模拟电流波动
            sim.current = (sim.power * 1000.0 / sim.voltage) + (rand() % 20 - 10) * 0.5;
            if (sim.current < 0) sim.current = abs(sim.current);
            
            // 模拟温度 (考虑充电功率和环境)
            double tempDelta = (sim.power / 60.0) * (1 + (rand() % 10) / 10.0);
            sim.temperature += tempDelta * deltaHours;
            if (sim.temperature < 25) sim.temperature = 25;
            if (sim.temperature > 65) sim.temperature = 65; // 最高65度触发保护
            
            // 模拟SOC增长
            double socIncrement = (sim.power / 60.0) * deltaHours; // 假设60kWh电池
            sim.soc += socIncrement;
            if (sim.soc > 100) sim.soc = 100;
            
            // 检查是否达到目标SOC
            if (sim.targetSoc > 0 && sim.soc >= sim.targetSoc) {
                sim.running = false;
                Logger::instance().log(INFO, "Charge completed: target SOC reached");
            }
            
            // 计算费用 (考虑峰谷电价)
            double currentPrice = getCurrentPrice();
            double energyFee = sim.currentEnergy * currentPrice;
            double serviceFee = sim.currentEnergy * sim.serviceFee;
            double totalFee = energyFee + serviceFee;
            
            // 构造监控数据
            json monitorData;
            monitorData["order_id"] = sim.orderId;
            monitorData["charger_id"] = sim.chargerId;
            monitorData["charger_serial"] = sim.chargerSerial;
            monitorData["energy"] = round(sim.currentEnergy * 100) / 100;
            monitorData["power"] = round(sim.power * 10) / 10;
            monitorData["voltage"] = round(sim.voltage * 10) / 10;
            monitorData["current"] = round(sim.current * 10) / 10;
            monitorData["temperature"] = round(sim.temperature * 10) / 10;
            monitorData["soc"] = round(sim.soc * 10) / 10;
            monitorData["progress"] = round(sim.getProgress() * 10) / 10;
            monitorData["fee"] = round(totalFee * 100) / 100;
            monitorData["energy_fee"] = round(energyFee * 100) / 100;
            monitorData["service_fee"] = round(serviceFee * 100) / 100;
            monitorData["current_price"] = currentPrice;
            monitorData["price_period"] = getCurrentPeriodName();
            monitorData["duration"] = sim.getDuration();
            monitorData["mode"] = sim.mode;
            monitorData["status"] = sim.running ? "charging" : "completed";
            
            // 发送给客户端
            auto clientIt = g_orderClients.find(sim.orderId);
            if (clientIt != g_orderClients.end() && clientIt->second > 0) {
                json push;
                push["cmd"] = CMD::CHARGE_MONITOR;
                push["status"] = "ok";
                push["data"] = monitorData;
                sendToClient(clientIt->second, push.dump());
            }
            
            // 也发送给用户
            auto userIt = g_userClients.find(sim.userId);
            if (userIt != g_userClients.end() && userIt->second > 0) {
                json push;
                push["cmd"] = CMD::CHARGE_MONITOR;
                push["status"] = "ok";
                push["data"] = monitorData;
                sendToClient(userIt->second, push.dump());
            }
            
            // 更新数据库
            std::string sql = "UPDATE charging_orders SET energy_kwh = " + 
                std::to_string(round(sim.currentEnergy * 100) / 100) +
                ", amount = " + std::to_string(round(totalFee * 100) / 100) +
                ", status = '" + (sim.running ? "charging" : "completed") + "'" +
                " WHERE id = " + std::to_string(sim.orderId);
            DatabaseManager::instance().execute(sql);
            
            // 如果充电完成，更新状态
            if (!sim.running) {
                DatabaseManager::instance().execute(
                    "UPDATE chargers SET status='online' WHERE id=" + std::to_string(sim.chargerId)
                );
                DatabaseManager::instance().execute(
                    "UPDATE charging_orders SET status='completed', end_time=NOW() WHERE id=" + std::to_string(sim.orderId)
                );
                g_orderClients.erase(sim.orderId);
                it = g_chargeSimulators.erase(it);
                continue;
            }
        }
        ++it;
    }
}

void handleStartCharge(SOCKET client, const json& data, const std::string& token, SOCKET serverClient) {
    Session sess;
    if (!SessionManager::instance().validate(token, sess)) {
        error(client, CMD::START_CHARGE, "Invalid session");
        return;
    }
    
    int chargerId = data.value("charger_id", 0);
    std::string mode = data.value("mode", "auto");
    double target = data.value("target", 100.0); // 默认100% SOC
    
    // 检查桩状态
    auto ch = DatabaseManager::instance().query(
        "SELECT status, power_kw, serial_number FROM chargers WHERE id=" + std::to_string(chargerId)
    );
    if (ch.empty()) {
        error(client, CMD::START_CHARGE, "Charger not found");
        return;
    }
    if (ch[0]["status"] != "online") {
        error(client, CMD::START_CHARGE, "Charger not available, current status: " + ch[0]["status"]);
        return;
    }
    
    // 获取费率
    auto rates = DatabaseManager::instance().query(
        "SELECT price_kwh, service_fee FROM rates LIMIT 1"
    );
    double feePerKwh = 0.6; // 默认
    double serviceFee = 0.1; // 默认服务费
    if (!rates.empty()) {
        feePerKwh = std::stod(rates[0]["price_kwh"]);
        serviceFee = std::stod(rates[0]["service_fee"]);
    }
    
    // 检查用户余额
    auto user = DatabaseManager::instance().query(
        "SELECT balance FROM users WHERE id=" + std::to_string(sess.userId)
    );
    if (user.empty() || std::stod(user[0]["balance"]) < 10.0) {
        error(client, CMD::START_CHARGE, "Insufficient balance, please recharge");
        return;
    }
    
    // 创建充电订单
    std::string sql = "INSERT INTO charging_orders (user_id, charger_id, mode, target_value, start_time, status) VALUES (" +
        std::to_string(sess.userId) + "," + std::to_string(chargerId) + ",'" + mode + "'," +
        std::to_string(target) + ",NOW(),'charging')";
    if (DatabaseManager::instance().execute(sql) <= 0) {
        error(client, CMD::START_CHARGE, "Failed to start charge");
        return;
    }
    int orderId = DatabaseManager::instance().lastInsertId();
    
    // 更新桩状态为充电中
    DatabaseManager::instance().execute(
        "UPDATE chargers SET status='charging' WHERE id=" + std::to_string(chargerId)
    );
    
    // 初始化充电模拟器
    ChargeSimulator sim;
    sim.orderId = orderId;
    sim.chargerId = chargerId;
    sim.userId = sess.userId;
    sim.initialEnergy = 0;
    sim.currentEnergy = 0;
    sim.power = std::stod(ch[0]["power_kw"]);
    sim.voltage = 380.0;
    sim.current = 0;
    sim.temperature = 30.0;
    sim.soc = 20.0; // 假设车辆初始SOC 20%
    sim.targetSoc = target;
    sim.feePerKwh = feePerKwh;
    sim.serviceFee = serviceFee;
    sim.startTime = clock() / (double)CLOCKS_PER_SEC;
    sim.lastUpdateTime = sim.startTime;
    sim.running = true;
    sim.mode = mode;
    sim.chargerSerial = ch[0]["serial_number"];
    
    g_chargeSimulators[orderId] = sim;
    g_orderClients[orderId] = client;
    g_userClients[sess.userId] = client;
    
    Logger::instance().log(INFO, "Charge started: orderId=" + std::to_string(orderId) +
                           ", userId=" + std::to_string(sess.userId) +
                           ", chargerId=" + std::to_string(chargerId) +
                           ", power=" + std::to_string(sim.power) + "kW" +
                           ", currentPrice=" + std::to_string(getCurrentPrice()) + " (" + getCurrentPeriodName() + ")");
    
    json resp;
    resp["order_id"] = orderId;
    resp["status"] = "charging";
    resp["power"] = sim.power;
    resp["voltage"] = sim.voltage;
    resp["fee_per_kwh"] = feePerKwh;
    resp["service_fee"] = serviceFee;
    resp["current_price"] = getCurrentPrice();
    resp["price_period"] = getCurrentPeriodName();
    resp["initial_soc"] = sim.soc;
    resp["target_soc"] = sim.targetSoc;
    resp["message"] = "Charging started successfully";
    response(client, CMD::START_CHARGE, resp, token);
}

void handleStopCharge(SOCKET client, const json& data, const std::string& token) {
    Session sess;
    if (!SessionManager::instance().validate(token, sess)) {
        error(client, CMD::STOP_CHARGE, "Invalid session");
        return;
    }
    
    int orderId = data.value("order_id", 0);
    
    auto order = DatabaseManager::instance().query(
        "SELECT charger_id, status, energy_kwh, amount FROM charging_orders WHERE id=" + std::to_string(orderId)
    );
    if (order.empty()) {
        error(client, CMD::STOP_CHARGE, "Order not found");
        return;
    }
    if (order[0]["status"] != "charging") {
        error(client, CMD::STOP_CHARGE, "Order is not charging");
        return;
    }
    
    int chargerId = std::stoi(order[0]["charger_id"]);
    double finalEnergy = std::stod(order[0]["energy_kwh"]);
    double finalAmount = std::stod(order[0]["amount"]);
    
    // 停止模拟器
    auto simIt = g_chargeSimulators.find(orderId);
    if (simIt != g_chargeSimulators.end()) {
        finalEnergy = simIt->second.currentEnergy;
        finalAmount = simIt->second.getCurrentFee();
        simIt->second.running = false;
        g_chargeSimulators.erase(simIt);
    }
    g_orderClients.erase(orderId);
    
    // 结束订单
    DatabaseManager::instance().execute(
        "UPDATE charging_orders SET status='completed', end_time=NOW(), " +
        "energy_kwh=" + std::to_string(round(finalEnergy * 100) / 100) +
        ", amount=" + std::to_string(round(finalAmount * 100) / 100) +
        " WHERE id=" + std::to_string(orderId)
    );
    
    // 恢复桩状态
    DatabaseManager::instance().execute(
        "UPDATE chargers SET status='online' WHERE id=" + std::to_string(chargerId)
    );
    
    // 扣除用户余额
    DatabaseManager::instance().execute(
        "UPDATE users SET balance = balance - " + std::to_string(round(finalAmount * 100) / 100) +
        " WHERE id = " + std::to_string(sess.userId)
    );
    
    Logger::instance().log(INFO, "Charge stopped: orderId=" + std::to_string(orderId) +
                           ", energy=" + std::to_string(finalEnergy) + "kWh" +
                           ", amount=¥" + std::to_string(finalAmount));
    
    json resp;
    resp["message"] = "Charge stopped successfully";
    resp["order_id"] = orderId;
    resp["final_energy"] = round(finalEnergy * 100) / 100;
    resp["final_amount"] = round(finalAmount * 100) / 100;
    resp["duration"] = simIt != g_chargeSimulators.end() ? 0 : 0;
    response(client, CMD::STOP_CHARGE, resp, token);
}

void handleChargeMonitor(SOCKET client, const json& data, const std::string& token) {
    int orderId = data.value("order_id", 0);
    
    auto order = DatabaseManager::instance().query(
        "SELECT co.*, c.power_kw, c.serial_number FROM charging_orders co "
        "JOIN chargers c ON co.charger_id = c.id "
        "WHERE co.id=" + std::to_string(orderId)
    );
    if (order.empty()) {
        error(client, CMD::CHARGE_MONITOR, "Order not found");
        return;
    }
    
    // 如果有模拟数据，返回实时数据
    auto simIt = g_chargeSimulators.find(orderId);
    if (simIt != g_chargeSimulators.end() && simIt->second.running) {
        ChargeSimulator& sim = simIt->second;
        double totalFee = sim.getCurrentFee();
        
        json resp;
        resp["order_id"] = orderId;
        resp["charger_id"] = sim.chargerId;
        resp["charger_serial"] = sim.chargerSerial;
        resp["status"] = "charging";
        resp["energy"] = round(sim.currentEnergy * 100) / 100;
        resp["power"] = round(sim.power * 10) / 10;
        resp["voltage"] = round(sim.voltage * 10) / 10;
        resp["current"] = round(sim.current * 10) / 10;
        resp["temperature"] = round(sim.temperature * 10) / 10;
        resp["soc"] = round(sim.soc * 10) / 10;
        resp["progress"] = round(sim.getProgress() * 10) / 10;
        resp["fee"] = round(totalFee * 100) / 100;
        resp["current_price"] = getCurrentPrice();
        resp["price_period"] = getCurrentPeriodName();
        resp["duration"] = sim.getDuration();
        resp["mode"] = sim.mode;
        response(client, CMD::CHARGE_MONITOR, resp, token);
    } else {
        // 返回数据库中的最终数据
        json resp;
        resp["order_id"] = orderId;
        resp["charger_id"] = std::stoi(order[0]["charger_id"]);
        resp["charger_serial"] = order[0]["serial_number"];
        resp["status"] = order[0]["status"];
        resp["energy"] = std::stod(order[0]["energy_kwh"]);
        resp["power"] = std::stod(order[0]["power_kw"]);
        resp["fee"] = std::stod(order[0]["amount"]);
        resp["current_price"] = getCurrentPrice();
        resp["price_period"] = getCurrentPeriodName();
        resp["start_time"] = order[0]["start_time"];
        response(client, CMD::CHARGE_MONITOR, resp, token);
    }
}
