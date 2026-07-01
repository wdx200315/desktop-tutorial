import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import "../components"

Page {
    title: "费率详情"
    property var rateList: ListModel { }

    Component.onCompleted: {
        refresh()
    }

    function refresh() {
        if (!net.connected) {
            toast.show("未连接服务端")
            return
        }
        net.getRateList()
    }

    Connections {
        target: net
        function onResponseReceived(resp) {
            if (resp.cmd === "4001" && resp.status === "ok") {
                rateList.clear()
                var list = resp.data
                if (Array.isArray(list)) {
                    for (var i = 0; i < list.length; ++i) {
                        rateList.append(list[i])
                    }
                }
                toast.show("共 " + rateList.count + " 个费率")
            }
            if (resp.status === "error") {
                toast.show("错误: " + resp.message)
            }
        }
    }

    Column {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 16

        // 费率说明卡片
        Rectangle {
            width: parent.width
            height: 80
            color: "#E3F2FD"
            radius: 8
            Column {
                anchors.centerIn: parent
                spacing: 8
                Text {
                    text: "💡 费率说明"
                    font.bold: true
                    font.pixelSize: 14
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Text {
                    text: "充电费用 = 电费 + 服务费"
                    color: "#1565C0"
                    font.pixelSize: 12
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
        }

        // 费率列表
        ListView {
            width: parent.width
            height: 500
            model: rateList
            spacing: 8
            clip: true
            delegate: Rectangle {
                width: parent.width
                height: 90
                color: "#FAFAFA"
                radius: 8
                Column {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 6

                    Row {
                        width: parent.width
                        Text {
                            text: model.mode || model.name || "标准充"
                            font.bold: true
                            font.pixelSize: 16
                        }
                        Text {
                            text: (model.active !== undefined && model.active) ? "✅ 启用" : "⏸️ 停用"
                            color: (model.active !== undefined && model.active) ? "#4CAF50" : "#9E9E9E"
                            font.pixelSize: 12
                            anchors.right: parent.right
                        }
                    }

                    Row {
                        width: parent.width
                        spacing: 20
                        Column {
                            Text { text: "电费单价"; color: "#757575"; font.pixelSize: 11 }
                            Text {
                                text: "¥" + ((parseFloat(model.electricity_fee) || parseFloat(model.price_kwh) || 0).toFixed(2)) + "/度"
                                color: "#1A73E8"
                                font.pixelSize: 14
                                font.bold: true
                            }
                        }
                        Column {
                            Text { text: "服务费"; color: "#757575"; font.pixelSize: 11 }
                            Text {
                                text: "¥" + ((parseFloat(model.service_fee) || 0).toFixed(2)) + "/度"
                                color: "#FF6D00"
                                font.pixelSize: 14
                                font.bold: true
                            }
                        }
                        Column {
                            Text { text: "时段"; color: "#757575"; font.pixelSize: 11 }
                            Text {
                                text: model.time_range || (model.start_time ? model.start_time + "-" + model.end_time : "全天")
                                color: "#333333"
                                font.pixelSize: 12
                            }
                        }
                    }

                    Text {
                        text: "总计: ¥" + ((parseFloat(model.electricity_fee || model.price_kwh || 0) + parseFloat(model.service_fee || 0)).toFixed(2)) + "/度"
                        color: "#4CAF50"
                        font.pixelSize: 13
                        font.bold: true
                    }
                }
            }
        }
    }

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            ToolButton {
                text: "←"
                onClicked: stack.pop()
            }
            Label {
                text: "费率详情"
                font.bold: true
                Layout.fillWidth: true
            }
            ToolButton {
                text: "刷新"
                onClicked: refresh()
            }
        }
    }
}
