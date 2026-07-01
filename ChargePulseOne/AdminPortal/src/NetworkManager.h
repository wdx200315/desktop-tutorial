#pragma once
#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>

class NetworkManager : public QObject
{
    Q_OBJECT
public:
    explicit NetworkManager(QObject *parent = nullptr);
    void connectToServer(const QString &host, int port);
    void sendRequest(const QJsonObject &request);
    void setToken(const QString &token);
    QString token() const;
    bool isConnected() const;

    // API 方法
    void login(const QString &username, const QString &password);
    void getUserList(int page = 1, int size = 20, const QString &keyword = "",
                     const QString &role = "", const QString &status = "");
    void editUser(int userId, const QJsonObject &data);
    void getChargerList(int page = 1, int size = 20, const QString &status = "");
    void controlCharger(int chargerId, const QString &command);
    void getRateList();
    void addRate(const QJsonObject &data);
    void updateRate(int rateId, const QJsonObject &data);
    void deleteRate(int rateId);
    void getOrderList(int page = 1, int size = 20, const QString &status = "");
    void getAlarmList(int page = 1, int size = 20);
    void handleAlarm(int alarmId, const QString &note);
    void getDailyReport(const QString &date);
    void getMonthlyReport(const QString &month);
    void getSystemConfig();
    void setSystemConfig(const QString &key, const QString &value);
    void getOperationLogs(int page = 1, int size = 20, const QString &type = "");

signals:
    void connected();
    void disconnected();
    void responseReceived(const QJsonObject &response);
    void errorOccurred(const QString &error);

private slots:
    void onConnected();
    void onReadyRead();
    void onSocketError(QAbstractSocket::SocketError);

private:
    QTcpSocket *socket;
    QString m_token;
    bool m_connected;
    QByteArray buffer;
    void parseMessage(const QByteArray &data);
};
