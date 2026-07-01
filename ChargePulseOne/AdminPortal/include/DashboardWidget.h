#ifndef DASHBOARD_WIDGET_H
#define DASHBOARD_WIDGET_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>

class NetworkManager;

class DashboardWidget : public QWidget {
    Q_OBJECT

public:
    explicit DashboardWidget(NetworkManager* net, QWidget* parent = nullptr);
    ~DashboardWidget();

private:
    NetworkManager* m_net;
    QTimer* m_refreshTimer;
    QTimer* m_chartTimer;
    QTimer* m_clockTimer;

    // 统计卡片标签
    QLabel* m_lblTotalOrders;
    QLabel* m_lblTodayOrders;
    QLabel* m_lblTotalEnergy;
    QLabel* m_lblTodayEnergy;
    QLabel* m_lblTotalRevenue;
    QLabel* m_lblTodayRevenue;
    QLabel* m_lblOnlineChargers;
    QLabel* m_lblActiveUsers;
    QLabel* m_lblActiveAlarms;
    QLabel* m_lblAvgChargingTime;
    QLabel* m_lblAvgSatisfaction;

    // 统计数据
    double m_todayOrders = 0;
    double m_todayEnergy = 0;
    double m_todayRevenue = 0;
    double m_totalOrders = 0;
    double m_totalEnergy = 0;
    double m_totalRevenue = 0;
    int m_onlineChargers = 0;
    int m_totalChargers = 0;
    int m_activeUsers = 0;
    int m_activeAlarms = 0;
    double m_avgChargingTime = 0;
    double m_avgSatisfaction = 0;

    // 图表
    QChartView* m_chargerStatusChart;
    QChartView* m_revenueChart;
    QChartView* m_energyChart;

    // 最近7天收入数据
    QVector<double> m_weeklyRevenue;
    QVector<double> m_weeklyEnergy;

    void setupUI();
    QLabel* createStatCard(const QString& icon, const QString& title, QLabel* valueLabel,
                          const QString& bgColor, const QString& textColor);

private slots:
    void refresh();
    void refreshStats();
    void updateCharts();
    void updateClock();
    void onResponse(const QJsonObject& resp);

    // 图表创建函数
    void createChargerStatusChart();
    void createRevenueChart();
    void createEnergyChart();
    void updateRevenueChart(double todayRevenue);
};

#endif // DASHBOARD_WIDGET_H
