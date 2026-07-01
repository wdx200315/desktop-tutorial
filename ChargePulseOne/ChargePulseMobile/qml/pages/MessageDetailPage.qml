import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import "../components"

Page {
    title: "消息详情"
    property int messageId: 0
    property var messageData: ({
        title: "",
        content: "",
        type: "",
        create_time: "",
        is_read: false
    })
    property bool isLoading: false
    
    Component.onCompleted: {
        loadData()
    }
    
    function loadData() {
        // 消息详情通常通过列表传递，这里显示占位信息
        if (messageId > 0) {
            toast.showInfo("消息详情加载中...")
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
                    text: "消息详情"
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
                
                // ========== 消息头部 ==========
                Rectangle {
                    width: parent.width
                    color: "white"
                    Column {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12
                        
                        // 标题
                        Text {
                            text: messageData.title || "系统通知"
                            font.bold: true
                            font.size: 18
                            color: "#333"
                            width: parent.width
                            wrapMode: Text.WordWrap
                        }
                        
                        // 时间和类型
                        Row {
                            spacing: 12
                            
                            Rectangle {
                                width: 50
                                height: 22
                                radius: 11
                                color: messageData.type === "system" ? "#E3F2FD" :
                                       messageData.type === "order" ? "#E8F5E9" :
                                       messageData.type === "promotion" ? "#FFF3E0" : "#F3E5F5"
                                
                                Text {
                                    text: messageData.type === "system" ? "系统" :
                                           messageData.type === "order" ? "订单" :
                                           messageData.type === "promotion" ? "活动" : "其他"
                                    font.pixelSize: 11
                                    color: messageData.type === "system" ? "#1976D2" :
                                           messageData.type === "order" ? "#388E3C" :
                                           messageData.type === "promotion" ? "#F57C00" : "#7B1FA2"
                                    anchors.centerIn: parent
                                }
                            }
                            
                            Text {
                                text: messageData.create_time || ""
                                font.size: 12
                                color: "#999"
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                    }
                }
                
                Rectangle {
                    width: parent.width
                    height: 1
                    color: "#F0F0F0"
                }
                
                // ========== 消息内容 ==========
                Rectangle {
                    width: parent.width
                    color: "white"
                    Column {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12
                        
                        Text {
                            text: messageData.content || "暂无消息内容"
                            font.size: 15
                            color: "#333"
                            width: parent.width
                            wrapMode: Text.WordWrap
                            lineHeight: 1.6
                        }
                    }
                }
                
                // ========== 附件区域（预留） ==========
                Rectangle {
                    width: parent.width
                    color: "#F5F7FA"
                    visible: false
                    Column {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12
                        
                        Text {
                            text: "📎 附件"
                            font.bold: true
                            font.size: 14
                            color: "#333"
                        }
                        
                        // TODO: 添加附件列表
                    }
                }
                
                // 底部间距
                Item { height: 100 }
            }
        }
    }
    
    header: null
}
