// =============================================================================
// SmartSched-HIS 通用按钮组件
// =============================================================================

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12

Button {
    id: control
    
    property color buttonColor: "#1976D2"
    property color textColor: "#FFFFFF"
    property int borderRadius: 8
    property int fontSize: 16
    property bool isOutlined: false
    
    contentItem: Text {
        text: control.text
        font.pixelSize: control.fontSize
        font.bold: true
        font.family: "Microsoft YaHei, SimHei, sans-serif"
        color: control.isOutlined ? control.buttonColor : control.textColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    
    background: Rectangle {
        implicitWidth: control.width || 120
        implicitHeight: control.height || 48
        radius: control.borderRadius
        
        color: control.isOutlined ? "transparent" : control.buttonColor
        border.width: control.isOutlined ? 2 : 0
        border.color: control.buttonColor
    }
    
    states: State {
        name: "pressed"
        when: control.pressed
        PropertyChanges {
            target: control.background
            color: control.isOutlined ? control.buttonColor : control.buttonColor + "CC"
        }
        PropertyChanges {
            target: control.contentItem
            color: control.isOutlined ? "#FFFFFF" : control.textColor
        }
    }
    
    transitions: Transition {
        NumberAnimation { properties: "color"; duration: 150 }
    }
}

// =============================================================================
// 大号主按钮
// =============================================================================
Button {
    id: primaryButton
    
    implicitWidth: 200
    implicitHeight: 56
    fontSize: 18
    buttonColor: "#1976D2"
    
    states: State {
        name: "disabled"; when: !enabled
        PropertyChanges {
            target: primaryButton.background
            color: "#BDBDBD"
        }
    }
}

// =============================================================================
// 次要按钮
// =============================================================================
Button {
    id: secondaryButton
    
    implicitWidth: 150
    implicitHeight: 48
    fontSize: 14
    buttonColor: "#757575"
    
    isOutlined: true
}

// =============================================================================
// 图标按钮
// =============================================================================
ToolButton {
    id: iconButton
    
    property url iconSource
    property int iconSize: 24
    
    width: 48
    height: 48
    
    contentItem: Image {
        source: iconSource
        width: iconSize
        height: iconSize
        anchors.centerIn: parent
        fillMode: Image.PreserveAspectFit
    }
}
