// =============================================================================
// SmartSched-HIS 科室列表页面
// 功能：显示所有科室，选择后进入挂号页面
// =============================================================================

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12
import SmartSched.Models 1.0

Page {
    id: departmentPage
    title: "选择科室"
    
    // 返回按钮处理
    onVisibleChanged: {
        if (visible) {
            // 加载科室列表
            loadDepartments()
        }
    }
    
    // 背景
    Rectangle {
        anchors.fill: parent
        color: "#FAFAFA"
    }
    
    // 顶部搜索栏
    Rectangle {
        id: searchBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 60
        color: "#FFFFFF"
        border.width: 1
        border.color: "#E0E0E0"
        
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            
            // 返回按钮
            ToolButton {
                Layout.preferredWidth: 50
                Layout.preferredHeight: 50
                text: "←"
                font.pixelSize: 24
                onClicked: pageStack.pop()
            }
            
            // 搜索框
            TextField {
                id: searchField
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                placeholderText: "搜索科室名称..."
                font.pixelSize: 16
                
                // 搜索图标
                leftPadding: 40
                
                background: Rectangle {
                    radius: 8
                    color: "#F5F5F5"
                }
                
                onTextChanged: filterDepartments()
            }
            
            // 搜索图标
            Text {
                text: "🔍"
                font.pixelSize: 20
                anchors.left: searchField.left
                anchors.verticalCenter: searchField.verticalCenter
                anchors.leftMargin: 10
            }
        }
    }
    
    // 科室列表
    ListView {
        id: departmentList
        anchors.top: searchBar.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 20
        spacing: 12
        
        model: ListModel {
            id: filteredModel
        }
        
        // 加载数据
        Component.onCompleted: loadDepartments()
        
        delegate: departmentDelegate
        
        // 滚动条
        ScrollBar.vertical: ScrollBar {
            width: 8
            anchors.right: parent.right
            policy: ScrollBar.AsNeeded
        }
    }
    
    // 加载中指示器
    LoadingIndicator {
        id: loadingIndicator
        anchors.centerIn: parent
        visible: false
    }
    
    // =====================================================================
    // 科室卡片委托
    // =====================================================================
    component departmentDelegate: Rectangle {
        id: delegateRoot
        width: departmentList.width
        height: 100
        radius: 12
        color: "#FFFFFF"
        border.width: 1
        border.color: "#E0E0E0"
        
        property bool isHovered: false
        
        // 点击效果
        states: State {
            name: "pressed"
            when: mouseArea.pressed
            PropertyChanges {
                target: delegateRoot
                color: "#F5F5F5"
            }
        }
        
        RowLayout {
            anchors.fill: parent
            anchors.margins: 20
            
            // 科室图标
            Rectangle {
                width: 60
                height: 60
                radius: 30
                color: getDeptColor(deptId)
                
                Text {
                    text: getDeptIcon(deptId)
                    font.pixelSize: 28
                    anchors.centerIn: parent
                }
            }
            
            // 科室信息
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4
                
                Text {
                    text: deptName
                    font.pixelSize: 20
                    font.bold: true
                    color: "#212121"
                    font.family: "Microsoft YaHei, SimHei, sans-serif"
                }
                
                Text {
                    text: description
                    font.pixelSize: 14
                    color: "#757575"
                    font.family: "Microsoft YaHei, sans-serif"
                    elide: Text.ElideRight
                }
                
                RowLayout {
                    spacing: 20
                    
                    Text {
                        text: "当前排队: " + queueCount + " 人"
                        font.pixelSize: 13
                        color: queueCount > 30 ? "#F44336" : "#4CAF50"
                        font.bold: true
                    }
                    
                    Text {
                        text: "容量: " + queueCapacity + " 人"
                        font.pixelSize: 13
                        color: "#9E9E9E"
                    }
                }
            }
            
            // 箭头
            Text {
                text: ">"
                font.pixelSize: 24
                color: "#BDBDBD"
            }
        }
        
        MouseArea {
            id: mouseArea
            anchors.fill: parent
            onClicked: {
                // 跳转到挂号页面
                pageStack.push(Qt.resolvedUrl("RegistrationPage.qml"), {
                    "deptId": deptId,
                    "deptName": deptName
                })
            }
        }
        
        // 阴影效果
        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: parent.color
            visible: false
        }
        
        // 颜色映射
        function getDeptColor(id) {
            var colors = ["#E3F2FD", "#E8F5E9", "#FFF3E0", "#FCE4EC", "#E0F7FA",
                         "#F3E5F5", "#E8EAF6", "#FBE9E7", "#EFEBE9", "#E8F5E9"]
            return colors[id % colors.length]
        }
        
        function getDeptIcon(id) {
            var icons = ["🫀", "🔪", "👶", "👩", "🦴", "🧠", "🧠", "❤️", "🫁", "🫃",
                       "👁️", "👂", "🧴", "🦷", "💆"]
            return icons[(id - 1) % icons.length]
        }
    }
    
    // =====================================================================
    // 辅助函数
    // =====================================================================
    function loadDepartments() {
        loadingIndicator.visible = true
        
        // 模拟加载数据（实际应从服务端获取）
        // 这里使用测试数据
        filteredModel.clear()
        
        var testData = [
            {"deptId": 1, "deptName": "内科", "description": "Internal Medicine - 诊治内科常见疾病", "queueCount": 8, "queueCapacity": 50},
            {"deptId": 2, "deptName": "外科", "description": "General Surgery - 普通外科疾病", "queueCount": 5, "queueCapacity": 40},
            {"deptId": 3, "deptName": "儿科", "description": "Pediatrics - 儿童疾病诊治", "queueCount": 15, "queueCapacity": 60},
            {"deptId": 4, "deptName": "妇科", "description": "Gynecology - 妇科疾病诊治", "queueCount": 12, "queueCapacity": 45},
            {"deptId": 5, "deptName": "骨科", "description": "Orthopedics - 骨科疾病诊治", "queueCount": 6, "queueCapacity": 35},
            {"deptId": 6, "deptName": "神经内科", "description": "Neurology - 神经系统疾病", "queueCount": 4, "queueCapacity": 30},
            {"deptId": 7, "deptName": "神经外科", "description": "Neurosurgery - 神经外科手术", "queueCount": 2, "queueCapacity": 20},
            {"deptId": 8, "deptName": "心血管内科", "description": "Cardiology - 心脏血管疾病", "queueCount": 9, "queueCapacity": 35},
            {"deptId": 9, "deptName": "呼吸内科", "description": "Pulmonology - 呼吸系统疾病", "queueCount": 7, "queueCapacity": 40},
            {"deptId": 10, "deptName": "消化内科", "description": "Gastroenterology - 消化系统疾病", "queueCount": 11, "queueCapacity": 40},
            {"deptId": 11, "deptName": "眼科", "description": "Ophthalmology - 眼科疾病诊治", "queueCount": 3, "queueCapacity": 50},
            {"deptId": 12, "deptName": "耳鼻喉科", "description": "ENT - 耳鼻喉疾病", "queueCount": 5, "queueCapacity": 45},
            {"deptId": 13, "deptName": "皮肤科", "description": "Dermatology - 皮肤疾病诊治", "queueCount": 8, "queueCapacity": 50},
            {"deptId": 14, "deptName": "口腔科", "description": "Stomatology - 口腔疾病诊治", "queueCount": 6, "queueCapacity": 40},
            {"deptId": 15, "deptName": "中医科", "description": "Traditional Chinese Medicine - 中医诊疗", "queueCount": 4, "queueCapacity": 35}
        ]
        
        for (var i = 0; i < testData.length; i++) {
            filteredModel.append(testData[i])
        }
        
        loadingIndicator.visible = false
    }
    
    function filterDepartments() {
        var searchText = searchField.text.toLowerCase()
        
        // 实际应该过滤 filteredModel
        // 这里简化处理
        if (searchText === "") {
            loadDepartments()
        }
    }
    
    // 加载指示器组件
    component LoadingIndicator: Item {
        visible: false
        
        ColumnLayout {
            spacing: 10
            anchors.centerIn: parent
            
            ProgressBar {
                indeterminate: true
                Layout.preferredWidth: 200
            }
            
            Text {
                text: "正在加载科室列表..."
                font.pixelSize: 14
                color: "#757575"
            }
        }
    }
}
