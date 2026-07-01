#include <QJsonArray>
#include "common.h"
#include "CouponManagementWidget.h"
#include "NetworkManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <QJsonArray>

CouponManagementWidget::CouponManagementWidget(NetworkManager *net, QWidget *parent)
    : QWidget(parent), m_net(net)
{
    auto *layout = new QVBoxLayout(this);

    auto *searchLayout = new QHBoxLayout;
    searchLayout->addWidget(new QLabel("搜索:"));
    editSearch = new QLineEdit;
    editSearch->setPlaceholderText("券码/名称");
    searchLayout->addWidget(editSearch);
    auto *btnSearch = new QPushButton("搜索");
    connect(btnSearch, &QPushButton::clicked, this, &CouponManagementWidget::onSearch);
    searchLayout->addWidget(btnSearch);
    searchLayout->addStretch();
    layout->addLayout(searchLayout);

    table = new QTableWidget;
    table->setColumnCount(7);
    table->setHorizontalHeaderLabels({"ID", "券码", "类型", "面值", "满减", "有效期", "库存"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(table);

    auto *btnLayout = new QHBoxLayout;
    auto *btnAdd = new QPushButton("添加");
    auto *btnEdit = new QPushButton("编辑");
    auto *btnDelete = new QPushButton("删除");
    btnLayout->addWidget(btnAdd);
    btnLayout->addWidget(btnEdit);
    btnLayout->addWidget(btnDelete);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    connect(btnAdd, &QPushButton::clicked, this, &CouponManagementWidget::onAdd);
    connect(btnEdit, &QPushButton::clicked, this, &CouponManagementWidget::onEdit);
    connect(btnDelete, &QPushButton::clicked, this, &CouponManagementWidget::onDelete);
    connect(m_net, &NetworkManager::responseReceived, this, &CouponManagementWidget::onResponse);

    loadCoupons();
}

void CouponManagementWidget::loadCoupons()
{
    QJsonObject req;
    req["cmd"] = CMD::COUPON_LIST;
    req["data"] = QJsonObject();
    m_net->sendRequest(req);
}

void CouponManagementWidget::onSearch()
{
    // 服务端暂不支持搜索，本地过滤
    // 简化：直接重新加载
    loadCoupons();
}

void CouponManagementWidget::onResponse(const QJsonObject &resp)
{
    if (resp["cmd"].toString() == CMD::COUPON_LIST && resp["status"].toString() == "ok") {
        auto list = resp["data"].toArray();
        table->setRowCount(list.size());
        for (int i = 0; i < list.size(); ++i) {
            auto obj = list[i].toObject();
            table->setItem(i, 0, new QTableWidgetItem(obj["id"].toString()));
            table->setItem(i, 1, new QTableWidgetItem(obj["code"].toString()));
            table->setItem(i, 2, new QTableWidgetItem(obj["type"].toString()));
            table->setItem(i, 3, new QTableWidgetItem(obj["value"].toString()));
            table->setItem(i, 4, new QTableWidgetItem(obj["min_order_amount"].toString()));
            QString valid = obj["valid_from"].toString() + " ~ " + obj["valid_to"].toString();
            table->setItem(i, 5, new QTableWidgetItem(valid));
            table->setItem(i, 6, new QTableWidgetItem(obj["quantity"].toString()));
        }
    }
}

void CouponManagementWidget::onAdd()
{
    bool ok;
    QString code = QInputDialog::getText(this, "添加优惠券", "券码:", QLineEdit::Normal, "", &ok);
    if (!ok) return;
    QString type = QInputDialog::getText(this, "添加优惠券", "类型(fixed/discount):", QLineEdit::Normal, "fixed", &ok);
    if (!ok) return;
    double value = QInputDialog::getDouble(this, "添加优惠券", "面值:", 5, 0, 100, 2, &ok);
    if (!ok) return;
    double minOrder = QInputDialog::getDouble(this, "添加优惠券", "满减金额:", 0, 0, 10000, 2, &ok);
    if (!ok) return;
    QString validFrom = QInputDialog::getText(this, "添加优惠券", "有效期开始(YYYY-MM-DD):", QLineEdit::Normal, QDate::currentDate().toString("yyyy-MM-dd"), &ok);
    if (!ok) return;
    QString validTo = QInputDialog::getText(this, "添加优惠券", "有效期结束(YYYY-MM-DD):", QLineEdit::Normal, QDate::currentDate().addDays(30).toString("yyyy-MM-dd"), &ok);
    if (!ok) return;
    int quantity = QInputDialog::getInt(this, "添加优惠券", "库存数量:", 100, 1, 9999, 1, &ok);
    if (!ok) return;

    QJsonObject req;
    req["cmd"] = CMD::COUPON_ADD;
    QJsonObject data;
    data["code"] = code;
    data["type"] = type;
    data["value"] = value;
    data["min_order"] = minOrder;
    data["valid_from"] = validFrom;
    data["valid_to"] = validTo;
    data["quantity"] = quantity;
    req["data"] = data;
    m_net->sendRequest(req);
    loadCoupons();
}

void CouponManagementWidget::onEdit()
{
    int row = table->currentRow();
    if (row < 0) return;
    int id = table->item(row, 0)->text().toInt();
    double newValue = QInputDialog::getDouble(this, "编辑", "新面值:", table->item(row, 3)->text().toDouble(), 0, 100, 2);
    QJsonObject req;
    req["cmd"] = CMD::COUPON_EDIT;
    QJsonObject data;
    data["id"] = id;
    data["value"] = newValue;
    req["data"] = data;
    m_net->sendRequest(req);
    loadCoupons();
}

void CouponManagementWidget::onDelete()
{
    int row = table->currentRow();
    if (row < 0) return;
    int id = table->item(row, 0)->text().toInt();
    if (QMessageBox::question(this, "确认", "删除该优惠券？") == QMessageBox::Yes) {
        QJsonObject req;
        req["cmd"] = CMD::COUPON_DELETE;
        QJsonObject data;
        data["id"] = id;
        req["data"] = data;
        m_net->sendRequest(req);
        loadCoupons();
    }
}
