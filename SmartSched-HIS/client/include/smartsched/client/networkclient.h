/**
 * @file networkclient.h
 * @brief 网络客户端
 * 
 * 患者视角: 简单易用的网络接口
 * 医生视角: 高效的通信机制
 */

#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>
#include <QTimer>
#include <QMutex>
#include <memory>
#include "../../common/include/smartsched/protocol/message.h"
#include "../../common/include/smartsched/utils/json_helper.h"

namespace smartsched {
namespace client {

// =============================================================================
// 网络客户端配置
// =============================================================================
struct NetworkClientConfig {
    QString host = "localhost";
    quint16 port = 8888;
    int connectionTimeout = 10;  // seconds
    int commandTimeout = 30;    // seconds
    int heartbeatInterval = 30; // seconds
    bool autoReconnect = true;
    int maxReconnectAttempts = 5;
};

// =============================================================================
// 连接状态
// =============================================================================
enum class ConnectionState {
    Disconnected,
    Connecting,
    Connected,
    Authenticating,
    Authenticated,
    Error
};

// =============================================================================
// 网络客户端
// =============================================================================
class NetworkClient : public QObject {
    Q_OBJECT
    
public:
    explicit NetworkClient(QObject* parent = nullptr);
    ~NetworkClient() override;
    
    // 配置
    void setConfig(const NetworkClientConfig& config);
    NetworkClientConfig config() const { return config_; }
    
    // 连接管理
    bool connectToServer();
    void disconnectFromServer();
    
    // 连接状态
    ConnectionState state() const { return state_; }
    bool isConnected() const { return state_ == ConnectionState::Connected || 
                                      state_ == ConnectionState::Authenticated; }
    
    // 发送请求
    void sendRequest(int cmd, const utils::JsonValue& params, 
                    std::function<void(const utils::JsonValue&)> callback);
    
    // 便捷方法
    void getDepartmentList(std::function<void(const utils::JsonValue&)> callback);
    void registerPatient(int patient_id, int dept_id, int doctor_id,
                         std::function<void(const utils::JsonValue&)> callback);
    void getQueueStatus(int patient_id, std::function<void(const utils::JsonValue&)> callback);
    void startConsultation(int doctor_id, int patient_id,
                          std::function<void(const utils::JsonValue&)> callback);
    void endConsultation(int doctor_id, int patient_id,
                         std::function<void(const utils::JsonValue&)> callback);
    
signals:
    void connected();
    void disconnected();
    void connectionError(const QString& error);
    void stateChanged(ConnectionState state);
    void messageReceived(const utils::JsonValue& message);
    void queueUpdated(const utils::JsonValue& queue);
    void patientCalled(const QString& queue_number);
    
private slots:
    void onReadyRead();
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError error);
    void onHeartbeat();
    void onReconnect();
    
private:
    void processIncomingData();
    void sendHeartbeat();
    void scheduleReconnect();
    uint32_t generateSequence();
    
    NetworkClientConfig config_;
    QTcpSocket* socket_;
    ConnectionState state_;
    
    // 数据缓冲区
    QByteArray read_buffer_;
    
    // 序列号
    QMutex sequence_mutex_;
    uint32_t sequence_;
    
    // 回调管理
    QMap<uint32_t, std::function<void(const utils::JsonValue&)>> callbacks_;
    QMutex callbacks_mutex_;
    
    // 重连
    int reconnect_attempts_;
    QTimer* reconnect_timer_;
    QTimer* heartbeat_timer_;
    
    // 统计
    qint64 bytes_sent_;
    qint64 bytes_received_;
};

// =============================================================================
// 内联实现
// =============================================================================

inline NetworkClient::NetworkClient(QObject* parent)
    : QObject(parent)
    , socket_(nullptr)
    , state_(ConnectionState::Disconnected)
    , sequence_(0)
    , reconnect_attempts_(0)
    , bytes_sent_(0)
    , bytes_received_(0)
{
    socket_ = new QTcpSocket(this);
    
    connect(socket_, &QTcpSocket::connected, this, &NetworkClient::onConnected);
    connect(socket_, &QTcpSocket::disconnected, this, &NetworkClient::onDisconnected);
    connect(socket_, &QTcpSocket::readyRead, this, &NetworkClient::onReadyRead);
    connect(QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
            this, &NetworkClient::onError);
    
    // 心跳定时器
    heartbeat_timer_ = new QTimer(this);
    connect(heartbeat_timer_, &QTimer::timeout, this, &NetworkClient::sendHeartbeat);
    
    // 重连定时器
    reconnect_timer_ = new QTimer(this);
    reconnect_timer_->setSingleShot(true);
    connect(reconnect_timer_, &QTimer::timeout, this, &NetworkClient::onReconnect);
}

inline NetworkClient::~NetworkClient() {
    disconnectFromServer();
}

inline void NetworkClient::setConfig(const NetworkClientConfig& config) {
    config_ = config;
}

inline bool NetworkClient::connectToServer() {
    if (isConnected()) return true;
    
    state_ = ConnectionState::Connecting;
    emit stateChanged(state_);
    
    socket_->connectToHost(config_.host, config_.port);
    return socket_->waitForConnected(config_.connectionTimeout * 1000);
}

inline void NetworkClient::disconnectFromServer() {
    heartbeat_timer_->stop();
    reconnect_timer_->stop();
    socket_->disconnectFromHost();
    state_ = ConnectionState::Disconnected;
    emit stateChanged(state_);
}

inline void NetworkClient::sendRequest(int cmd, const utils::JsonValue& params,
                                       std::function<void(const utils::JsonValue&)> callback) {
    if (!isConnected()) {
        if (callback) {
            utils::JsonValue error = utils::JsonValue::object();
            error["success"] = false;
            error["error"] = "Not connected";
            callback(error);
        }
        return;
    }
    
    uint32_t seq = generateSequence();
    
    // 保存回调
    if (callback) {
        QMutexLocker lock(&callbacks_mutex_);
        callbacks_[seq] = callback;
    }
    
    // 构建消息
    utils::JsonValue message = utils::JsonValue::object();
    message["cmd"] = cmd;
    message["seq"] = static_cast<int>(seq);
    message["body"] = params;
    
    QString json = QString::fromStdString(message.serialize()) + "\n";
    QByteArray data = json.toUtf8();
    
    qint64 written = socket_->write(data);
    if (written > 0) {
        bytes_sent_ += written;
        socket_->flush();
    }
}

inline uint32_t NetworkClient::generateSequence() {
    QMutexLocker lock(&sequence_mutex_);
    return ++sequence_;
}

inline void NetworkClient::getDepartmentList(std::function<void(const utils::JsonValue&)> callback) {
    sendRequest(static_cast<int>(protocol::CommandID::DEPT_LIST), 
                utils::JsonValue::object(), callback);
}

inline void NetworkClient::registerPatient(int patient_id, int dept_id, int doctor_id,
                                            std::function<void(const utils::JsonValue&)> callback) {
    utils::JsonValue params = utils::JsonValue::object();
    params["patient_id"] = patient_id;
    params["dept_id"] = dept_id;
    params["doctor_id"] = doctor_id;
    sendRequest(static_cast<int>(protocol::CommandID::REGISTER), params, callback);
}

inline void NetworkClient::getQueueStatus(int patient_id, 
                                          std::function<void(const utils::JsonValue&)> callback) {
    utils::JsonValue params = utils::JsonValue::object();
    params["patient_id"] = patient_id;
    sendRequest(static_cast<int>(protocol::CommandID::QUEUE_STATUS), params, callback);
}

inline void NetworkClient::startConsultation(int doctor_id, int patient_id,
                                             std::function<void(const utils::JsonValue&)> callback) {
    utils::JsonValue params = utils::JsonValue::object();
    params["doctor_id"] = doctor_id;
    params["patient_id"] = patient_id;
    sendRequest(static_cast<int>(protocol::CommandID::CONSULT_START), params, callback);
}

inline void NetworkClient::endConsultation(int doctor_id, int patient_id,
                                           std::function<void(const utils::JsonValue&)> callback) {
    utils::JsonValue params = utils::JsonValue::object();
    params["doctor_id"] = doctor_id;
    params["patient_id"] = patient_id;
    sendRequest(static_cast<int>(protocol::CommandID::CONSULT_END), params, callback);
}

inline void NetworkClient::onConnected() {
    state_ = ConnectionState::Connected;
    emit stateChanged(state_);
    emit connected();
    
    reconnect_attempts_ = 0;
    
    // 启动心跳
    heartbeat_timer_->start(config_.heartbeatInterval * 1000);
}

inline void NetworkClient::onDisconnected() {
    heartbeat_timer_->stop();
    state_ = ConnectionState::Disconnected;
    emit stateChanged(state_);
    emit disconnected();
    
    // 清理回调
    {
        QMutexLocker lock(&callbacks_mutex_);
        callbacks_.clear();
    }
    
    // 自动重连
    if (config_.autoReconnect) {
        scheduleReconnect();
    }
}

inline void NetworkClient::onError(QAbstractSocket::SocketError error) {
    QString error_str = socket_->errorString();
    emit connectionError(error_str);
    
    if (state_ != ConnectionState::Disconnected) {
        state_ = ConnectionState::Error;
        emit stateChanged(state_);
    }
}

inline void NetworkClient::onReadyRead() {
    QByteArray data = socket_->readAll();
    bytes_received_ += data.size();
    read_buffer_.append(data);
    
    processIncomingData();
}

inline void NetworkClient::processIncomingData() {
    while (true) {
        int pos = read_buffer_.indexOf('\n');
        if (pos < 0) break;
        
        QByteArray message = read_buffer_.left(pos);
        read_buffer_.remove(0, pos + 1);
        
        if (message.isEmpty()) continue;
        
        try {
            auto json = utils::JsonValue::parse(QString::fromUtf8(message).toStdString());
            
            // 检查是否有对应的回调
            if (json.isObject() && json.has("seq")) {
                uint32_t seq = json["seq"].asInt();
                
                QMutexLocker lock(&callbacks_mutex_);
                if (callbacks_.contains(seq)) {
                    auto callback = callbacks_[seq];
                    callbacks_.remove(seq);
                    lock.unlock();
                    
                    if (callback) {
                        callback(json["data"]);
                    }
                    continue;
                }
            }
            
            // 广播消息
            emit messageReceived(json);
            
        } catch (const std::exception& e) {
            qWarning() << "Failed to parse message:" << e.what();
        }
    }
}

inline void NetworkClient::sendHeartbeat() {
    if (!isConnected()) return;
    
    utils::JsonValue heartbeat = utils::JsonValue::object();
    heartbeat["cmd"] = static_cast<int>(protocol::CommandID::PING);
    heartbeat["ts"] = QDateTime::currentMSecsSinceEpoch();
    
    QString json = QString::fromStdString(heartbeat.serialize()) + "\n";
    socket_->write(json.toUtf8());
    socket_->flush();
}

inline void NetworkClient::scheduleReconnect() {
    if (reconnect_attempts_ >= config_.maxReconnectAttempts) {
        qWarning() << "Max reconnect attempts reached";
        return;
    }
    
    reconnect_attempts_++;
    int delay = qMin(30000, 1000 * (1 << reconnect_attempts_));  // 指数退避
    
    qDebug() << "Scheduling reconnect in" << delay << "ms (attempt" << reconnect_attempts_ << ")";
    reconnect_timer_->start(delay);
}

inline void NetworkClient::onReconnect() {
    qDebug() << "Attempting to reconnect...";
    connectToServer();
}

} // namespace client
} // namespace smartsched
