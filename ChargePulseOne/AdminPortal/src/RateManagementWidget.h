#pragma once
#include <QWidget>
#include <QTableWidget>
class NetworkManager;

class RateManagementWidget : public QWidget
{
    Q_OBJECT
public:
    explicit RateManagementWidget(NetworkManager *net, QWidget *parent = nullptr);

private slots:
    void onResponse(const QJsonObject &resp);
    void onAdd();
    void onEdit();
    void onDelete();

private:
    NetworkManager *m_net;
    QTableWidget *table;
    void loadRates();
};
