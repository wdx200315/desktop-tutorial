/**
 * @file tcpserver.cpp
 * @brief TCP服务器实现
 */

#include "smartsched/server/tcpserver.h"
#include <QTimer>
#include <QDateTime>

namespace smartsched {
namespace server {

// =============================================================================
// 静态成员初始化
// =============================================================================
qint64 TcpConnection::connection_counter_ = 0;
QMutex TcpConnection::counter_mutex_;

// =============================================================================
// TcpConnection实现
// =============================================================================

void TcpConnection::onReadyRead() {
    if (!socket_) return;
    
    QByteArray data = socket_->readAll();
    if (data.isEmpty()) return;
    
    bytes_received_ += data.size();
    read_buffer_.append(data);
    info_.lastActivityTime = QDateTime::currentMSecsSinceEpoch();
    
    // 处理接收到的数据
    processIncomingData();
    
    emit activity(connection_id_);
}

void TcpConnection::onDisconnected() {
    LOG_INFO("Connection disconnected: " + connection_id_);
    emit disconnected(connection_id_);
}

void TcpConnection::onError(QAbstractSocket::SocketError error) {
    QString error_str = QString("Socket error %1: %2")
        .arg(static_cast<int>(error))
        .arg(socket_ ? socket_->errorString() : "Unknown");
    
    LOG_ERROR("Connection error: " + connection_id_ + " - " + error_str);
    emit errorOccurred(connection_id_, error_str);
}

void TcpConnection::onBytesWritten(qint64 bytes) {
    Q_UNUSED(bytes);
    emit activity(connection_id_);
}

void TcpConnection::sendHeartbeat() {
    if (!isConnected() || !heartbeat_enabled_) return;
    
    // 检查心跳超时
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - info_.lastActivityTime > 90000) {  // 90秒超时
        LOG_WARN("Connection heartbeat timeout: " + connection_id_);
        disconnectClient();
        return;
    }
    
    // 发送心跳包
    QString heartbeat = "{\"cmd\":9001,\"ts\":" + QString::number(now) + "}\n";
    socket_->write(heartbeat.toUtf8());
}

void TcpConnection::processIncomingData() {
    // 按\n分割消息
    while (true) {
        int pos = read_buffer_.indexOf('\n');
        if (pos < 0) break;
        
        QByteArray message = read_buffer_.left(pos);
        read_buffer_.remove(0, pos + 1);
        
        if (message.isEmpty()) continue;
        
        // 检查消息大小
        if (message.size() > 1024 * 1024) {  // 1MB限制
            LOG_WARN("Message too large from " + connection_id_ + 
                    ": " + QString::number(message.size()) + " bytes");
            continue;
        }
        
        QString json_message = QString::fromUtf8(message);
        
        LOG_DEBUG("Received from " + connection_id_ + ": " + 
                 json_message.left(200));  // 限制日志长度
        
        emit messageReceived(connection_id_, json_message);
    }
}

// =============================================================================
// TcpServer实现
// =============================================================================

TcpServer::TcpServer(QObject* parent)
    : QObject(parent)
    , server_socket_(nullptr)
    , thread_pool_(nullptr)
    , message_handler_(nullptr)
    , is_running_(false)
    , listen_port_(0)
    , total_connections_(0)
    , total_bytes_in_(0)
    , total_bytes_out_(0)
{
    // 创建服务器socket
    server_socket_ = new QTcpServer(this);
    
    // 信号连接
    connect(server_socket_, &QTcpServer::newConnection,
            this, &TcpServer::onNewConnection);
    
    // 创建线程池
    thread_pool_ = new QThreadPool(this);
    thread_pool_->setObjectName("TcpServerThreadPool");
    
    // 心跳定时器
    heartbeat_timer_ = new QTimer(this);
    connect(heartbeat_timer_, &QTimer::timeout,
            this, &TcpServer::onHeartbeatTimeout);
    
    // 清理定时器
    cleanup_timer_ = new QTimer(this);
    connect(cleanup_timer_, &QTimer::timeout,
            this, &TcpServer::onCleanupTimeout);
    
    // 统计定时器
    stats_timer_ = new QTimer(this);
    connect(stats_timer_, &QTimer::timeout,
            this, &TcpServer::onStatisticsUpdated);
}

TcpServer::~TcpServer() {
    stop();
}

void TcpServer::setConfig(const TcpServerConfig& config) {
    config_ = config;
    
    if (thread_pool_) {
        thread_pool_->setMaxThreadCount(config.maxThreads);
        thread_pool_->setExpiryTimeout(config.threadIdleTime);
    }
}

bool TcpServer::start() {
    if (is_running_) {
        LOG_WARN("Server is already running");
        return true;
    }
    
    // 启动监听
    if (!server_socket_->listen(QHostAddress(config_.bindAddress), config_.port)) {
        QString error = "Failed to start server: " + server_socket_->errorString();
        LOG_ERROR(error);
        emit errorOccurred(error);
        return false;
    }
    
    listen_port_ = server_socket_->serverPort();
    is_running_ = true;
    
    // 启动定时器
    heartbeat_timer_->start(config_.heartbeatInterval * 1000);
    cleanup_timer_->start(60000);  // 每分钟清理
    stats_timer_->start(5000);  // 每5秒统计
    
    LOG_INFO("Server started on " + config_.bindAddress + ":" + 
             QString::number(listen_port_));
    
    emit serverStarted(listen_port_);
    return true;
}

void TcpServer::stop() {
    if (!is_running_) return;
    
    LOG_INFO("Stopping server...");
    
    // 停止监听
    server_socket_->close();
    
    // 断开所有连接
    disconnectAll();
    
    // 停止定时器
    heartbeat_timer_->stop();
    cleanup_timer_->stop();
    stats_timer_->stop();
    
    // 等待线程池结束
    thread_pool_->waitForDone(5000);
    
    is_running_ = false;
    listen_port_ = 0;
    
    LOG_INFO("Server stopped");
    emit serverStopped();
}

int TcpServer::connectionCount() const {
    QMutexLocker lock(&connections_mutex_);
    return connections_.size();
}

QStringList TcpServer::connectionIds() const {
    QMutexLocker lock(&connections_mutex_);
    return connections_.keys();
}

void TcpServer::disconnectAll() {
    QMutexLocker lock(&connections_mutex_);
    
    for (TcpConnection* conn : connections_.values()) {
        if (conn) {
            conn->disconnectClient();
        }
    }
    
    // 等待连接清理
    QThread::msleep(100);
    
    // 清理已断开的连接
    for (auto it = connections_.begin(); it != connections_.end();) {
        if (!it.value() || !it.value()->isConnected()) {
            it = connections_.erase(it);
        } else {
            ++it;
        }
    }
}

void TcpServer::disconnectClient(const QString& connection_id) {
    QMutexLocker lock(&connections_mutex_);
    
    TcpConnection* conn = connections_.value(connection_id);
    if (conn) {
        conn->disconnectClient();
        connections_.remove(connection_id);
    }
}

void TcpServer::broadcastMessage(const QString& message) {
    QMutexLocker lock(&connections_mutex_);
    
    for (TcpConnection* conn : connections_.values()) {
        if (conn && conn->isConnected()) {
            conn->sendMessage(message);
        }
    }
    
    LOG_DEBUG("Broadcasted message to " + QString::number(connections_.size()) + " clients");
}

void TcpServer::sendToClient(const QString& connection_id, const QString& message) {
    QMutexLocker lock(&connections_mutex_);
    
    TcpConnection* conn = connections_.value(connection_id);
    if (conn && conn->isConnected()) {
        conn->sendMessage(message);
    } else {
        LOG_WARN("Failed to send message: client not found or disconnected: " + connection_id);
    }
}

void TcpServer::setMessageHandler(MessageHandler* handler) {
    message_handler_ = handler;
}

void TcpServer::onNewConnection() {
    while (server_socket_->hasPendingConnections()) {
        qintptr descriptor = server_socket_->nextPendingConnectionDescriptor();
        if (descriptor != -1) {
            acceptConnection(descriptor);
        }
    }
}

void TcpServer::acceptConnection(qintptr socketDescriptor) {
    // 检查最大连接数
    if (connectionCount() >= config_.maxConnections) {
        LOG_WARN("Maximum connections reached, rejecting new connection");
        // 可以在这里添加拒绝连接的逻辑
        return;
    }
    
    // 创建连接对象
    TcpConnection* connection = new TcpConnection(socketDescriptor, this);
    
    // 信号连接
    connect(connection, &TcpConnection::messageReceived,
            this, &TcpServer::onConnectionMessage);
    connect(connection, &TcpConnection::disconnected,
            this, &TcpServer::onConnectionDisconnected);
    connect(connection, &TcpConnection::errorOccurred,
            this, &TcpServer::onConnectionError);
    connect(connection, &TcpConnection::activity,
            this, &TcpServer::onConnectionActivity);
    
    // 添加到连接列表
    {
        QMutexLocker lock(&connections_mutex_);
        connections_[connection->connectionId()] = connection;
    }
    
    ++total_connections_;
    
    // 通知消息处理器
    if (message_handler_) {
        message_handler_->onClientConnected(connection->connectionId());
    }
    
    emit clientConnected(connection->connectionId(), connection->info().address);
}

void TcpServer::onConnectionDisconnected(const QString& connection_id) {
    LOG_INFO("Client disconnected: " + connection_id);
    
    // 从连接列表移除
    {
        QMutexLocker lock(&connections_mutex_);
        connections_.remove(connection_id);
    }
    
    // 通知消息处理器
    if (message_handler_) {
        message_handler_->onClientDisconnected(connection_id);
    }
    
    emit clientDisconnected(connection_id);
}

void TcpServer::onConnectionMessage(const QString& connection_id, const QString& message) {
    LOG_DEBUG("Message from " + connection_id + ": " + message.left(100));
    
    // 统计
    total_bytes_in_ += message.size();
    
    // 转发给消息处理器
    if (message_handler_) {
        // 在线程池中处理消息
        thread_pool_->start([this, connection_id, message]() {
            QMetaObject::invokeMethod(this, [this, connection_id, message]() {
                message_handler_->handleMessage(connection_id, message);
            }, Qt::QueuedConnection);
        });
    }
    
    emit messageReceived(connection_id, message);
}

void TcpServer::onConnectionError(const QString& connection_id, const QString& error) {
    LOG_ERROR("Connection error: " + connection_id + " - " + error);
    Q_UNUSED(connection_id);
}

void TcpServer::onConnectionActivity(const QString& connection_id) {
    Q_UNUSED(connection_id);
    // 可以用于更新连接的活动状态
}

void TcpServer::onHeartbeatTimeout() {
    cleanupStaleConnections();
}

void TcpServer::onCleanupTimeout() {
    cleanupStaleConnections();
}

void TcpServer::onStatisticsUpdated() {
    QMutexLocker lock(&connections_mutex_);
    emit statisticsUpdated(
        connections_.size(),
        total_bytes_in_,
        total_bytes_out_
    );
}

void TcpServer::cleanupStaleConnections() {
    QMutexLocker lock(&connections_mutex_);
    
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    qint64 timeout = config_.connectionTimeout * 1000;
    
    QStringList to_remove;
    
    for (auto it = connections_.begin(); it != connections_.end(); ++it) {
        TcpConnection* conn = it.value();
        if (!conn || !conn->isConnected()) {
            to_remove.append(it.key());
        } else if (timeout > 0 && (now - conn->info().lastActivityTime) > timeout) {
            LOG_INFO("Disconnecting stale connection: " + it.key());
            conn->disconnectClient();
            to_remove.append(it.key());
        }
    }
    
    for (const QString& id : to_remove) {
        connections_.remove(id);
    }
    
    if (!to_remove.isEmpty()) {
        LOG_DEBUG("Cleaned up " + QString::number(to_remove.size()) + " stale connections");
    }
}

} // namespace server
} // namespace smartsched
