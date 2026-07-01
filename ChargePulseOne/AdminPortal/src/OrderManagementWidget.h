#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
class NetworkManager;

class OrderManagementWidget : public QWidget
{
    Q_OBJECT
public:
    explicit OrderManagementWidget(NetworkManager *net, QWidget *parent = nullptr);

private slots:
    void onResponse(const QJsonObject &resp);
    void onSearch();
    void onExport();

private:
    NetworkManager *m_net;
    QTableWidget *table;
    QComboBox *comboStatus;
    void loadOrders();
};
