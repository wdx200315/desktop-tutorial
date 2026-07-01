import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import "../components"

Page {
    title: "我的订单"
    property var orderModel: ListModel { }
    property bool isLoading: false
    property int currentTabIndex: 0

    // 标签页数据
    property var tabLabels: ["全部", "进行中", "已完成", "待支付"]

    Component.onCompleted: {
        loadData()
    }

    function loadData() {
        if (net.connected) {
            isLoading = true
            net.getOrderList(1, 50, "")
        } else {
            toast.showWarning("网络未连接，请检查网络后重试")
        }
    }

    function refresh() {
        loadData()
    }

    function filterOrders() {
        orderModel.clear()
        var allOrders = orderModel_All
        for (var i = 0; i < allOrders.count; ++i) {
            var order = allOrders.get(i)
            if (currentTabIndex === 0) {
                orderModel.append(order)
            } else if (currentTabIndex === 1 && order.status === "charging") {
                orderModel.append(order)
            } else if (currentTabIndex === 2 && order.status === "completed") {
                orderModel.append(order)
            } else if (currentTabIndex === 3 && order.status === "pending") {
                orderModel.append(order)
            }
        }
    }

    // 存储所有订单
    property var orderModel_All: ListModel { }

    Connections {
        target: net
        function onResponseReceived(resp) {
            isLoading = false
            if (resp.cmd === "5001" && resp.status === "ok") {
                orderModel_All.clear()
                var list = resp.data.list
                if (list) {
                    for (var i = 0; i < list.length; ++i) {
                        orderModel_All.append(list[i])
                    }
                }
                filterOrders()
                if (orderModel.count === 0) {
                    toast.showInfo("暂无订单数据")
                }
            }
        }
        function onErrorOccurred(msg) {
            isLoading = false
            toast.showError("加载失败: " + msg)
        }
    }

    Column {
        anchors.fill: parent
        spacing: 0

        // ========== 顶部标题栏 ==========
        Rectangle {
            width: parent.width
            height: 56
            color: "#4A90D9"

            Row {
                anchors.fill: parent
                anchors.leftMargin: 16
                spacing: 8

                Text {
                    text: "←"
                    color: "white"
                    font.pixelSize: 24
                    anchors.verticalCenter: parent.verticalCenter
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: root.stackView.pop()
                }

                Text {
                    text: "我的订单"
                    color: "white"
                    font.bold: true
                    font.pixelSize: 18
                    anchors.verticalCenter: parent.verticalCenter
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: "🔄"
                    color: "white"
                    font.pixelSize: 18
                    anchors.verticalCenter: parent.verticalCenter
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: refresh()
                }
            }
        }

        // ========== 标签页 ==========
        Rectangle {
            width: parent.width
            height: 44
            color: "white"

            Row {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16

                Repeater {
                    model: tabLabels.length

                    Rectangle {
                        width: parent.width / tabLabels.length
                        height: parent.height
                        color: "transparent"

                        Column {
                            anchors.centerIn: parent
                            spacing: 4

                            Text {
                                text: tabLabels[index]
                                font.pixelSize: 14
                                color: currentTabIndex === index ? "#4A90D9" : "#666666"
                                font.bold: currentTabIndex === index
                                anchors.horizontalCenter: parent.horizontalCenter
                            }

                            Rectangle {
                                width: 24
                                height: 3
                                radius: 1.5
                                color: currentTabIndex === index ? "#4A90D9" : "transparent"
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                currentTabIndex = index
                                filterOrders()
                            }
                        }
                    }
                }
            }
        }

        // 分隔线
        Rectangle {
            width: parent.width
            height: 1
            color: "#F0F0F0"
        }

        // ========== 内容区域 ==========
        Frame {
            width: parent.width
            height: parent.height - 100
            padding: 0
            background: Rectangle { color: "#F5F7FA" }

            // 加载状态
            LoadingOverlay {
                visible: isLoading
                loading: isLoading
                loadingText: "加载订单中..."
                anchors.fill: parent
            }

            // 空状态
            EmptyState {
                visible: orderModel.count === 0 && !isLoading
                emptyIcon: "📋"
                emptyTitle: "暂无订单"
                emptyDescription: currentTabIndex === 0 ? "您还没有任何订单" : 
                                 currentTabIndex === 1 ? "当前没有进行中的订单" :
                                 currentTabIndex === 2 ? "暂无已完成订单" : "暂无待支付订单"
                emptyActionText: currentTabIndex === 0 ? "去充电" : ""
                emptyAction: function() {
                    root.stackView.push(scanPageComponent)
                }
                anchors.fill: parent
            }

            // 订单列表
            ListView {
                visible: orderModel.count > 0
                width: parent.width
                height: parent.height
                model: orderModel
                clip: true
                spacing: 0

                // 下拉刷新
                PullToRefresh {
                    id: pullToRefresh
                    refreshing: isLoading
                    onRefresh: refresh()
                }

                delegate: Rectangle {
                    width: parent.width
                    height: 100
                    color: "white"

                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#F0F0F0"
                        anchors.bottom: parent.bottom
                    }

                    // 状态标签
                    Rectangle {
                        width: 60
                        height: 22
                        radius: 11
                        anchors.top: parent.top
                        anchors.right: parent.right
                        anchors.topMargin: 12
                        anchors.rightMargin: 12
                        color: model.status === "completed" ? "#E8F5E9" :
                               model.status === "charging" ? "#FFF3E0" :
                               model.status === "pending" ? "#FFEBEE" : "#F5F5F5"
                        
                        Text {
                            text: model.status === "completed" ? "已完成" :
                                  model.status === "charging" ? "充电中" :
                                  model.status === "pending" ? "待支付" : "未知"
                            font.pixelSize: 11
                            color: model.status === "completed" ? "#4CAF50" :
                                   model.status === "charging" ? "#FF9800" :
                                   model.status === "pending" ? "#E53935" : "#999"
                            anchors.centerIn: parent
                        }
                    }

                    Column {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 6

                        // 充电桩信息
                        Row {
                            width: parent.width
                            Text {
                                text: "⚡ " + (model.charger_name || ("充电桩 #" + model.charger_id))
                                font.bold: true
                                font.pixelSize: 15
                                color: "#333333"
                            }
                        }

                        // 位置和电量
                        Row {
                            width: parent.width
                            spacing: 16
                            Text {
                                text: "📍 " + (model.location || "位置未知")
                                font.pixelSize: 12
                                color: "#666"
                            }
                            Text {
                                text: "⚡ " + (model.energy || "0") + " kWh"
                                font.pixelSize: 12
                                color: "#4A90D9"
                                font.bold: true
                            }
                        }

                        // 时间
                        Row {
                            width: parent.width
                            Text {
                                text: model.start_time || ""
                                font.pixelSize: 11
                                color: "#999"
                            }

                            Item { Layout.fillWidth: true }

                            Text {
                                text: "¥" + (model.amount || "0.00")
                                font.bold: true
                                font.pixelSize: 16
                                color: "#FF9800"
                            }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            root.stackView.push(orderDetailPageComponent, { "orderId": model.id })
                        }
                    }
                }

                // 底部间距
                footer: Item { height: 20 }
            }
        }

        // 底部间距
        Item { height: 10 }
    }

    header: null
}
