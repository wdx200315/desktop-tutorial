import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import "../components"

Page {
    title: "充值"
    property double currentBalance: 0.00
    property string selectedAmount: "50"

    Component.onCompleted: {
        loadData()
    }

    function loadData() {
        if (net.connected) {
            net.getUserInfo()
        }
    }

    Connections {
        target: net
        function onResponseReceived(resp) {
            if (resp.cmd === "1005" && resp.status === "ok") {
                currentBalance = parseFloat(resp.data.balance) || 0
            }
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
                    text: "账户充值"
                    color: "white"
                    font.bold: true
                    font.pixelSize: 18
                    anchors.verticalCenter: parent.verticalCenter
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

                // ========== 当前余额 ==========
                Rectangle {
                    width: parent.width
                    color: "#F5F7FA"
                    Column {
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 8

                        Text {
                            text: "当前余额"
                            font.pixelSize: 14
                            color: "#666666"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }

                        Text {
                            text: "¥" + currentBalance.toFixed(2)
                            font.pixelSize: 36
                            font.bold: true
                            color: "#333333"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                }

                // ========== 选择充值金额 ==========
                Rectangle {
                    width: parent.width
                    color: "white"
                    Column {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 16

                        Text {
                            text: "选择充值金额"
                            font.bold: true
                            font.pixelSize: 16
                            color: "#333333"
                        }

                        Grid {
                            columns: 3
                            spacing: 12
                            width: parent.width

                            property var amounts: ["20", "50", "100", "200", "500", "1000"]

                            Repeater {
                                model: parent.amounts
                                Rectangle {
                                    width: (parent.width - 24) / 3
                                    height: 50
                                    radius: 8
                                    color: selectedAmount === modelData ? "#4A90D9" : "#F5F7FA"
                                    border.width: selectedAmount === modelData ? 0 : 1
                                    border.color: "#E0E0E0"

                                    Text {
                                        text: "¥" + modelData
                                        font.pixelSize: 16
                                        font.bold: true
                                        color: selectedAmount === modelData ? "white" : "#333333"
                                        anchors.centerIn: parent
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: selectedAmount = modelData
                                    }
                                }
                            }
                        }

                        // 自定义金额
                        Row {
                            spacing: 12
                            Text {
                                text: "自定义金额"
                                font.pixelSize: 14
                                color: "#666666"
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            TextField {
                                id: txtCustomAmount
                                width: 120
                                height: 40
                                placeholderText: "输入金额"
                                font.pixelSize: 14
                                inputMethodHints: Qt.ImhDigitsOnly
                                background: Rectangle {
                                    color: "#F5F7FA"
                                    border.width: 1
                                    border.color: "#E0E0E0"
                                    radius: 8
                                }
                                onTextChanged: {
                                    if (text.length > 0) {
                                        selectedAmount = text
                                    }
                                }
                            }
                        }
                    }
                }

                // ========== 支付方式 ==========
                Rectangle {
                    width: parent.width
                    color: "white"
                    Column {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12

                        Text {
                            text: "支付方式"
                            font.bold: true
                            font.pixelSize: 16
                            color: "#333333"
                        }

                        // 微信支付
                        Rectangle {
                            width: parent.width
                            height: 56
                            radius: 8
                            color: "#F5F7FA"

                            Row {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 12

                                Text {
                                    text: "💳"
                                    font.pixelSize: 24
                                    anchors.verticalCenter: parent.verticalCenter
                                }

                                Column {
                                    anchors.verticalCenter: parent.verticalCenter
                                    Text {
                                        text: "微信支付"
                                        font.pixelSize: 14
                                        color: "#333333"
                                    }
                                    Text {
                                        text: "推荐"
                                        font.pixelSize: 11
                                        color: "#4CAF50"
                                    }
                                }

                                Item { Layout.fillWidth: true }

                                Rectangle {
                                    width: 20
                                    height: 20
                                    radius: 10
                                    color: "#4A90D9"
                                    anchors.verticalCenter: parent.verticalCenter

                                    Text {
                                        text: "✓"
                                        color: "white"
                                        font.pixelSize: 12
                                        anchors.centerIn: parent
                                    }
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: toast.show("微信支付")
                            }
                        }

                        // 支付宝
                        Rectangle {
                            width: parent.width
                            height: 56
                            radius: 8
                            color: "#F5F7FA"

                            Row {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 12

                                Text {
                                    text: "💰"
                                    font.pixelSize: 24
                                    anchors.verticalCenter: parent.verticalCenter
                                }

                                Column {
                                    anchors.verticalCenter: parent.verticalCenter
                                    Text {
                                        text: "支付宝"
                                        font.pixelSize: 14
                                        color: "#333333"
                                    }
                                }

                                Item { Layout.fillWidth: true }

                                Rectangle {
                                    width: 20
                                    height: 20
                                    radius: 10
                                    border.width: 1
                                    border.color: "#CCCCCC"
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: toast.show("支付宝支付")
                            }
                        }
                    }
                }

                // ========== 充值优惠 ==========
                Rectangle {
                    width: parent.width
                    color: "#FFF3E0"
                    Column {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 8

                        Row {
                            spacing: 8
                            Text {
                                text: "🎁"
                                font.pixelSize: 16
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Text {
                                text: "充值优惠"
                                font.bold: true
                                font.pixelSize: 14
                                color: "#FF9800"
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }

                        Text {
                            text: "• 充值满100元送5元"
                            font.pixelSize: 12
                            color: "#FF9800"
                        }
                        Text {
                            text: "• 充值满500元送30元"
                            font.pixelSize: 12
                            color: "#FF9800"
                        }
                        Text {
                            text: "• 充值满1000元送80元"
                            font.pixelSize: 12
                            color: "#FF9800"
                        }
                    }
                }

                // ========== 确认充值按钮 ==========
                Rectangle {
                    width: parent.width
                    color: "#F5F7FA"
                    Column {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12

                        Rectangle {
                            width: parent.width
                            height: 50
                            radius: 25
                            color: "#4A90D9"

                            Text {
                                text: "立即充值 ¥" + selectedAmount
                                color: "white"
                                font.bold: true
                                font.pixelSize: 16
                                anchors.centerIn: parent
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    toast.show("充值功能开发中，请稍后...")
                                }
                            }
                        }

                        Text {
                            text: "充值金额将直接到账，请放心支付"
                            font.pixelSize: 11
                            color: "#999999"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                }

                Item { height: 100 }
            }
        }
    }

    header: null
}
