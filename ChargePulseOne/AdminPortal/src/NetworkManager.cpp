#include "common.h"
#include "NetworkManager.h"
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDebug>

NetworkManager::NetworkManager(QObject *parent) : QObject(parent), m_connected(false)
{
    socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::connected, this, &NetworkManager::onConnected);
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, &NetworkManager::onSocketError);
}

void NetworkManager::connectToServer(const QString &host, int port)
{
    socket->connectToHost(host, port);
}

bool NetworkManager::isConnected() const { return m_connected; }
QString NetworkManager::token() const { return m_token; }
void NetworkManager::setToken(const QString &token) { m_token = token; }

void NetworkManager::sendRequest(const QJsonObject &request)
{
    if (!m_connected) {
        emit errorOccurred("Not connected to server");
        return;
    }
    QJsonObject req = request;
    if (!m_token.isEmpty() && !req.contains("token")) {
        req["token"] = m_token;
    }
    QByteArray data = QJsonDocument(req).toJson(QJsonDocument::Compact);
    data.append('\n');
    socket->write(data);
}

void NetworkManager::login(const QString &username, const QString &password)
{
    QJsonObject req;
    req["cmd"] = CMD::LOGIN;
    QJsonObject data;
    data["username"] = username;
    data["password"] = password;
    req["data"] = data;
    sendRequest(req);
}

void NetworkManager::getUserList(int page, int size, const QString &keyword,
                                 const QString &role, const QString &status)
{
    QJsonObject req;
    req["cmd"] = CMD::USER_LIST;
    QJsonObject data;
    data["page"] = page;
    data["size"] = size;
    if (!keyword.isEmpty()) data["keyword"] = keyword;
    if (!role.isEmpty()) data["role"] = role;
    if (!status.isEmpty()) data["status"] = status;
    req["data"] = data;
    sendRequest(req);
}

void NetworkManager::editUser(int userId, const QJsonObject &data)
{
    QJsonObject req;
    req["cmd"] = CMD::USER_EDIT;
    QJsonObject d = data;
    d["user_id"] = userId;
    req["data"] = d;
    sendRequest(req);
}

void NetworkManager::getChargerList(int page, int size, const QString &status)
{
    QJsonObject req;
    req["cmd"] = CMD::CHARGER_LIST;
    QJsonObject data;
    data["page"] = page;
    data["size"] = size;
    if (!status.isEmpty()) data["status"] = status;
    req["data"] = data;
    sendRequest(req);
}

void NetworkManager::controlCharger(int chargerId, const QString &command)
{
    QJsonObject req;
    req["cmd"] = CMD::CHARGER_CTRL;
    QJsonObject data;
    data["id"] = chargerId;
    data["command"] = command;
    req["data"] = data;
    sendRequest(req);
}

void NetworkManager::getRateList()
{
    QJsonObject req;
    req["cmd"] = CMD::RATE_LIST;
    req["data"] = QJsonObject();
    sendRequest(req);
}

void NetworkManager::addRate(const QJsonObject &data)
{
    QJsonObject req;
    req["cmd"] = CMD::RATE_LIST; // 使用同一命令但内部 action? 简化
    // 为了简化，服务端 handleRateAdd 已实现，我们用不同的数据字段
    QJsonObject d = data;
    d["action"] = "add";
    req["data"] = d;
    sendRequest(req);
}

void NetworkManager::updateRate(int rateId, const QJsonObject &data)
{
    QJsonObject req;
    req["cmd"] = CMD::RATE_LIST;
    QJsonObject d = data;
    d["id"] = rateId;
    d["action"] = "update";
    req["data"] = d;
    sendRequest(req);
}

void NetworkManager::deleteRate(int rateId)
{
    QJsonObject req;
    req["cmd"] = CMD::RATE_LIST;
    QJsonObject data;
    data["id"] = rateId;
    data["action"] = "delete";
    req["data"] = data;
    sendRequest(req);
}

void NetworkManager::getOrderList(int page, int size, const QString &status)
{
    QJsonObject req;
    req["cmd"] = CMD::ORDER_LIST;
    QJsonObject data;
    data["page"] = page;
    data["size"] = size;
    if (!status.isEmpty()) data["status"] = status;
    req["data"] = data;
    sendRequest(req);
}

void NetworkManager::getAlarmList(int page, int size)
{
    QJsonObject req;
    req["cmd"] = CMD::ALARM_LIST;
    QJsonObject data;
    data["page"] = page;
    data["size"] = size;
    req["data"] = data;
    sendRequest(req);
}

void NetworkManager::handleAlarm(int alarmId, const QString &note)
{
    QJsonObject req;
    req["cmd"] = CMD::ALARM_HANDLE;
    QJsonObject data;
    data["id"] = alarmId;
    data["note"] = note;
    req["data"] = data;
    sendRequest(req);
}

void NetworkManager::getDailyReport(const QString &date)
{
    QJsonObject req;
    req["cmd"] = CMD::REPORT_DAILY;
    QJsonObject data;
    data["date"] = date;
    req["data"] = data;
    sendRequest(req);
}

void NetworkManager::getMonthlyReport(const QString &month)
{
    QJsonObject req;
    req["cmd"] = CMD::REPORT_MONTHLY;
    QJsonObject data;
    data["month"] = month;
    req["data"] = data;
    sendRequest(req);
}

void NetworkManager::getSystemConfig()
{
    QJsonObject req;
    req["cmd"] = CMD::SYS_CONFIG_GET;
    req["data"] = QJsonObject();
    sendRequest(req);
}

void NetworkManager::setSystemConfig(const QString &key, const QString &value)
{
    QJsonObject req;
    req["cmd"] = CMD::SYS_CONFIG_SET;
    QJsonObject data;
    data["key"] = key;
    data["value"] = value;
    req["data"] = data;
    sendRequest(req);
}

void NetworkManager::getOperationLogs(int page, int size, const QString &type)
{
    QJsonObject req;
    req["cmd"] = CMD::LOG_QUERY;
    QJsonObject data;
    data["page"] = page;
    data["size"] = size;
    if (!type.isEmpty()) data["type"] = type;
    req["data"] = data;
    sendRequest(req);
}

// ---------- 私有槽 ----------
void NetworkManager::onConnected()
{
    m_connected = true;
    emit connected();
}

void NetworkManager::onReadyRead()
{
    buffer.append(socket->readAll());
    int pos;
    while ((pos = buffer.indexOf('\n')) != -1) {
        QByteArray line = buffer.left(pos);
        buffer.remove(0, pos + 1);
        parseMessage(line);
    }
}

void NetworkManager::onSocketError(QAbstractSocket::SocketError)
{
    m_connected = false;
    emit disconnected();
    emit errorOccurred(socket->errorString());
}

void NetworkManager::parseMessage(const QByteArray &data)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) {
        emit errorOccurred("Invalid JSON");
        return;
    }
    QJsonObject obj = doc.object();
    if (obj.contains("token")) {
        setToken(obj["token"].toString());
    }
    emit responseReceived(obj);
}
