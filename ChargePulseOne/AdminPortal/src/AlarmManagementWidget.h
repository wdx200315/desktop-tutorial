#pragma once
#include <QWidget>
#include <QTableWidget>
class NetworkManager;

class AlarmManagementWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AlarmManagementWidget(NetworkManager *net, QWidget *parent = nullptr);

private slots:
    void onResponse(const QJsonObject &resp);
    void onHandle();
    void refresh();

private:
    NetworkManager *m_net;
    QTableWidget *table;
    void loadAlarms();
};
