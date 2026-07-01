// =============================================================================
// SmartSched-HIS 挂号页面
// 功能：选择医生，确认挂号信息，完成挂号
// =============================================================================

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12

Page {
    id: registrationPage
    title: "选择医生"
    
    property int deptId: 0
    property string deptName: ""
    
    // 背景
    Rectangle {
        anchors.fill: parent
        color: "#FAFAFA"
    }
    
    // 顶部信息栏
    Rectangle {
        id: headerBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 80
        color: "#1976D2"
        
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            
            // 返回按钮
            ToolButton {
                Layout.preferredWidth: 50
                text: "←"
                font.pixelSize: 24
                onClicked: pageStack.pop()
            }
            
            ColumnLayout {
                spacing: 4
                
                Text {
                    text: deptName
                    font.pixelSize: 24
                    font.bold: true
                    color: "#FFFFFF"
                    font.family: "Microsoft YaHei, SimHei, sans-serif"
                }
                
                Text {
                    text: "请选择接诊医生"
                    font.pixelSize: 14
                    color: "#FFFFFF"
                    opacity: 0.9
                }
            }
            
            Item { Layout.fillWidth: true }
        }
    }
    
    // 医生列表
    ListView {
        id: doctorList
        anchors.top: headerBar.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 20
        spacing: 16
        
        model: ListModel {
            id: doctorModel
        }
        
        delegate: doctorCard
        
        Component.onCompleted: loadDoctors()
        
        ScrollBar.vertical: ScrollBar { width: 8 }
    }
    
    // =====================================================================
    // 医生卡片
    // =====================================================================
    component doctorCard: Rectangle {
        id: card
        width: doctorList.width
        height: 140
        radius: 12
        color: "#FFFFFF"
        border.width: 1
        border.color: "#E0E0E0"
        
        // 可用状态边框
        Rectangle {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 6
            radius: 3
            color: available ? "#4CAF50" : "#9E9E9E"
        }
        
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 25
            anchors.rightMargin: 20
            
            // 医生头像
            Rectangle {
                width: 80
                height: 80
                radius: 40
                color: available ? "#E3F2FD" : "#EEEEEE"
                
                Text {
                    text: "👨‍⚕️"
                    font.pixelSize: 40
                    anchors.centerIn: parent
                }
            }
            
            // 医生信息
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6
                
                Text {
                    text: doctorName
                    font.pixelSize: 20
                    font.bold: true
                    color: "#212121"
                    font.family: "Microsoft YaHei, SimHei, sans-serif"
                }
                
                Text {
                    text: doctorTitle
                    font.pixelSize: 14
                    color: "#757575"
                    font.family: "Microsoft YaHei, sans-serif"
                }
                
                Text {
                    text: doctorSpecialty
                    font.pixelSize: 13
                    color: "#9E9E9E"
                    font.family: "Microsoft YaHei, sans-serif"
                }
                
                // 状态标签
                Rectangle {
                    width: statusText === "可接诊" ? 80 : 60
                    height: 26
                    radius: 13
                    color: available ? "#E8F5E9" : "#EEEEEE"
                    
                    Text {
                        text: statusText
                        font.pixelSize: 12
                        font.bold: true
                        color: available ? "#4CAF50" : "#9E9E9E"
                        anchors.centerIn: parent
                    }
                }
            }
            
            // 挂号按钮
            Button {
                Layout.preferredWidth: 120
                Layout.preferredHeight: 48
                text: "选择"
                font.pixelSize: 16
                font.bold: true
                buttonColor: available ? "#1976D2" : "#BDBDBD"
                
                onClicked: {
                    if (available) {
                        confirmRegistration(doctorId, doctorName)
                    }
                }
            }
        }
        
        // 点击效果
        MouseArea {
            anchors.fill: parent
            onClicked: {
                if (available) {
                    confirmRegistration(doctorId, doctorName)
                }
            }
        }
    }
    
    // =====================================================================
    // 挂号确认对话框
    // =====================================================================
    Dialog {
        id: confirmDialog
        title: "确认挂号信息"
        standardButtons: Dialog.Yes | Dialog.No
        modal: true
        anchors.centerIn: parent
        
        ColumnLayout {
            spacing: 16
            
            // 提示信息
            Rectangle {
                width: 400
                height: 60
                radius: 8
                color: "#E3F2FD"
                
                Text {
                    text: "📋 请确认以下挂号信息"
                    font.pixelSize: 16
                    font.bold: true
                    color: "#1976D2"
                    anchors.centerIn: parent
                }
            }
            
            // 挂号信息
            GridLayout {
                columns: 2
                rowSpacing: 12
                columnSpacing: 20
                
                Text {
                    text: "科室："
                    font.pixelSize: 14
                    color: "#757575"
                }
                Text {
                    text: deptName
                    font.pixelSize: 14
                    font.bold: true
                    color: "#212121"
                }
                
                Text {
                    text: "医生："
                    font.pixelSize: 14
                    color: "#757575"
                }
                Text {
                    text: confirmDoctorName
                    font.pixelSize: 14
                    font.bold: true
                    color: "#212121"
                }
                
                Text {
                    text: "患者姓名："
                    font.pixelSize: 14
                    color: "#757575"
                }
                TextField {
                    id: patientNameInput
                    placeholderText: "请输入患者姓名"
                    font.pixelSize: 14
                    Layout.preferredWidth: 200
                }
                
                Text {
                    text: "联系电话："
                    font.pixelSize: 14
                    color: "#757575"
                }
                TextField {
                    id: patientPhoneInput
                    placeholderText: "请输入联系电话"
                    font.pixelSize: 14
                    Layout.preferredWidth: 200
                    validator: RegExpValidator { regExp: /^[0-9]{11}$/ }
                }
            }
        }
        
        onAccepted: {
            // 执行挂号
            doRegister()
        }
    }
    
    // =====================================================================
    // 挂号成功对话框
    // =====================================================================
    Popup {
        id: successPopup
        anchors.centerIn: parent
        modal: true
        closePolicy: Popup.NoAutoClose
        padding: 30
        
        Rectangle {
            width: 450
            height: 380
            radius: 20
            color: "#FFFFFF"
            
            ColumnLayout {
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
                    height: 140
                    radius: 10
                    color: "#FAFAFA"
                    border.width: 2
                    border.color: "#E0E0E0"
                    
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 15
                        spacing: 10
                        
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
                            rowSpacing: 8
                            columnSpacing: 30
                            
                            Text { text: "排队号："; font.pixelSize: 16; color: "#757575" }
                            Text { 
                                text: resultQueueNumber
                                font.pixelSize: 24
                                font.bold: true
                                color: "#1976D2"
                            }
                            
                            Text { text: "科室："; font.pixelSize: 14; color: "#757575" }
                            Text { text: deptName; font.pixelSize: 14; color: "#212121" }
                            
                            Text { text: "医生："; font.pixelSize: 14; color: "#757575" }
                            Text { text: resultDoctorName; font.pixelSize: 14; color: "#212121" }
                            
                            Text { text: "排队位置："; font.pixelSize: 14; color: "#757575" }
                            Text { 
                                text: "第 " + resultPosition + " 位"
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
                        successPopup.close()
                        pageStack.pop() // 返回首页
                    }
                }
            }
        }
    }
    
    // 加载指示器
    Popup {
        id: loadingPopup
        anchors.centerIn: parent
        modal: true
        closePolicy: Popup.NoAutoClose
        
        ColumnLayout {
            spacing: 15
            
            ProgressBar {
                indeterminate: true
                Layout.preferredWidth: 200
            }
            
            Text {
                text: "正在挂号..."
                font.pixelSize: 14
                color: "#757575"
            }
        }
    }
    
    // =====================================================================
    // 辅助变量
    // =====================================================================
    property string confirmDoctorName: ""
    property int confirmDoctorId: 0
    property string resultQueueNumber: ""
    property string resultDoctorName: ""
    property int resultPosition: 0
    
    // =====================================================================
    // 辅助函数
    // =====================================================================
    function loadDoctors() {
        // 模拟医生数据
        var doctors = [
            {"doctorId": 1, "doctorName": "张明华", "doctorTitle": "主任医师", "doctorSpecialty": "心血管疾病、老年病", "available": true},
            {"doctorId": 2, "doctorName": "李秀英", "doctorTitle": "副主任医师", "doctorSpecialty": "消化系统疾病", "available": true},
            {"doctorId": 3, "doctorName": "王建国", "doctorTitle": "主治医师", "doctorSpecialty": "呼吸系统疾病", "available": false}
        ]
        
        // 根据科室筛选
        for (var i = 0; i < doctors.length; i++) {
            // 随机设置可用状态
            doctors[i].available = Math.random() > 0.3
            doctors[i].statusText = doctors[i].available ? "可接诊" : "已满"
            doctorModel.append(doctors[i])
        }
    }
    
    function confirmRegistration(doctorId, doctorName) {
        confirmDoctorId = doctorId
        confirmDoctorName = doctorName
        confirmDialog.text = "您选择的医生是：" + doctorName + "\n是否确认挂号？"
        confirmDialog.open()
    }
    
    function doRegister() {
        loadingPopup.open()
        
        // 模拟挂号成功
        setTimeout(function() {
            loadingPopup.close()
            
            // 生成排队号
            resultQueueNumber = "D" + String(deptId).padStart(2, '0') + "-" + 
                              new Date().toLocaleDateString('zh-CN').replace(/\//g, '') + 
                              String(Math.floor(Math.random() * 9999) + 1).padStart(4, '0')
            resultDoctorName = confirmDoctorName
            resultPosition = Math.floor(Math.random() * 20) + 1
            
            successPopup.open()
        }, 1000)
    }
}
