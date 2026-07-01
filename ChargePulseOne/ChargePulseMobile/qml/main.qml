import QtQuick 6.0
import QtQuick.Controls 6.0
import ChargePulse 1.0

ApplicationWindow {
    id: appWindow
    width: 375
    height: 812
    visible: true
    title: "ChargePulse"

    // 网络管理器
    property NetworkManager net: NetworkManager {}

    // 主界面
    Rectangle {
        anchors.fill: parent
        color: "#4A90D9"

        Column {
            anchors.centerIn: parent
            spacing: 20

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "⚡ ChargePulse"
                color: "white"
                font.pixelSize: 36
                font.bold: true
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "电动车充电管理"
                color: "#BBDEFB"
                font.pixelSize: 18
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "连接状态: " + (net.connected ? "已连接" : "未连接")
                color: "white"
                font.pixelSize: 14
            }

            Rectangle {
                width: 200
                height: 1
                color: "#FFFFFF33"
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Text {
                id: statusText
                anchors.horizontalCenter: parent.horizontalCenter
                text: "系统就绪"
                color: "#BBDEFB"
                font.pixelSize: 14
            }
        }
    }

    Component.onCompleted: {
        console.log("ChargePulse Mobile Started")
    }
}
