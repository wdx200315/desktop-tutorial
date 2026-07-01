import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import "../components"

Page {
    title: "我的预约"
    property var model: ListModel { }
    property int currentTab: 0

    Component.onCompleted: {
        refresh()
    }

    function refresh() {
        if (!net.connected) {
            toast.show("未连接服务端")
            return
        }
        var req = {
            "cmd": "3004",
            "data": { "action": "list" },
            "token": session.token
        }
        net.sendRequest(req)
    }

    Connections {
        target: net
        function onResponseReceived(resp) {
            if (resp.cmd === "3004" && resp.status === "ok") {
                model.clear()
                var list = resp.data.list
                if (list) {
                    for (var i = 0; i < list.length; ++i) {
                        model.append(list[i])
                    }
                }
            }
        }
        function onErrorOccurred(msg) {
            toast.show("网络错误: " + msg)
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
                    text: "我的预约"
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

        Flickable {
            width: parent.width
            height: parent.height - 56
            contentHeight: contentColumn.height
            clip: true

            Column {
                id: contentColumn
                spacing: 0

                // 加载中
                Rectangle {
                    width: parent.width
                    height: 40
                    color: "#F5F7FA"
                    visible: model.count === 0
                    Text {
                        text: "加载中..."
                        color: "#999"
                        anchors.centerIn: parent
                    }
                }

                // 空状态
                Rectangle {
                    width: parent.width
                    height: 200
                    color: "#F5F7FA"
                    visible: model.count === 0
                    Column {
                        anchors.centerIn: parent
                        Text {
                            text: "📅"
                            font.pixelSize: 48
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        Text {
                            text: "暂无预约记录"
                            font.pixelSize: 14
                            color: "#999"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        Text {
                            text: "去预约一个充电桩吧"
                            font.pixelSize: 12
                            color: "#BBB"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                }

                // 预约列表
                ListView {
                    width: parent.width
                    height: contentHeight
                    model: model
                    clip: true

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

                        Column {
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 8

                            Row {
                                width: parent.width
                                Text {
                                    text: "📍 充电桩 #" + (model.charger_id || model.id)
                                    font.bold: true
                                    font.pixelSize: 15
                                    color: "#333333"
                                }

                                Item { Layout.fillWidth: true }

                                Text {
                                    text: model.status === "pending" ? "待使用" :
                                          model.status === "completed" ? "已完成" :
                                          model.status === "cancelled" ? "已取消" : "未知"
                                    font.pixelSize: 13
                                    color: model.status === "pending" ? "#FF9800" :
                                           model.status === "completed" ? "#4CAF50" :
                                           model.status === "cancelled" ? "#999" : "#999"
                                }
                            }

                            Row {
                                width: parent.width
                                spacing: 16
                                Text {
                                    text: "📅 " + (model.reserve_time || "未知时间")
                                    font.pixelSize: 13
                                    color: "#666"
                                }
                                Text {
                                    text: "⚡ " + (model.mode === "auto" ? "自动充满" : "定时停止")
                                    font.pixelSize: 13
                                    color: "#4A90D9"
                                }
                            }

                            Row {
                                width: parent.width
                                visible: model.status === "pending"
                                spacing: 12

                                Item { Layout.fillWidth: true }

                                Rectangle {
                                    width: 80
                                    height: 30
                                    radius: 15
                                    color: "#FFEBEE"
                                    border.width: 1
                                    border.color: "#E74C3C"

                                    Text {
                                        text: "取消预约"
                                        color: "#E74C3C"
                                        font.pixelSize: 11
                                        anchors.centerIn: parent
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: {
                                            var req = {
                                                "cmd": "3004",
                                                "data": { "action": "cancel", "id": model.id },
                                                "token": session.token
                                            }
                                            net.sendRequest(req)
                                            refresh()
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                Item { height: 100 }
            }
        }
    }

    header: null
}
