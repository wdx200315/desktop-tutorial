import QtQuick 6.0
import QtQuick.Controls 6.0

Rectangle {
    width: parent ? parent.width : 360
    height: 36
    color: "#F5F5F5"
    property alias text: label.text

    Text {
        id: label
        color: "#757575"
        font.pixelSize: 12
        anchors.left: parent.left
        anchors.leftMargin: 16
        anchors.verticalCenter: parent.verticalCenter
    }
}
