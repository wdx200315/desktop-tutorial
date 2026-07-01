import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import "../components"

Page {
    title: "会员中心"
    property var memberInfo: ({
        level: "普通会员",
        points: 0,
        discount: 0,
        nextLevel: "",
        needPoints: 0
    })
    property var memberLevels: ListModel { }

    Component.onCompleted: {
        refresh()
    }

    function refresh() {
        if (!net.connected) {
            toast.show("未连接服务端")
            return
        }
        net.getMemberStats()
        net.getMemberLevels()
    }

    Connections {
        target: net
        function onResponseReceived(resp) {
            if (resp.cmd === "9202" && resp.status === "ok") {
                memberInfo = resp.data || memberInfo
                updateMemberUI()
            }
            if (resp.cmd === "9201" && resp.status === "ok") {
                memberLevels.clear()
                var list = resp.data
                if (Array.isArray(list)) {
                    for (var i = 0; i < list.length; ++i) {
                        memberLevels.append(list[i])
                    }
                }
            }
        }
    }

    function updateMemberUI() {
        lblLevel.text = memberInfo.level || "普通会员"
        lblPoints.text = (memberInfo.points || 0).toString()
        lblDiscount.text = ((memberInfo.discount || 0) * 100).toFixed(0) + "%"
        lblNextLevel.text = "下一级: " + (memberInfo.next_level || "暂无")
        lblNeedPoints.text = "还需 " + (memberInfo.need_points || 0) + " 积分"

        // 更新进度条
        var current = memberInfo.points || 0
        var need = memberInfo.need_points || 100
        progressBar.value = Math.min(current / (current + need), 1.0)
    }

    Column {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 16

        // 会员卡背景 - 使用垂直渐变
        Rectangle {
            width: parent.width
            height: 180
            radius: 16
            Rectangle {
                anchors.fill: parent
                radius: 16
                gradient: Gradient {
                    orientation: Gradient.Vertical
                    GradientStop { position: 0.0; color: "#1A73E8" }
                    GradientStop { position: 1.0; color: "#0D47A1" }
                }
            }

            Column {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 12

                Row {
                    width: parent.width
                    spacing: 8
                    Text {
                        text: "⚡"
                        font.pixelSize: 24
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: "ChargePulse 会员"
                        color: "white"
                        font.bold: true
                        font.pixelSize: 16
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: "#FFFFFF33"
                }

                Row {
                    width: parent.width
                    spacing: 20

                    Column {
                        Text {
                            id: lblLevel
                            text: "普通会员"
                            color: "#FFEB3B"
                            font.bold: true
                            font.pixelSize: 24
                        }
                        Text {
                            text: "会员等级"
                            color: "#BBDEFB"
                            font.pixelSize: 12
                        }
                    }

                    Rectangle {
                        width: 1
                        height: 50
                        color: "#FFFFFF33"
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Column {
                        Text {
                            id: lblDiscount
                            text: "100%"
                            color: "#FFEB3B"
                            font.bold: true
                            font.pixelSize: 24
                        }
                        Text {
                            text: "充电折扣"
                            color: "#BBDEFB"
                            font.pixelSize: 12
                        }
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: "#FFFFFF33"
                }

                Row {
                    width: parent.width
                    spacing: 8
                    Text {
                        id: lblPoints
                        text: "0"
                        color: "white"
                        font.bold: true
                        font.pixelSize: 18
                    }
                    Text {
                        text: "积分"
                        color: "#BBDEFB"
                        font.pixelSize: 14
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                Text {
                    id: lblNextLevel
                    text: "下一级: 银卡会员"
                    color: "#BBDEFB"
                    font.pixelSize: 12
                }

                // 进度条
                ProgressBar {
                    id: progressBar
                    width: parent.width
                    height: 8
                    value: 0.3
                    background: Rectangle {
                        color: "#FFFFFF33"
                        radius: 4
                    }
                    contentItem: Rectangle {
                        color: "#FFEB3B"
                        radius: 4
                    }
                }

                Text {
                    id: lblNeedPoints
                    text: "还需 100 积分"
                    color: "#BBDEFB"
                    font.pixelSize: 11
                }
            }
        }

        // 会员权益
        Text {
            text: "🏆 会员权益"
            font.bold: true
            font.pixelSize: 16
        }

        Grid {
            columns: 2
            spacing: 12
            width: parent.width

            Rectangle {
                width: (parent.width - 12) / 2
                height: 80
                color: "#E8F5E9"
                radius: 8
                Column {
                    anchors.centerIn: parent
                    Text { text: "💰"; font.pixelSize: 24; anchors.horizontalCenter: parent.horizontalCenter }
                    Text { text: "充电折扣"; font.pixelSize: 12; anchors.horizontalCenter: parent.horizontalCenter }
                    Text { text: "最高 8 折"; color: "#4CAF50"; font.pixelSize: 11; anchors.horizontalCenter: parent.horizontalCenter }
                }
            }

            Rectangle {
                width: (parent.width - 12) / 2
                height: 80
                color: "#FFF3E0"
                radius: 8
                Column {
                    anchors.centerIn: parent
                    Text { text: "🎫"; font.pixelSize: 24; anchors.horizontalCenter: parent.horizontalCenter }
                    Text { text: "专属优惠券"; font.pixelSize: 12; anchors.horizontalCenter: parent.horizontalCenter }
                    Text { text: "每月限领"; color: "#FF9800"; font.pixelSize: 11; anchors.horizontalCenter: parent.horizontalCenter }
                }
            }

            Rectangle {
                width: (parent.width - 12) / 2
                height: 80
                color: "#E3F2FD"
                radius: 8
                Column {
                    anchors.centerIn: parent
                    Text { text: "⏰"; font.pixelSize: 24; anchors.horizontalCenter: parent.horizontalCenter }
                    Text { text: "优先预约"; font.pixelSize: 12; anchors.horizontalCenter: parent.horizontalCenter }
                    Text { text: "高峰期优先"; color: "#1E88E5"; font.pixelSize: 11; anchors.horizontalCenter: parent.horizontalCenter }
                }
            }

            Rectangle {
                width: (parent.width - 12) / 2
                height: 80
                color: "#FCE4EC"
                radius: 8
                Column {
                    anchors.centerIn: parent
                    Text { text: "🎁"; font.pixelSize: 24; anchors.horizontalCenter: parent.horizontalCenter }
                    Text { text: "积分兑换"; font.pixelSize: 12; anchors.horizontalCenter: parent.horizontalCenter }
                    Text { text: "积分抵现金"; color: "#E91E63"; font.pixelSize: 11; anchors.horizontalCenter: parent.horizontalCenter }
                }
            }
        }

        // 会员等级说明
        Text {
            text: "📋 等级规则"
            font.bold: true
            font.pixelSize: 16
        }

        Rectangle {
            width: parent.width
            height: 120
            color: "#FAFAFA"
            radius: 8
            border.width: 1
            border.color: "#E0E0E0"

            Column {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 6

                Row {
                    width: parent.width
                    spacing: 8
                    Rectangle {
                        width: 8
                        height: 8
                        radius: 4
                        color: "#9E9E9E"
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: "普通会员"
                        font.bold: true
                        font.pixelSize: 13
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: "注册即得"
                        color: "#757575"
                        font.pixelSize: 11
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                Row {
                    width: parent.width
                    spacing: 8
                    Rectangle {
                        width: 8
                        height: 8
                        radius: 4
                        color: "#C0C0C0"
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: "银卡会员"
                        font.bold: true
                        font.pixelSize: 13
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: "累计 500 积分"
                        color: "#757575"
                        font.pixelSize: 11
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                Row {
                    width: parent.width
                    spacing: 8
                    Rectangle {
                        width: 8
                        height: 8
                        radius: 4
                        color: "#FFD700"
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: "金卡会员"
                        font.bold: true
                        font.pixelSize: 13
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: "累计 2000 积分"
                        color: "#757575"
                        font.pixelSize: 11
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                Row {
                    width: parent.width
                    spacing: 8
                    Rectangle {
                        width: 8
                        height: 8
                        radius: 4
                        color: "#1A73E8"
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: "钻石会员"
                        font.bold: true
                        font.pixelSize: 13
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: "累计 5000 积分，专属服务"
                        color: "#757575"
                        font.pixelSize: 11
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
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
                text: "会员中心"
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
