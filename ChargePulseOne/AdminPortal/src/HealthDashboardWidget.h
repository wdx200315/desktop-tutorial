#pragma once
#include <QWidget>
#include <QLabel>
class NetworkManager;

class HealthDashboardWidget : public QWidget {
    Q_OBJECT
public:
    explicit HealthDashboardWidget(NetworkManager *net, QWidget *parent = nullptr);

private slots:
    void onRefresh();
    void updateStats();

private:
    NetworkManager *m_net;
    QLabel *lblOverallHealth;
    QLabel *lblChargerHealth;
    QLabel *lblNetworkHealth;
    QLabel *lblDatabaseHealth;
    QLabel *lblLastCheck;
    void loadHealthData();
};
