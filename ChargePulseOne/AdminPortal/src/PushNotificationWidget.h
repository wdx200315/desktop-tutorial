#pragma once
#include <QWidget>
#include <QTableWidget>
class NetworkManager;

class PushNotificationWidget : public QWidget {
    Q_OBJECT
public:
    explicit PushNotificationWidget(NetworkManager *net, QWidget *parent = nullptr);

private slots:
    void onResponse(const QJsonObject &resp);
    void onSendNotification();
    void onDeleteNotification();
    void onRefresh();

private:
    NetworkManager *m_net;
    QTableWidget *table;
    void loadNotifications();
};
