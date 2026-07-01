import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

Rectangle {
    id: button
    
    // 按钮属性
    property string text: ""
    property bool enabled: true
    property string type: "primary"  // primary, secondary, danger, success, warning, outline, text
    property string size: "medium"  // small, medium, large
    property bool loading: false
    property var onClicked: function() {}
    
    // 图标属性
    property string icon: ""  // 例如: "⚡", "✓", "✕"
    property string iconPosition: "left"  // left, right
    
    // 颜色配置
    property color primaryColor: "#4A90D9"
    property color secondaryColor: "#6C757D"
    property color dangerColor: "#E53935"
    property color successColor: "#4CAF50"
    property color warningColor: "#FF9800"
    property color disabledBgColor: "#E0E0E0"
    property color disabledTextColor: "#9E9E9E"
    
    // 根据类型和状态设置颜色
    property color bgColor: {
        if (!enabled) return disabledBgColor
        switch(type) {
            case "primary": return primaryColor
            case "secondary": return secondaryColor
            case "danger": return dangerColor
            case "success": return successColor
            case "warning": return warningColor
            case "outline": return "transparent"
            case "text": return "transparent"
            default: return primaryColor
        }
    }
    
    property color txtColor: {
        if (!enabled) return disabledTextColor
        switch(type) {
            case "outline": return primaryColor
            case "text": return primaryColor
            default: return "white"
        }
    }
    
    // 圆角
    property int cornerRadius: {
        switch(size) {
            case "small": return 4
            case "large": return 25
            default: return 8
        }
    }
    
    // 高度
    property int btnHeight: {
        switch(size) {
            case "small": return 32
            case "large": return 50
            default: return 42
        }
    }
    
    // 字体大小
    property int fontSize: {
        switch(size) {
            case "small": return 12
            case "large": return 16
            default: return 14
        }
    }
    
    color: bgColor
    radius: cornerRadius
    height: btnHeight
    width: parent ? parent.width : 120
    
    // 边框（outline类型）
    Rectangle {
        visible: type === "outline"
        anchors.fill: parent
        radius: parent.radius
        color: "transparent"
        border.width: 1.5
        border.color: enabled ? parent.primaryColor : disabledTextColor
    }
    
    // 内容布局
    Row {
        anchors.centerIn: parent
        spacing: icon && text ? 8 : 0
        layoutDirection: iconPosition === "right" ? Qt.RightToLeft : Qt.LeftToRight
        
        // 图标
        Text {
            text: icon
            font.pixelSize: fontSize + 2
            color: txtColor
            anchors.verticalCenter: parent.verticalCenter
            visible: icon !== ""
        }
        
        // 文字
        Text {
            text: loading ? "加载中..." : button.text
            color: txtColor
            font.pixelSize: fontSize
            font.bold: type !== "text" && type !== "outline"
            anchors.verticalCenter: parent.verticalCenter
            visible: button.text !== ""
        }
    }
    
    // 点击波纹效果
    Rectangle {
        visible: enabled && !loading
        anchors.fill: parent
        radius: parent.radius
        color: "white"
        opacity: mouseArea.pressed ? 0.15 : 0
    }
    
    // 加载动画
    Rectangle {
        visible: loading
        anchors.centerIn: parent
        width: 20
        height: 20
        radius: 10
        color: "transparent"
        border.width: 2
        border.color: txtColor
        
        RotationAnimation on rotation {
            loops: Animation.Infinite
            from: 0
            to: 360
            duration: 800
            running: loading
        }
    }
    
    MouseArea {
        id: mouseArea
        anchors.fill: parent
        enabled: enabled && !loading
        onClicked: {
            if (typeof onClicked === 'function' && enabled && !loading) {
                onClicked()
            }
        }
    }
    
    // 禁用遮罩
    Rectangle {
        visible: !enabled
        anchors.fill: parent
        radius: parent.radius
        color: "#F5F5F5"
        opacity: 0.3
    }
    
    // 状态变化动画
    Behavior on color {
        ColorAnimation { duration: 150 }
    }
    Behavior on opacity {
        NumberAnimation { duration: 150 }
    }
}
