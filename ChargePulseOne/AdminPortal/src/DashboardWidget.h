#ifndef DASHBOARDWIDGET_H
#define DASHBOARDWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QJsonObject>

class NetworkManager;

class DashboardWidget : public QWidget {
    Q_OBJECT
public:
    explicit DashboardWidget(NetworkManager* net, QWidget* parent = nullptr);
    ~DashboardWidget();

private slots:
    void refresh();
    void onResponse(const QJsonObject& resp);
    void refreshStats();
    void updateClock();

private:
    NetworkManager* m_net;
    QTimer* m_refreshTimer;
    QTimer* m_clockTimer;

    // 时间显示
    QLabel* m_lblTime;

    // 统计卡片标签
    QLabel* m_lblTodayOrders;
    QLabel* m_lblTodayRevenue;
    QLabel* m_lblTotalEnergy;

    // 充电桩状态
    QLabel* m_lblOnlineChargers;
    QLabel* m_lblChargingChargers;
    QLabel* m_lblOfflineChargers;
    QLabel* m_lblFaultChargers;

    // 告警
    QLabel* m_lblRecentAlarm;

    // 数据
    QList<double> m_weeklyOrders;
    QList<double> m_weeklyRevenue;
    QList<double> m_weeklyEnergy;
    double m_todayOrders = 0;
    double m_todayEnergy = 0;
    double m_todayRevenue = 0;
    double m_totalEnergy = 0;
    double m_avgChargingTime = 0;
    double m_avgSatisfaction = 0;
    int m_totalChargers = 0;
    int m_onlineChargers = 0;
    int m_chargingChargers = 0;
    int m_activeAlarms = 0;

    void setupUI();
    QWidget* createStatCard(const QString& title, const QString& value, const QString& unit, const QString& color);
    QWidget* createInfoCard(const QString& title);
    void updateSimulatedData();
};

#endif // DASHBOARDWIDGET_H
