import QtQuick 6.0
import QtQuick.Controls 6.0

Rectangle {
    id: root
    visible: false
    opacity: 0
    color: "#323232"
    radius: 10
    width: Math.min(parent.width * 0.85, 320)
    height: 56
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.verticalCenter: parent.verticalCenter
    
    // 类型：success(绿色), error(红色), warning(橙色), info(蓝色), default(灰色)
    property string toastType: "default"
    property alias message: toastLabel.text

    // 左侧图标
    Rectangle {
        id: iconBg
        anchors.left: parent.left
        anchors.leftMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        width: 24
        height: 24
        radius: 12
        color: "transparent"
        
        Text {
            id: iconText
            anchors.centerIn: parent
            font.pixelSize: 18
            color: "white"
        }
    }

    Label {
        id: toastLabel
        anchors.left: iconBg.right
        anchors.leftMargin: 12
        anchors.right: parent.right
        anchors.rightMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        color: "white"
        font.pixelSize: 14
        font.weight: Font.Medium
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.WordWrap
        maximumLineCount: 2
        elide: Text.ElideRight
    }

    // 类型样式
    onToastTypeChanged: {
        switch(toastType) {
            case "success":
                color = "#4CAF50"
                iconText.text = "✓"
                break
            case "error":
                color = "#E53935"
                iconText.text = "✕"
                break
            case "warning":
                color = "#FF9800"
                iconText.text = "⚠"
                break
            case "info":
                color = "#2196F3"
                iconText.text = "ℹ"
                break
            default:
                color = "#323232"
                iconText.text = "•"
        }
    }

    // 显示方法重载
    function show(msg, type) {
        if (type !== undefined) toastType = type
        else toastType = "default"
        message = msg
        showAnimation.start()
        hideTimer.restart()
    }
    
    function showSuccess(msg) { show(msg, "success") }
    function showError(msg) { show(msg, "error") }
    function showWarning(msg) { show(msg, "warning") }
    function showInfo(msg) { show(msg, "info") }

    SequentialAnimation {
        id: showAnimation
        PropertyAnimation { target: root; property: "opacity"; from: 0; to: 1; duration: 200; easing.type: Easing.OutQuad }
        PropertyAnimation { target: root; property: "visible"; from: false; to: true; duration: 0 }
    }

    SequentialAnimation {
        id: hideAnimation
        PropertyAnimation { target: root; property: "opacity"; from: 1; to: 0; duration: 200; easing.type: Easing.InQuad }
        ScriptAction { script: root.visible = false }
    }

    Timer {
        id: hideTimer
        interval: 2500
        onTriggered: hideAnimation.start()
    }
}
