#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
class NetworkManager;

class ReservationManagementWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ReservationManagementWidget(NetworkManager *net, QWidget *parent = nullptr);

private slots:
    void onResponse(const QJsonObject &resp);
    void onSearch();
    void onCancel();

private:
    NetworkManager *m_net;
    QTableWidget *table;
    QComboBox *comboStatus;
    void loadReservations();
};
