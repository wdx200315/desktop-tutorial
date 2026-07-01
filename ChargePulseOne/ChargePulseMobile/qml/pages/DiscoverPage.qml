import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import "../components"

Page {
    title: "发现"
    property var recommendList: ListModel { }
    property var activityList: ListModel { }
    property bool isLoading: false

    Component.onCompleted: {
        loadData()
    }

    function loadData() {
        if (net.connected) {
            isLoading = true
            net.getChargerList(1, 10, "online")
        } else {
            toast.showWarning("网络未连接")
        }
    }

    function refresh() {
        loadData()
    }

    Connections {
        target: net
        function onResponseReceived(resp) {
            isLoading = false
            if (resp.cmd === "2001" && resp.status === "ok") {
                recommendList.clear()
                var list = resp.data || []
                for (var i = 0; i < list.length; ++i) {
                    recommendList.append(list[i])
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

        // ========== 顶部搜索栏 ==========
        Rectangle {
            width: parent.width
            height: 56
            color: "#4A90D9"

            Row {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                // 搜索框
                Rectangle {
                    width: parent.width - 24
                    height: 36
                    radius: 18
                    color: "white"

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        spacing: 8
                        verticalCenter: parent.verticalCenter

                        Text {
                            text: "🔍"
                            font.pixelSize: 16
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Text {
                            text: "搜索充电桩、站点..."
                            color: "#999999"
                            font.pixelSize: 14
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: toast.showInfo("搜索功能开发中")
                    }
                }
            }
        }

        Flickable {
            width: parent.width
            height: parent.height - 56
            contentHeight: contentColumn.height
            clip: true

            Column {
                id: contentColumn
                spacing: 0

                // ========== 加载状态 ==========
                LoadingOverlay {
                    visible: isLoading
                    loading: isLoading
                    loadingText: "加载中..."
                    width: parent.width
                    height: 200
                }

                // ========== Banner轮播区 ==========
                Rectangle {
                    width: parent.width
                    height: 160
                    color: "#4A90D9"

                    Column {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12

                        Text {
                            text: "⚡ ChargePulse"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 24
                        }

                        Text {
                            text: "让充电更简单"
                            color: "#BBDEFB"
                            font.pixelSize: 14
                        }

                        // 优惠信息
                        Rectangle {
                            width: parent.width
                            height: 50
                            radius: 12
                            color: "#FFFFFF22"

                            Row {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 16

                                Text {
                                    text: "🎁"
                                    font.pixelSize: 24
                                    anchors.verticalCenter: parent.verticalCenter
                                }

                                Column {
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: 2

                                    Text {
                                        text: "新人专享"
                                        color: "#FFD700"
                                        font.bold: true
                                        font.pixelSize: 14
                                    }

                                    Text {
                                        text: "首充送20元优惠券"
                                        color: "white"
                                        font.pixelSize: 12
                                    }
                                }

                                Item { Layout.fillWidth: true }

                                Rectangle {
                                    width: 70
                                    height: 28
                                    radius: 14
                                    color: "#FFD700"
                                    anchors.verticalCenter: parent.verticalCenter

                                    Text {
                                        text: "立即领取"
                                        color: "#333"
                                        font.bold: true
                                        font.pixelSize: 12
                                        anchors.centerIn: parent
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: {
                                            toast.showSuccess("优惠券已领取")
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // ========== 快捷功能区 ==========
                Rectangle {
                    width: parent.width
                    color: "white"
                    Column {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 16

                        Text {
                            text: "快捷服务"
                            font.bold: true
                            font.pixelSize: 16
                            color: "#333"
                        }

                        Row {
                            width: parent.width
                            spacing: 12

                            // 扫码充电
                            Rectangle {
                                width: (parent.width - 36) / 4
                                height: 80
                                radius: 12
                                color: "#E3F2FD"

                                Column {
                                    anchors.centerIn: parent
                                    spacing: 6
                                    Text {
                                        text: "📷"
                                        font.pixelSize: 28
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                    Text {
                                        text: "扫码充电"
                                        font.pixelSize: 12
                                        color: "#333"
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: root.stackView.push(scanPageComponent)
                                }
                            }

                            // 预约充电
                            Rectangle {
                                width: (parent.width - 36) / 4
                                height: 80
                                radius: 12
                                color: "#FFF3E0"

                                Column {
                                    anchors.centerIn: parent
                                    spacing: 6
                                    Text {
                                        text: "📅"
                                        font.pixelSize: 28
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                    Text {
                                        text: "预约充电"
                                        font.pixelSize: 12
                                        color: "#333"
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: root.stackView.push(reservationPageComponent)
                                }
                            }

                            // 附近站点
                            Rectangle {
                                width: (parent.width - 36) / 4
                                height: 80
                                radius: 12
                                color: "#E8F5E9"

                                Column {
                                    anchors.centerIn: parent
                                    spacing: 6
                                    Text {
                                        text: "📍"
                                        font.pixelSize: 28
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                    Text {
                                        text: "附近站点"
                                        font.pixelSize: 12
                                        color: "#333"
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        toast.showInfo("附近站点功能开发中")
                                    }
                                }
                            }

                            // 充电记录
                            Rectangle {
                                width: (parent.width - 36) / 4
                                height: 80
                                radius: 12
                                color: "#F3E5F5"

                                Column {
                                    anchors.centerIn: parent
                                    spacing: 6
                                    Text {
                                        text: "📊"
                                        font.pixelSize: 28
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                    Text {
                                        text: "充电记录"
                                        font.pixelSize: 12
                                        color: "#333"
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: root.stackView.push(orderPageComponent)
                                }
                            }
                        }
                    }
                }

                // ========== 推荐充电桩 ==========
                Rectangle {
                    width: parent.width
                    color: "#F5F7FA"
                    Column {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12

                        Row {
                            width: parent.width

                            Text {
                                text: "🔥 推荐充电桩"
                                font.bold: true
                                font.pixelSize: 16
                                color: "#333"
                            }

                            Item { Layout.fillWidth: true }

                            Text {
                                text: "查看全部 >"
                                color: "#4A90D9"
                                font.pixelSize: 13
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: toast.showInfo("更多充电桩")
                            }
                        }

                        // 空状态
                        EmptyState {
                            visible: recommendList.count === 0 && !isLoading
                            emptyIcon: "🔌"
                            emptyTitle: "暂无推荐"
                            emptyDescription: "暂无可用充电桩"
                            width: parent.width
                            height: 150
                        }

                        // 推荐列表
                        ListView {
                            visible: recommendList.count > 0
                            width: parent.width
                            height: recommendList.count * 120
                            model: recommendList
                            clip: true
                            spacing: 12

                            delegate: Rectangle {
                                width: parent.width
                                height: 110
                                radius: 12
                                color: "white"

                                Column {
                                    anchors.fill: parent
                                    anchors.margins: 16
                                    spacing: 8

                                    Row {
                                        width: parent.width

                                        Text {
                                            text: "⚡ " + (model.name || ("充电桩 #" + model.id))
                                            font.bold: true
                                            font.pixelSize: 15
                                            color: "#333"
                                        }

                                        Item { Layout.fillWidth: true }

                                        Rectangle {
                                            width: 50
                                            height: 22
                                            radius: 11
                                            color: model.status === "online" ? "#E8F5E9" : "#FFEBEE"

                                            Text {
                                                text: model.status === "online" ? "空闲" : "占用"
                                                font.pixelSize: 11
                                                color: model.status === "online" ? "#4CAF50" : "#E53935"
                                                anchors.centerIn: parent
                                            }
                                        }
                                    }

                                    Row {
                                        width: parent.width
                                        Text {
                                            text: "📍 " + (model.location || "位置未知")
                                            font.pixelSize: 12
                                            color: "#666"
                                        }
                                    }

                                    Row {
                                        width: parent.width
                                        spacing: 16

                                        Text {
                                            text: "⚡ " + (model.power_kw || model.power || 60) + " kW"
                                            font.pixelSize: 13
                                            color: "#4A90D9"
                                            font.bold: true
                                        }

                                        Text {
                                            text: "💰 ¥" + ((model.price_kwh || model.price || 1.2)).toFixed(2) + "/度"
                                            font.pixelSize: 13
                                            color: "#FF9800"
                                            font.bold: true
                                        }

                                        Item { Layout.fillWidth: true }

                                        Rectangle {
                                            width: 70
                                            height: 30
                                            radius: 15
                                            color: model.status === "online" ? "#4CAF50" : "#CCCCCC"

                                            Text {
                                                text: "去充电"
                                                color: "white"
                                                font.bold: true
                                                font.pixelSize: 12
                                                anchors.centerIn: parent
                                            }

                                            MouseArea {
                                                anchors.fill: parent
                                                enabled: model.status === "online"
                                                onClicked: {
                                                    root.stackView.push(chargerDetailPageComponent, { "chargerId": model.id })
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // ========== 优惠活动 ==========
                Rectangle {
                    width: parent.width
                    color: "white"
                    Column {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12

                        Text {
                            text: "🎁 优惠活动"
                            font.bold: true
                            font.pixelSize: 16
                            color: "#333"
                        }

                        // 活动卡片
                        Rectangle {
                            width: parent.width
                            height: 120
                            radius: 12
                            color: "#4A90D9"

                            Row {
                                anchors.fill: parent
                                anchors.margins: 16
                                spacing: 16

                                Column {
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: 8

                                    Text {
                                        text: "限时优惠"
                                        color: "#FFD700"
                                        font.bold: true
                                        font.pixelSize: 18
                                    }

                                    Text {
                                        text: "谷时充电低至0.3元/度"
                                        color: "white"
                                        font.pixelSize: 13
                                    }

                                    Text {
                                        text: "每晚0点-6点专享"
                                        color: "#BBDEFB"
                                        font.pixelSize: 11
                                    }
                                }

                                Item { Layout.fillWidth: true }

                                Rectangle {
                                    width: 80
                                    height: 32
                                    radius: 16
                                    color: "#FFD700"
                                    anchors.verticalCenter: parent.verticalCenter

                                    Text {
                                        text: "去看看"
                                        color: "#333"
                                        font.bold: true
                                        font.size: 13
                                        anchors.centerIn: parent
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: {
                                            toast.showInfo("查看峰谷电价详情")
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // ========== 使用指南 ==========
                Rectangle {
                    width: parent.width
                    color: "#F5F7FA"
                    Column {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12

                        Text {
                            text: "📖 使用指南"
                            font.bold: true
                            font.pixelSize: 16
                            color: "#333"
                        }

                        Row {
                            width: parent.width
                            spacing: 12

                            Rectangle {
                                width: (parent.width - 12) / 2
                                height: 100
                                radius: 12
                                color: "white"

                                Column {
                                    anchors.centerIn: parent
                                    spacing: 8

                                    Text {
                                        text: "1️⃣"
                                        font.pixelSize: 28
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }

                                    Text {
                                        text: "扫描充电桩二维码"
                                        font.pixelSize: 13
                                        color: "#333"
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                }
                            }

                            Rectangle {
                                width: (parent.width - 12) / 2
                                height: 100
                                radius: 12
                                color: "white"

                                Column {
                                    anchors.centerIn: parent
                                    spacing: 8

                                    Text {
                                        text: "2️⃣"
                                        font.pixelSize: 28
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }

                                    Text {
                                        text: "连接充电枪开始充电"
                                        font.pixelSize: 13
                                        color: "#333"
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                }
                            }
                        }

                        Row {
                            width: parent.width
                            spacing: 12

                            Rectangle {
                                width: (parent.width - 12) / 2
                                height: 100
                                radius: 12
                                color: "white"

                                Column {
                                    anchors.centerIn: parent
                                    spacing: 8

                                    Text {
                                        text: "3️⃣"
                                        font.pixelSize: 28
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }

                                    Text {
                                        text: "实时监控充电状态"
                                        font.pixelSize: 13
                                        color: "#333"
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                }
                            }

                            Rectangle {
                                width: (parent.width - 12) / 2
                                height: 100
                                radius: 12
                                color: "white"

                                Column {
                                    anchors.centerIn: parent
                                    spacing: 8

                                    Text {
                                        text: "4️⃣"
                                        font.pixelSize: 28
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }

                                    Text {
                                        text: "充满自动停止结算"
                                        font.pixelSize: 13
                                        color: "#333"
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                }
                            }
                        }
                    }
                }

                // 底部间距
                Item { height: 100 }
            }
        }
    }

    header: null
}
