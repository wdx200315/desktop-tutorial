import QtQuick 6.0
import QtQuick.Controls 6.0

Rectangle {
    id: networkIndicator
    width: 60
    height: 28
    radius: 14
    color: connected ? "#4CAF50" : "#F44336"

    property bool connected: false

    Text {
        anchors.centerIn: parent
        text: connected ? "在线" : "离线"
        color: "white"
        font.pixelSize: 12
        font.bold: true
    }
}
