#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
class NetworkManager;

class ChargerManagementWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ChargerManagementWidget(NetworkManager *net, QWidget *parent = nullptr);

private slots:
    void onResponse(const QJsonObject &resp);
    void onSearch();
    void onControl(const QString &command);
    void onAddCharger();

private:
    NetworkManager *m_net;
    QTableWidget *table;
    QLineEdit *editKeyword;
    QComboBox *comboStatus;
    void loadChargers();
};
