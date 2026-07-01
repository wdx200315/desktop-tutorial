import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import "../components"

Page {
    title: "充电统计"
    property var dailyData: ({})
    property var monthlyData: ({})
    property string selectedDate: new Date().toISOString().split('T')[0]
    property string selectedMonth: new Date().toISOString().split('T')[0].substring(0, 7)

    Component.onCompleted: {
        loadDailyReport(selectedDate)
        loadMonthlyReport(selectedMonth)
    }

    function loadDailyReport(date) {
        if (!net.connected) return
        net.getDailyReport(date)
    }

    function loadMonthlyReport(month) {
        if (!net.connected) return
        net.getMonthlyReport(month)
    }

    Connections {
        target: net
        function onResponseReceived(resp) {
            if (resp.cmd === "7001" && resp.status === "ok") {
                dailyData = resp.data || {}
                updateDailyUI()
            }
            if (resp.cmd === "7002" && resp.status === "ok") {
                monthlyData = resp.data || {}
                updateMonthlyUI()
            }
        }
    }

    function updateDailyUI() {
        lblDailyOrders.text = dailyData.order_count || "0"
        lblDailyEnergy.text = (dailyData.total_energy || "0") + " kWh"
        lblDailyAmount.text = "¥" + (dailyData.total_amount || "0.00")
        lblDailyTime.text = (dailyData.total_duration || "0") + " 分钟"
    }

    function updateMonthlyUI() {
        lblMonthlyOrders.text = monthlyData.order_count || "0"
        lblMonthlyEnergy.text = (monthlyData.total_energy || "0") + " kWh"
        lblMonthlyAmount.text = "¥" + (monthlyData.total_amount || "0.00")
        lblMonthlyDays.text = (monthlyData.charging_days || "0") + " 天"
    }

    Column {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 16

        // 今日概览卡片
        Rectangle {
            width: parent.width
            height: 160
            color: "#1A73E8"
            radius: 12

            Column {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 8

                Row {
                    width: parent.width
                    spacing: 8
                    Text {
                        text: "📊 今日统计"
                        color: "white"
                        font.bold: true
                        font.pixelSize: 18
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Item { Layout.fillWidth: true }
                    Text {
                        text: selectedDate
                        color: "#BBDEFB"
                        font.pixelSize: 12
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                Row {
                    width: parent.width
                    spacing: 8

                    // 订单数
                    Rectangle {
                        width: (parent.width - 8) / 2
                        height: 55
                        color: "#1565C0"
                        radius: 8
                        Column {
                            anchors.centerIn: parent
                            Text {
                                id: lblDailyOrders
                                text: "0"
                                color: "white"
                                font.bold: true
                                font.pixelSize: 24
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                            Text {
                                text: "充电次数"
                                color: "#BBDEFB"
                                font.pixelSize: 11
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                        }
                    }

                    // 用电量
                    Rectangle {
                        width: (parent.width - 8) / 2
                        height: 55
                        color: "#1565C0"
                        radius: 8
                        Column {
                            anchors.centerIn: parent
                            Text {
                                id: lblDailyEnergy
                                text: "0 kWh"
                                color: "white"
                                font.bold: true
                                font.pixelSize: 20
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                            Text {
                                text: "用电量"
                                color: "#BBDEFB"
                                font.pixelSize: 11
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                        }
                    }
                }

                Row {
                    width: parent.width
                    spacing: 8

                    // 消费金额
                    Rectangle {
                        width: (parent.width - 8) / 2
                        height: 55
                        color: "#1565C0"
                        radius: 8
                        Column {
                            anchors.centerIn: parent
                            Text {
                                id: lblDailyAmount
                                text: "¥0.00"
                                color: "#FFEB3B"
                                font.bold: true
                                font.pixelSize: 20
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                            Text {
                                text: "消费金额"
                                color: "#BBDEFB"
                                font.pixelSize: 11
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                        }
                    }

                    // 充电时长
                    Rectangle {
                        width: (parent.width - 8) / 2
                        height: 55
                        color: "#1565C0"
                        radius: 8
                        Column {
                            anchors.centerIn: parent
                            Text {
                                id: lblDailyTime
                                text: "0 分钟"
                                color: "white"
                                font.bold: true
                                font.pixelSize: 20
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                            Text {
                                text: "充电时长"
                                color: "#BBDEFB"
                                font.pixelSize: 11
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                        }
                    }
                }
            }
        }

        // 本月统计
        Rectangle {
            width: parent.width
            height: 130
            color: "#FAFAFA"
            radius: 12
            border.width: 1
            border.color: "#E0E0E0"

            Column {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 8

                Text {
                    text: "📅 本月统计 (" + selectedMonth + ")"
                    font.bold: true
                    font.pixelSize: 16
                }

                Row {
                    width: parent.width
                    spacing: 12

                    Column {
                        Text {
                            id: lblMonthlyOrders
                            text: "0"
                            font.bold: true
                            font.pixelSize: 18
                        }
                        Text {
                            text: "充电次数"
                            color: "#757575"
                            font.pixelSize: 11
                        }
                    }

                    Column {
                        Text {
                            id: lblMonthlyEnergy
                            text: "0 kWh"
                            font.bold: true
                            font.pixelSize: 18
                        }
                        Text {
                            text: "总用电量"
                            color: "#757575"
                            font.pixelSize: 11
                        }
                    }

                    Column {
                        Text {
                            id: lblMonthlyAmount
                            text: "¥0.00"
                            font.bold: true
                            font.pixelSize: 18
                            color: "#EF5350"
                        }
                        Text {
                            text: "总消费"
                            color: "#757575"
                            font.pixelSize: 11
                        }
                    }

                    Column {
                        Text {
                            id: lblMonthlyDays
                            text: "0 天"
                            font.bold: true
                            font.pixelSize: 18
                        }
                        Text {
                            text: "充电天数"
                            color: "#757575"
                            font.pixelSize: 11
                        }
                    }
                }
            }
        }

        // 充电趋势提示
        Rectangle {
            width: parent.width
            height: 60
            color: "#FFF3E0"
            radius: 8
            Row {
                anchors.centerIn: parent
                spacing: 12
                Text {
                    text: "📈"
                    font.pixelSize: 24
                    anchors.verticalCenter: parent.verticalCenter
                }
                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    Text {
                        text: "充电趋势"
                        font.bold: true
                        font.pixelSize: 14
                    }
                    Text {
                        text: "查看历史充电数据，了解您的充电习惯"
                        color: "#757575"
                        font.pixelSize: 11
                    }
                }
            }
        }

        // 刷新按钮
        Button {
            text: "刷新数据"
            width: parent.width
            onClicked: {
                loadDailyReport(selectedDate)
                loadMonthlyReport(selectedMonth)
                toast.show("数据已刷新")
            }
        }

        Item { Layout.fillHeight: true }
    }

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            ToolButton {
                text: "←"
                onClicked: stack.pop()
            }
            Label {
                text: "充电统计"
                font.bold: true
                Layout.fillWidth: true
            }
        }
    }
}
