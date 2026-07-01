import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

Rectangle {
    property string name: ""
    property string status: ""
    property int power: 0
    property string distance: ""
    width: parent.width
    height: 80
    radius: 8
    color: "#F5F5F5"
    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        Column {
            Layout.fillWidth: true
            Text { text: name; font.bold: true; font.pixelSize: 16 }
            Text { text: "功率: " + power + "kW  |  距离: " + distance; color: "#757575"; font.pixelSize: 12 }
        }
        Rectangle {
            width: 60
            height: 30
            radius: 15
            color: status === "空闲" ? "#4CAF50" : (status === "充电中" ? "#FFA726" : "#EF5350")
            Text {
                anchors.centerIn: parent
                text: status
                color: "white"
                font.pixelSize: 12
            }
        }
    }
}
