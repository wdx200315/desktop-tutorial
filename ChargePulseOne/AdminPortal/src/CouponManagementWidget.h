#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
class NetworkManager;

class CouponManagementWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CouponManagementWidget(NetworkManager *net, QWidget *parent = nullptr);

private slots:
    void onResponse(const QJsonObject &resp);
    void onAdd();
    void onEdit();
    void onDelete();
    void onSearch();

private:
    NetworkManager *m_net;
    QTableWidget *table;
    QLineEdit *editSearch;
    void loadCoupons();
};
