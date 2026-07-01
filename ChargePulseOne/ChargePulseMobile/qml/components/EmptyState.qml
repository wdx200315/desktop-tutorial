import QtQuick 6.0
import QtQuick.Controls 6.0

Rectangle {
    id: root
    visible: false
    color: "#F5F7FA"
    
    property string emptyIcon: "📭"
    property string emptyTitle: "暂无数据"
    property string emptyDescription: "这里什么都没有"
    property string emptyActionText: ""
    property var emptyAction: function() {}
    
    Column {
        anchors.centerIn: parent
        spacing: 12
        
        // 图标
        Text {
            text: emptyIcon
            font.pixelSize: 64
            anchors.horizontalCenter: parent.horizontalCenter
        }
        
        // 标题
        Text {
            text: emptyTitle
            font.pixelSize: 18
            font.bold: true
            color: "#333333"
            anchors.horizontalCenter: parent.horizontalCenter
        }
        
        // 描述
        Text {
            text: emptyDescription
            font.pixelSize: 14
            color: "#999999"
            anchors.horizontalCenter: parent.horizontalCenter
        }
        
        // 操作按钮
        Rectangle {
            visible: emptyActionText !== ""
            width: 120
            height: 36
            radius: 18
            color: "#4A90D9"
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.topMargin: 8
            
            Text {
                text: emptyActionText
                color: "white"
                font.pixelSize: 14
                font.bold: true
                anchors.centerIn: parent
            }
            
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (typeof emptyAction === 'function') {
                        emptyAction()
                    }
                }
            }
        }
    }
    
    // 显示动画
    PropertyAnimation {
        target: root
        property: "opacity"
        from: 0
        to: 1
        duration: 300
    }
}
