/**
 * @file tcpserver.h
 * @brief 多线程TCP服务器
 * 
 * 基于Qt的TcpServer实现，支持：
 * - 多线程处理客户端连接
 * - 线程池管理
 * - 心跳检测
 * - 断线重连
 */

#pragma once

#include <QtCore/QObject>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QHostAddress>
#include <QtCore/QThreadPool>
#include <QtCore/QMutex>
#include <QtCore/QMap>
#include <QtCore/QSet>
#include <QtCore/QWaitCondition>
#include <memory>
#include <functional>
#include "../../common/include/smartsched/protocol/message.h"
#include "../../common/include/smartsched/utils/logger.h"

namespace smartsched {
namespace server {

// =============================================================================
// 前向声明
// =============================================================================
class TcpConnection;
class MessageHandler;

// =============================================================================
// TCP服务器配置
// =============================================================================
struct TcpServerConfig {
    quint16 port = 8888;
    QString bindAddress = "0.0.0.0";
    
    // 线程池配置
    int minThreads = 4;
    int maxThreads = 16;
    int threadIdleTime = 60000;  // ms
    
    // 连接配置
    int maxConnections = 1000;
    int connectionTimeout = 300;  // seconds
    int heartbeatInterval = 30;   // seconds
    
    // 缓冲区配置
    int maxMessageSize = 1024 * 1024;  // 1MB
    
    // SSL配置（可选）
    bool useSSL = false;
    QString sslCertFile;
    QString sslKeyFile;
    
    // 心跳配置
    bool enableHeartbeat = true;
    int heartbeatTimeout = 90;  // seconds
};

// =============================================================================
// TCP连接信息
// =============================================================================
struct ConnectionInfo {
    QString clientId;
    QHostAddress address;
    quint16 port;
    qint64 connectTime;
    qint64 lastActivityTime;
    bool authenticated;
    QString sessionId;
    
    ConnectionInfo() : connectTime(0), lastActivityTime(0), authenticated(false) {}
};

// =============================================================================
// TCP连接类
// =============================================================================
class TcpConnection : public QObject {
    Q_OBJECT
    
public:
    TcpConnection(qintptr socketDescriptor, QObject* parent = nullptr);
    ~TcpConnection() override;
    
    // 连接状态
    bool isConnected() const { return socket_ && socket_->state() == QAbstractSocket::ConnectedState; }
    QString connectionId() const { return connection_id_; }
    const ConnectionInfo& info() const { return info_; }
    
    // 发送消息
    void sendMessage(const QString& json_message);
    void sendRawData(const QByteArray& data);
    
    // 连接管理
    void disconnectClient();
    void setHeartbeatEnabled(bool enabled);
    
signals:
    void messageReceived(const QString& connection_id, const QString& message);
    void disconnected(const QString& connection_id);
    void errorOccurred(const QString& connection_id, const QString& error);
    void activity(const QString& connection_id);
    
private slots:
    void onReadyRead();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError error);
    void onBytesWritten(qint64 bytes);
    void sendHeartbeat();
    
private:
    void processIncomingData();
    qint64 readMessageLength();
    
    QString connection_id_;
    QTcpSocket* socket_;
    ConnectionInfo info_;
    
    // 数据缓冲区
    QByteArray read_buffer_;
    QByteArray write_buffer_;
    
    // 心跳
    QTimer* heartbeat_timer_;
    bool heartbeat_enabled_;
    qint64 last_heartbeat_time_;
    
    // 统计
    qint64 bytes_sent_;
    qint64 bytes_received_;
    
    static qint64 connection_counter_;
    static QMutex counter_mutex_;
};

// =============================================================================
// TCP服务器
// =============================================================================
class TcpServer : public QObject {
    Q_OBJECT
    
public:
    explicit TcpServer(QObject* parent = nullptr);
    ~TcpServer() override;
    
    // 配置
    void setConfig(const TcpServerConfig& config);
    TcpServerConfig config() const { return config_; }
    
    // 生命周期
    bool start();
    void stop();
    bool isRunning() const { return is_running_; }
    
    // 连接管理
    int connectionCount() const;
    QStringList connectionIds() const;
    void disconnectAll();
    void disconnectClient(const QString& connection_id);
    
    // 广播
    void broadcastMessage(const QString& message);
    void sendToClient(const QString& connection_id, const QString& message);
    
    // 消息处理器
    void setMessageHandler(MessageHandler* handler);
    MessageHandler* messageHandler() const { return message_handler_; }
    
signals:
    void serverStarted(quint16 port);
    void serverStopped();
    void clientConnected(const QString& connection_id, const QHostAddress& address);
    void clientDisconnected(const QString& connection_id);
    void messageReceived(const QString& connection_id, const QString& message);
    void errorOccurred(const QString& error);
    void statisticsUpdated(int connections, qint64 bytesIn, qint64 bytesOut);
    
private slots:
    void onNewConnection();
    void onConnectionDisconnected(const QString& connection_id);
    void onConnectionMessage(const QString& connection_id, const QString& message);
    void onConnectionError(const QString& connection_id, const QString& error);
    void onConnectionActivity(const QString& connection_id);
    void onHeartbeatTimeout();
    void onCleanupTimeout();
    
private:
    void acceptConnection(qintptr socketDescriptor);
    void cleanupStaleConnections();
    
    TcpServerConfig config_;
    QTcpServer* server_socket_;
    QThreadPool* thread_pool_;
    
    // 连接管理
    QMap<QString, TcpConnection*> connections_;
    QMutex connections_mutex_;
    
    // 消息处理器
    MessageHandler* message_handler_;
    
    // 状态
    bool is_running_;
    quint16 listen_port_;
    
    // 统计
    qint64 total_connections_;
    qint64 total_bytes_in_;
    qint64 total_bytes_out_;
    
    // 定时器
    QTimer* heartbeat_timer_;
    QTimer* cleanup_timer_;
    
    // 统计定时器
    QTimer* stats_timer_;
};

// =============================================================================
// 消息处理器接口
// =============================================================================
class MessageHandler : public QObject {
    Q_OBJECT
public:
    explicit MessageHandler(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~MessageHandler() = default;
    
    virtual void handleMessage(const QString& connection_id, const QString& message) = 0;
    virtual void onClientConnected(const QString& connection_id) = 0;
    virtual void onClientDisconnected(const QString& connection_id) = 0;
    virtual void onHeartbeat(const QString& connection_id) = 0;
};

// =============================================================================
// 服务器统计信息
// =============================================================================
struct ServerStatistics {
    qint64 uptime;
    int peakConnections;
    qint64 totalConnections;
    qint64 totalMessages;
    qint64 totalBytesIn;
    qint64 totalBytesOut;
    double messagesPerSecond;
    double bytesPerSecond;
};

// =============================================================================
// 内联实现
// =============================================================================

inline TcpConnection::TcpConnection(qintptr socketDescriptor, QObject* parent)
    : QObject(parent)
    , socket_(nullptr)
    , heartbeat_enabled_(true)
    , last_heartbeat_time_(0)
    , bytes_sent_(0)
    , bytes_received_(0)
{
    // 生成唯一连接ID
    {
        QMutexLocker lock(&counter_mutex_);
        ++connection_counter_;
        connection_id_ = QString("conn_%1").arg(connection_counter_, 8, 16, QChar('0'));
    }
    
    socket_ = new QTcpSocket(this);
    
    // 设置socket描述符
    if (!socket_->setSocketDescriptor(socketDescriptor)) {
        LOG_ERROR("Failed to set socket descriptor: " + socket_->errorString());
        return;
    }
    
    // 获取连接信息
    info_.clientId = connection_id_;
    info_.address = socket_->peerAddress();
    info_.port = socket_->peerPort();
    info_.connectTime = QDateTime::currentMSecsSinceEpoch();
    info_.lastActivityTime = info_.connectTime;
    
    // 信号连接
    connect(socket_, &QTcpSocket::readyRead, this, &TcpConnection::onReadyRead);
    connect(socket_, &QTcpSocket::disconnected, this, &TcpConnection::onDisconnected);
    connect(socket_, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
            this, &TcpConnection::onError);
    connect(socket_, &QTcpSocket::bytesWritten, this, &TcpConnection::onBytesWritten);
    
    // 心跳定时器
    heartbeat_timer_ = new QTimer(this);
    connect(heartbeat_timer_, &QTimer::timeout, this, &TcpConnection::sendHeartbeat);
    heartbeat_timer_->start(30000);  // 30秒心跳
    
    LOG_INFO("New connection: " + connection_id_ + " from " + 
             info_.address.toString() + ":" + QString::number(info_.port));
}

inline TcpConnection::~TcpConnection() {
    if (socket_) {
        socket_->disconnectFromHost();
        socket_->deleteLater();
    }
    LOG_DEBUG("Connection destroyed: " + connection_id_);
}

inline void TcpConnection::sendMessage(const QString& json_message) {
    if (!isConnected()) return;
    
    QByteArray data = json_message.toUtf8();
    data.append('\n');  // 消息分隔符
    
    qint64 written = socket_->write(data);
    if (written > 0) {
        bytes_sent_ += written;
        socket_->flush();
    }
}

inline void TcpConnection::sendRawData(const QByteArray& data) {
    if (!isConnected()) return;
    
    qint64 written = socket_->write(data);
    if (written > 0) {
        bytes_sent_ += written;
        socket_->flush();
    }
}

inline void TcpConnection::disconnectClient() {
    if (socket_) {
        socket_->disconnectFromHost();
    }
}

inline void TcpConnection::setHeartbeatEnabled(bool enabled) {
    heartbeat_enabled_ = enabled;
    if (enabled) {
        heartbeat_timer_->start();
    } else {
        heartbeat_timer_->stop();
    }
}

} // namespace server
} // namespace smartsched
