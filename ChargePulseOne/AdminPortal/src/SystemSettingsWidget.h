#pragma once
#include <QWidget>
#include <QTableWidget>
class NetworkManager;

class SystemSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SystemSettingsWidget(NetworkManager *net, QWidget *parent = nullptr);

private slots:
    void onResponse(const QJsonObject &resp);
    void onAdd();
    void onDelete();

private:
    NetworkManager *m_net;
    QTableWidget *table;
    void loadConfig();
};
