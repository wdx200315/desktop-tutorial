#include "NetworkManager.h"
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDebug>
#include <QCoreApplication>
#include <QNetworkInformation>

NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent)
    , m_socket(nullptr)
    , m_connected(false)
    , m_connectionState(Disconnected)
    , m_port(9010)
    , m_heartbeatInterval(30000)  // 30秒心跳
    , m_heartbeatMissed(0)
    , m_reconnectAttempts(0)
    , m_maxReconnectAttempts(5)
    , m_reconnectInterval(3000)    // 3秒重连间隔
    , m_autoReconnect(true)
    , m_bytesSent(0)
    , m_bytesReceived(0)
    , m_lastMessageTime(0)
    , m_networkAvailable(true)
{
    // 使用Qt 6的QNetworkInformation检测网络状态
    auto* networkInfo = QNetworkInformation::instance();
    
    // 监听网络状态变化
    connect(networkInfo, &QNetworkInformation::reachabilityChanged,
            this, [this](QNetworkInformation::Reachability reachability) {
        bool available = (reachability != QNetworkInformation::Reachability::Disconnected);
        onNetworkStatusChanged(available);
    });

    // 检查初始网络状态
    m_networkAvailable = (networkInfo->reachability() != QNetworkInformation::Reachability::Disconnected);

    // 初始化套接字
    initSocket();

    // 心跳定时器
    m_heartbeatTimer = new QTimer(this);
    m_heartbeatTimer->setInterval(m_heartbeatInterval);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &NetworkManager::sendHeartbeat);

    // 重连定时器
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, &NetworkManager::attemptReconnect);

    qDebug() << "[NetworkManager] Initialized, network available:" << m_networkAvailable;
}

NetworkManager::~NetworkManager()
{
    disconnect();
}

void NetworkManager::initSocket()
{
    QMutexLocker locker(&m_socketMutex);

    if (m_socket) {
        m_socket->disconnect();
        m_socket->deleteLater();
        m_socket = nullptr;
    }

    m_socket = new QTcpSocket(this);

    // 连接信号槽
    connect(m_socket, &QTcpSocket::connected, this, &NetworkManager::onConnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    connect(m_socket, &QAbstractSocket::errorOccurred, this, &NetworkManager::onError);
    connect(m_socket, &QAbstractSocket::stateChanged, this, &NetworkManager::onStateChanged);
}

bool NetworkManager::connected() const
{
    return m_connected && m_connectionState == Connected;
}

QString NetworkManager::token() const
{
    return m_token;
}

void NetworkManager::setToken(const QString &token)
{
    if (m_token != token) {
        m_token = token;
        emit tokenChanged();
        qDebug() << "[NetworkManager] Token updated";
    }
}

int NetworkManager::connectionState() const
{
    return m_connectionState;
}

void NetworkManager::connectToServer(const QString &host, int port)
{
    qDebug() << "[NetworkManager] Connecting to" << host << ":" << port;

    // 先断开现有连接
    if (m_socket && m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
    }

    m_host = host;
    m_port = port;
    m_reconnectAttempts = 0;

    doConnect();
}

void NetworkManager::doConnect()
{
    if (m_host.isEmpty() || m_port <= 0) {
        qWarning() << "[NetworkManager] Invalid host or port";
        emit errorOccurred("Invalid server address");
        setConnectionState(Failed);
        return;
    }

    if (!m_networkAvailable) {
        qWarning() << "[NetworkManager] Network not available";
        emit errorOccurred("Network not available");
        setConnectionState(Failed);
        return;
    }

    setConnectionState(Connecting);
    emit stateChanged("Connecting");

    // 确保在主线程执行
    QMutexLocker locker(&m_socketMutex);
    if (m_socket) {
        m_socket->connectToHost(m_host, m_port);
    }
}

void NetworkManager::disconnect()
{
    qDebug() << "[NetworkManager] Disconnecting...";

    m_autoReconnect = false;
    m_heartbeatTimer->stop();
    m_reconnectTimer->stop();

    QMutexLocker locker(&m_socketMutex);
    if (m_socket) {
        m_socket->disconnectFromHost();
    }

    setConnectionState(Disconnected);
    m_connected = false;
    emit connectedChanged();
}

void NetworkManager::reconnect()
{
    qDebug() << "[NetworkManager] Manual reconnect...";
    m_autoReconnect = true;
    m_reconnectAttempts = 0;
    doConnect();
}

void NetworkManager::sendRequest(const QJsonObject &request)
{
    QMutexLocker locker(&m_queueMutex);

    // 如果已连接，直接发送
    if (m_connected && m_socket && m_socket->state() == QAbstractSocket::ConnectedState) {
        locker.unlock();
        doSendRequest(request);
    } else {
        // 加入队列等待发送
        m_requestQueue.enqueue(request);
        qDebug() << "[NetworkManager] Request queued, queue size:" << m_requestQueue.size();

        // 尝试自动重连
        if (m_autoReconnect && m_connectionState != Connecting) {
            qDebug() << "[NetworkManager] Attempting auto reconnect for queued request";
            doConnect();
        }
    }
}

void NetworkManager::doSendRequest(const QJsonObject &request)
{
    QMutexLocker locker(&m_socketMutex);

    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) {
        emit errorOccurred("Socket not connected");
        return;
    }

    // 添加token到请求
    QJsonObject req = request;
    if (!m_token.isEmpty() && !req.contains("token")) {
        req["token"] = m_token;
    }

    // 序列化并发送
    QByteArray data = QJsonDocument(req).toJson(QJsonDocument::Compact);
    data.append('\n');

    qint64 written = m_socket->write(data);
    m_socket->flush();

    m_bytesSent += written;
    m_lastMessageTime = QDateTime::currentMSecsSinceEpoch();

    qDebug() << "[NetworkManager] Sent request:" << req["cmd"].toString()
             << "bytes:" << written;
}

void NetworkManager::processRequestQueue()
{
    QMutexLocker locker(&m_queueMutex);

    while (!m_requestQueue.isEmpty() && m_connected) {
        QJsonObject req = m_requestQueue.dequeue();
        locker.unlock();
        doSendRequest(req);
        locker.relock();
    }

    qDebug() << "[NetworkManager] Processed queue, remaining:" << m_requestQueue.size();
}

void NetworkManager::setConnectionState(ConnectionState state)
{
    if (m_connectionState != state) {
        m_connectionState = state;
        emit connectionStateChanged();
    }
}

// ========== 私有槽实现 ==========

void NetworkManager::onConnected()
{
    qDebug() << "[NetworkManager] Connected to server!";

    m_connected = true;
    m_heartbeatMissed = 0;
    m_reconnectAttempts = 0;

    setConnectionState(Connected);
    emit connectedChanged();
    emit stateChanged("Connected");

    // 启动心跳
    m_heartbeatTimer->start();

    // 处理队列中的请求
    processRequestQueue();

    // 发送连接成功事件
    QJsonObject event;
    event["event"] = "connected";
    emit responseReceived(event);
}

void NetworkManager::onReadyRead()
{
    QMutexLocker locker(&m_socketMutex);

    if (!m_socket) return;

    QByteArray newData = m_socket->readAll();
    m_bytesReceived += newData.size();
    m_lastMessageTime = QDateTime::currentMSecsSinceEpoch();

    // 添加到缓冲区
    QMutexLocker bufferLocker(&m_bufferMutex);
    m_buffer.append(newData);

    // 解析消息
    int pos;
    while ((pos = m_buffer.indexOf('\n')) != -1) {
        QByteArray line = m_buffer.left(pos);
        m_buffer.remove(0, pos + 1);
        bufferLocker.unlock();

        parseMessage(line);

        bufferLocker.relock();
    }
}

void NetworkManager::onError(QAbstractSocket::SocketError err)
{
    QString errorMsg;

    switch (err) {
        case QAbstractSocket::ConnectionRefusedError:
            errorMsg = "Connection refused";
            break;
        case QAbstractSocket::RemoteHostClosedError:
            errorMsg = "Remote host closed";
            break;
        case QAbstractSocket::HostNotFoundError:
            errorMsg = "Host not found";
            break;
        case QAbstractSocket::SocketTimeoutError:
            errorMsg = "Connection timeout";
            break;
        case QAbstractSocket::NetworkError:
            errorMsg = "Network error";
            break;
        case QAbstractSocket::SocketAccessError:
            errorMsg = "Socket access error";
            break;
        case QAbstractSocket::SocketResourceError:
            errorMsg = "Socket resource error";
            break;
        default:
            errorMsg = m_socket ? m_socket->errorString() : "Unknown error";
    }

    qWarning() << "[NetworkManager] Socket error:" << err << errorMsg;

    m_connected = false;
    setConnectionState(Disconnected);
    emit connectedChanged();
    emit errorOccurred(errorMsg);

    // 停止心跳
    m_heartbeatTimer->stop();

    // 自动重连
    if (m_autoReconnect) {
        scheduleReconnect();
    }
}

void NetworkManager::onStateChanged(QAbstractSocket::SocketState state)
{
    QString stateStr;
    switch (state) {
        case QAbstractSocket::UnconnectedState: stateStr = "Unconnected"; break;
        case QAbstractSocket::HostLookupState: stateStr = "HostLookup"; break;
        case QAbstractSocket::ConnectingState: stateStr = "Connecting"; break;
        case QAbstractSocket::ConnectedState: stateStr = "Connected"; break;
        case QAbstractSocket::BoundState: stateStr = "Bound"; break;
        case QAbstractSocket::ListeningState: stateStr = "Listening"; break;
        case QAbstractSocket::ClosingState: stateStr = "Closing"; break;
    }

    qDebug() << "[NetworkManager] Socket state changed:" << stateStr;

    // 处理意外断开
    if (state == QAbstractSocket::UnconnectedState && m_connected) {
        m_connected = false;
        emit connectedChanged();

        if (m_autoReconnect) {
            scheduleReconnect();
        }
    }
}

// ========== 心跳机制 ==========

void NetworkManager::sendHeartbeat()
{
    if (!m_connected) {
        qDebug() << "[NetworkManager] Skipping heartbeat - not connected";
        return;
    }

    qDebug() << "[NetworkManager] Sending heartbeat...";

    // 检查最后消息时间，如果超过2倍心跳间隔，认为连接可能已断开
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - m_lastMessageTime > m_heartbeatInterval * 2) {
        m_heartbeatMissed++;
        qDebug() << "[NetworkManager] Heartbeat missed:" << m_heartbeatMissed;

        if (m_heartbeatMissed >= MAX_HEARTBEAT_MISSED) {
            qWarning() << "[NetworkManager] Too many heartbeats missed, reconnecting...";
            m_socket->disconnectFromHost();
            scheduleReconnect();
            return;
        }
    } else {
        m_heartbeatMissed = 0;
    }

    // 发送心跳包
    QJsonObject heartbeat;
    heartbeat["cmd"] = CMD::HEARTBEAT;
    heartbeat["timestamp"] = now;
    heartbeat["data"] = QJsonObject();

    doSendRequest(heartbeat);
}

void NetworkManager::heartbeatTimeout()
{
    // 心跳超时处理（在sendHeartbeat中已处理）
}

// ========== 重连机制 ==========

void NetworkManager::attemptReconnect()
{
    if (m_reconnectAttempts >= m_maxReconnectAttempts) {
        qWarning() << "[NetworkManager] Max reconnect attempts reached";
        setConnectionState(Failed);
        emit errorOccurred("Connection failed after multiple attempts");
        return;
    }

    m_reconnectAttempts++;
    qDebug() << "[NetworkManager] Reconnect attempt" << m_reconnectAttempts
             << "of" << m_maxReconnectAttempts;

    setConnectionState(Reconnecting);
    emit reconnecting();

    // 重新初始化套接字
    initSocket();

    // 延迟重连
    int delay = m_reconnectInterval * m_reconnectAttempts;  // 递增延迟
    QTimer::singleShot(delay, this, &NetworkManager::doConnect);
}

void NetworkManager::scheduleReconnect()
{
    if (m_reconnectTimer->isActive()) {
        return;  // 已经在计划重连
    }

    qDebug() << "[NetworkManager] Scheduling reconnect...";
    setConnectionState(Reconnecting);
    emit reconnecting();

    // 指数退避重连
    int delay = m_reconnectInterval * (m_reconnectAttempts + 1);
    m_reconnectTimer->start(delay);
}

// ========== 网络状态 ==========

void NetworkManager::onNetworkStatusChanged(bool available)
{
    bool wasAvailable = m_networkAvailable;
    m_networkAvailable = available;

    qDebug() << "[NetworkManager] Network state changed. Available:" << m_networkAvailable;

    if (m_networkAvailable && !wasAvailable) {
        // 网络恢复，尝试重连
        qDebug() << "[NetworkManager] Network restored, reconnecting...";
        if (m_autoReconnect && m_connectionState != Connected) {
            m_reconnectAttempts = 0;
            doConnect();
        }
    } else if (!m_networkAvailable && wasAvailable) {
        // 网络断开
        qWarning() << "[NetworkManager] Network lost";
        emit errorOccurred("Network connection lost");
    }
}

bool NetworkManager::isNetworkAvailable()
{
    auto* networkInfo = QNetworkInformation::instance();
    return (networkInfo->reachability() != QNetworkInformation::Reachability::Disconnected);
}

// ========== 消息解析 ==========

void NetworkManager::parseMessage(const QByteArray &data)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);

    if (err.error != QJsonParseError::NoError) {
        qWarning() << "[NetworkManager] Invalid JSON:" << err.errorString();
        emit errorOccurred("Invalid JSON: " + err.errorString());
        return;
    }

    QJsonObject obj = doc.object();
    QString cmd = obj["cmd"].toString();

    // 重置心跳计数（收到任何消息都认为连接正常）
    m_heartbeatMissed = 0;

    // 处理心跳响应
    if (cmd == CMD::HEARTBEAT_RESPONSE) {
        qDebug() << "[NetworkManager] Heartbeat response received";
        return;
    }

    // 处理充电状态推送（实时数据）
    if (cmd == "3003" && obj["status"].toString() == "ok") {
        emit chargeStatusUpdate(obj["data"].toObject());
    }

    // 如果返回token，自动保存
    if (obj.contains("token")) {
        setToken(obj["token"].toString());
    }

    // 发送响应信号
    emit responseReceived(obj);

    qDebug() << "[NetworkManager] Parsed message, cmd:" << cmd;
}

// ========== 辅助函数 ==========

QJsonObject NetworkManager::createRequest(const QString &cmd, const QJsonObject &data)
{
    QJsonObject req;
    req["cmd"] = cmd;
    req["data"] = data;
    return req;
}

// ========== 业务API实现 ==========

void NetworkManager::login(const QString &username, const QString &password)
{
    QJsonObject data;
    data["username"] = username;
    data["password"] = password;
    sendRequest(createRequest("1001", data));
}

void NetworkManager::registerUser(const QString &username, const QString &password,
                                 const QString &phone, const QString &plate)
{
    QJsonObject data;
    data["username"] = username;
    data["password"] = password;
    data["phone"] = phone;
    data["plate_number"] = plate;
    sendRequest(createRequest("1002", data));
}

void NetworkManager::getChargerList(int page, int size, const QString &status)
{
    QJsonObject data;
    data["page"] = page;
    data["size"] = size;
    if (!status.isEmpty()) data["status"] = status;
    sendRequest(createRequest("2001", data));
}

void NetworkManager::getChargerDetail(int chargerId)
{
    QJsonObject data;
    data["charger_id"] = chargerId;
    sendRequest(createRequest("2002", data));
}

void NetworkManager::getOrderList(int page, int size, const QString &status)
{
    QJsonObject data;
    data["page"] = page;
    data["size"] = size;
    if (!status.isEmpty()) data["status"] = status;
    sendRequest(createRequest("5001", data));
}

void NetworkManager::getOrderDetail(int orderId)
{
    QJsonObject data;
    data["order_id"] = orderId;
    sendRequest(createRequest("5002", data));
}

void NetworkManager::getUserInfo()
{
    sendRequest(createRequest("1005"));
}

void NetworkManager::updateUserInfo(const QJsonObject &data)
{
    sendRequest(createRequest("1006", data));
}

void NetworkManager::startCharge(int chargerId, const QString &mode, double target)
{
    QJsonObject data;
    data["charger_id"] = chargerId;
    data["mode"] = mode;
    data["target"] = target;
    sendRequest(createRequest("3001", data));
}

void NetworkManager::stopCharge(int orderId)
{
    QJsonObject data;
    data["order_id"] = orderId;
    sendRequest(createRequest("3002", data));
}

void NetworkManager::getChargeStatus(int orderId)
{
    QJsonObject data;
    data["order_id"] = orderId;
    sendRequest(createRequest("3003", data));
}

void NetworkManager::createReservation(int chargerId, const QString &reserveTime,
                                      const QString &mode, double target)
{
    QJsonObject data;
    data["action"] = "create";
    data["charger_id"] = chargerId;
    data["reserve_time"] = reserveTime;
    data["mode"] = mode;
    data["target"] = target;
    sendRequest(createRequest("3004", data));
}

void NetworkManager::getAlarmList(int page, int size)
{
    QJsonObject data;
    data["page"] = page;
    data["size"] = size;
    sendRequest(createRequest("6001", data));
}

void NetworkManager::changePassword(const QString &oldPwd, const QString &newPwd)
{
    QJsonObject data;
    data["old_password"] = oldPwd;
    data["new_password"] = newPwd;
    sendRequest(createRequest("1003", data));
}

void NetworkManager::getRateList()
{
    sendRequest(createRequest("4001"));
}

void NetworkManager::getFeeEstimate(int chargerId, double energy)
{
    QJsonObject data;
    data["charger_id"] = chargerId;
    data["energy"] = energy;
    sendRequest(createRequest("4002", data));
}

void NetworkManager::getDailyReport(const QString &date)
{
    QJsonObject data;
    data["date"] = date;
    sendRequest(createRequest("7001", data));
}

void NetworkManager::getMonthlyReport(const QString &month)
{
    QJsonObject data;
    data["month"] = month;
    sendRequest(createRequest("7002", data));
}

void NetworkManager::getMemberLevels()
{
    sendRequest(createRequest("9201"));
}

void NetworkManager::getMemberStats()
{
    sendRequest(createRequest("9202"));
}

void NetworkManager::getCouponList()
{
    sendRequest(createRequest("9101"));
}

void NetworkManager::claimCoupon(int couponId)
{
    QJsonObject data;
    data["coupon_id"] = couponId;
    sendRequest(createRequest("9105", data));
}

void NetworkManager::rechargeBalance(double amount)
{
    QJsonObject data;
    data["action"] = "recharge";
    data["amount"] = amount;
    sendRequest(createRequest("4004", data));
}

void NetworkManager::getMessageList(int page, int size)
{
    QJsonObject data;
    data["page"] = page;
    data["size"] = size;
    sendRequest(createRequest("9001", data));
}

void NetworkManager::sendMessage(const QString &title, const QString &content)
{
    QJsonObject data;
    data["title"] = title;
    data["content"] = content;
    sendRequest(createRequest("9002", data));
}

void NetworkManager::deleteMessage(int msgId)
{
    QJsonObject data;
    data["id"] = msgId;
    sendRequest(createRequest("9004", data));
}

void NetworkManager::logout()
{
    m_token.clear();
    emit tokenChanged();

    // 发送登出请求
    sendRequest(createRequest("1004"));

    // 断开连接
    disconnect();
}
