#pragma once
#include <QWidget>
#include <QLabel>
#include <QTableWidget>
#include <QDateEdit>
class NetworkManager;

class FinanceReportWidget : public QWidget {
    Q_OBJECT
public:
    explicit FinanceReportWidget(NetworkManager *net, QWidget *parent = nullptr);

private slots:
    void onResponse(const QJsonObject &resp);
    void onDaily();
    void onMonthly();
    void onExport();
    void onRefresh();

private:
    NetworkManager *m_net;
    QDateEdit *dateEdit;
    QTableWidget *table;
    QLabel *lblTotalRevenue;
    QLabel *lblTotalOrders;
    QLabel *lblAvgOrderValue;
    QLabel *lblTotalEnergy;
    void loadData();
};
