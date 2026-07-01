#pragma once
#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QMutex>
#include <QQueue>
#include <QWaitCondition>
#include <QThread>
#include <QNetworkInformation>

class NetworkManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString token READ token WRITE setToken NOTIFY tokenChanged)
    Q_PROPERTY(int connectionState READ connectionState NOTIFY connectionStateChanged)
    Q_PROPERTY(QString serverHost READ serverHost WRITE setServerHost)
    Q_PROPERTY(int serverPort READ serverPort WRITE setServerPort)

public:
    // 连接状态枚举
    enum ConnectionState {
        Disconnected = 0,
        Connecting = 1,
        Connected = 2,
        Reconnecting = 3,
        Failed = 4
    };
    Q_ENUM(ConnectionState)

    explicit NetworkManager(QObject *parent = nullptr);
    ~NetworkManager();

    // 属性
    bool connected() const;
    QString token() const;
    void setToken(const QString &token);
    int connectionState() const;
    QString serverHost() const { return m_host; }
    void setServerHost(const QString &host) { m_host = host; }
    int serverPort() const { return m_port; }
    void setServerPort(int port) { m_port = port; }

    // 连接管理
    Q_INVOKABLE void connectToServer(const QString &host, int port);
    Q_INVOKABLE void disconnect();
    Q_INVOKABLE void reconnect();

    // 请求发送
    Q_INVOKABLE void sendRequest(const QJsonObject &request);

    // ---------- 业务 API ----------
    Q_INVOKABLE void login(const QString &username, const QString &password);
    Q_INVOKABLE void registerUser(const QString &username, const QString &password,
                                  const QString &phone, const QString &plate);
    Q_INVOKABLE void getChargerList(int page = 1, int size = 20, const QString &status = "");
    Q_INVOKABLE void getChargerDetail(int chargerId);
    Q_INVOKABLE void getOrderList(int page = 1, int size = 20, const QString &status = "");
    Q_INVOKABLE void getOrderDetail(int orderId);
    Q_INVOKABLE void getUserInfo();
    Q_INVOKABLE void updateUserInfo(const QJsonObject &data);
    Q_INVOKABLE void startCharge(int chargerId, const QString &mode, double target);
    Q_INVOKABLE void stopCharge(int orderId);
    Q_INVOKABLE void getChargeStatus(int orderId);
    Q_INVOKABLE void createReservation(int chargerId, const QString &reserveTime, const QString &mode, double target);
    Q_INVOKABLE void getAlarmList(int page = 1, int size = 20);

    // ---------- 新增 API ----------
    Q_INVOKABLE void changePassword(const QString &oldPwd, const QString &newPwd);
    Q_INVOKABLE void getRateList();
    Q_INVOKABLE void getFeeEstimate(int chargerId, double energy);
    Q_INVOKABLE void getDailyReport(const QString &date);
    Q_INVOKABLE void getMonthlyReport(const QString &month);
    Q_INVOKABLE void getMemberLevels();
    Q_INVOKABLE void getMemberStats();
    Q_INVOKABLE void getCouponList();
    Q_INVOKABLE void claimCoupon(int couponId);
    Q_INVOKABLE void rechargeBalance(double amount);
    Q_INVOKABLE void getMessageList(int page = 1, int size = 20);
    Q_INVOKABLE void sendMessage(const QString &title, const QString &content);
    Q_INVOKABLE void deleteMessage(int msgId);
    Q_INVOKABLE void logout();

    // 网络状态
    Q_INVOKABLE bool isNetworkAvailable();

signals:
    void connectedChanged();
    void tokenChanged();
    void connectionStateChanged();
    void responseReceived(const QJsonObject &response);
    void errorOccurred(const QString &error);
    void stateChanged(const QString &state);
    void reconnecting();
    void connectionTimeout();

    // 充电实时数据推送
    void chargeStatusUpdate(const QJsonObject &status);

private slots:
    void onConnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError err);
    void onStateChanged(QAbstractSocket::SocketState state);

    // 心跳
    void sendHeartbeat();
    void heartbeatTimeout();

    // 重连
    void attemptReconnect();

    // 刷新网络状态
    void onNetworkStatusChanged(bool available = false);

private:
    void initSocket();
    void setConnectionState(ConnectionState state);
    void doSendRequest(const QJsonObject &request);
    void parseMessage(const QByteArray &data);
    void processRequestQueue();
    void scheduleReconnect();
    void doConnect();
    QJsonObject createRequest(const QString &cmd, const QJsonObject &data = QJsonObject());

    // 网络相关
    QTcpSocket *m_socket;
    QMutex m_socketMutex;

    // 状态
    bool m_connected;
    ConnectionState m_connectionState;
    QString m_token;
    QString m_host;
    int m_port;

    // 心跳
    QTimer *m_heartbeatTimer;
    int m_heartbeatInterval;
    int m_heartbeatMissed;
    static const int MAX_HEARTBEAT_MISSED = 3;

    // 重连
    QTimer *m_reconnectTimer;
    int m_reconnectAttempts;
    int m_maxReconnectAttempts;
    int m_reconnectInterval;
    bool m_autoReconnect;

    // 请求队列
    QQueue<QJsonObject> m_requestQueue;
    QMutex m_queueMutex;

    // 缓冲
    QByteArray m_buffer;
    QMutex m_bufferMutex;

    // 统计
    qint64 m_bytesSent;
    qint64 m_bytesReceived;
    qint64 m_lastMessageTime;

    // 网络可用性
    bool m_networkAvailable;
};

// 心跳命令定义
namespace CMD {
    const QString HEARTBEAT = "9999";
    const QString HEARTBEAT_RESPONSE = "9998";
}
