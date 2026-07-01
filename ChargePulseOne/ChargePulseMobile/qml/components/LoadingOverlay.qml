import QtQuick 6.0
import QtQuick.Controls 6.0

Rectangle {
    id: loadingOverlay
    anchors.fill: parent
    color: "#80000000"
    visible: false
    property string loadingText: "加载中..."
    property bool loading: false

    Rectangle {
        anchors.centerIn: parent
        width: 120
        height: 120
        radius: 12
        color: "white"

        Column {
            anchors.centerIn: parent
            spacing: 12

            BusyIndicator {
                anchors.horizontalCenter: parent.horizontalCenter
                running: true
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: loadingOverlay.loadingText
                font.pixelSize: 14
                color: "#333"
            }
        }
    }

    function show() {
        visible = true
        loading = true
    }

    function hide() {
        visible = false
        loading = false
    }
}
