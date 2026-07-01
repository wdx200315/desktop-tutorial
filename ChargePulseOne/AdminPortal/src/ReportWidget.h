#pragma once
#include <QWidget>
#include <QDateEdit>
#include <QLabel>
class NetworkManager;

class ReportWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ReportWidget(NetworkManager *net, QWidget *parent = nullptr);

private slots:
    void onResponse(const QJsonObject &resp);
    void onDaily();
    void onMonthly();

private:
    NetworkManager *m_net;
    QDateEdit *dateEdit;
    QLabel *lblOrders, *lblEnergy, *lblRevenue, *lblAlarms;
};
