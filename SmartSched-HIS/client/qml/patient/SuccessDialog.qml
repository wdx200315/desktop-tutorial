// =============================================================================
// SmartSched-HIS 挂号成功对话框
// =============================================================================

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12

Popup {
    id: successDialog
    modal: true
    closePolicy: Popup.NoAutoClose
    anchors.centerIn: parent
    
    background: Rectangle {
        width: 450
        height: 400
        radius: 20
        color: "#FFFFFF"
    }
    
    // 内容由外部设置
    contentItem: ColumnLayout {
        anchors.fill: parent
        anchors.margins: 30
        spacing: 20
        
        // 成功图标
        Rectangle {
            width: 100
            height: 100
            radius: 50
            color: "#E8F5E9"
            Layout.alignment: Qt.AlignHCenter
            
            Text {
                text: "✓"
                font.pixelSize: 60
                font.bold: true
                color: "#4CAF50"
                anchors.centerIn: parent
            }
        }
        
        Text {
            text: "挂号成功！"
            font.pixelSize: 28
            font.bold: true
            color: "#4CAF50"
            font.family: "Microsoft YaHei, SimHei, sans-serif"
            Layout.alignment: Qt.AlignHCenter
        }
        
        // 凭条信息
        Rectangle {
            Layout.fillWidth: true
            height: 160
            radius: 10
            color: "#FAFAFA"
            border.width: 2
            border.color: "#E0E0E0"
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 15
                spacing: 12
                
                Text {
                    text: HOSPITAL_NAME + " 挂号凭条"
                    font.pixelSize: 14
                    font.bold: true
                    color: "#212121"
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                
                Rectangle {
                    height: 1
                    color: "#E0E0E0"
                }
                
                GridLayout {
                    columns: 2
                    rowSpacing: 10
                    columnSpacing: 30
                    
                    Text { text: "排队号："; font.pixelSize: 14; color: "#757575" }
                    Text { 
                        id: queueNumberText
                        text: ""
                        font.pixelSize: 22
                        font.bold: true
                        color: "#1976D2"
                    }
                    
                    Text { text: "科室："; font.pixelSize: 14; color: "#757575" }
                    Text { 
                        id: deptNameText
                        text: ""
                        font.pixelSize: 14
                        color: "#212121"
                    }
                    
                    Text { text: "医生："; font.pixelSize: 14; color: "#757575" }
                    Text { 
                        id: doctorNameText
                        text: ""
                        font.pixelSize: 14
                        color: "#212121"
                    }
                    
                    Text { text: "排队位置："; font.pixelSize: 14; color: "#757575" }
                    Text { 
                        id: positionText
                        text: ""
                        font.pixelSize: 14
                        font.bold: true
                        color: "#FF5722"
                    }
                }
            }
        }
        
        Text {
            text: "请妥善保管此凭条，听到呼叫后凭条入室就诊"
            font.pixelSize: 12
            color: "#9E9E9E"
            font.family: "Microsoft YaHei, sans-serif"
            Layout.alignment: Qt.AlignHCenter
        }
        
        // 确定按钮
        Button {
            text: "确定"
            font.pixelSize: 18
            buttonColor: "#1976D2"
            Layout.preferredWidth: 200
            Layout.preferredHeight: 50
            Layout.alignment: Qt.AlignHCenter
            
            onClicked: {
                successDialog.close()
            }
        }
    }
    
    // 便捷方法
    function show(queueNumber, dept, doctor, position) {
        queueNumberText.text = queueNumber
        deptNameText.text = dept
        doctorNameText.text = doctor
        positionText.text = "第 " + position + " 位"
        successDialog.open()
    }
}
