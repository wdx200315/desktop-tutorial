#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
class NetworkManager;

class LogAuditWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LogAuditWidget(NetworkManager *net, QWidget *parent = nullptr);

private slots:
    void onResponse(const QJsonObject &resp);
    void onSearch();

private:
    NetworkManager *m_net;
    QTableWidget *table;
    QComboBox *comboType;
    void loadLogs();
};
