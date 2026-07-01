import QtQuick 6.0
import QtQuick.Controls 6.0
Rectangle {
    property string date: ""
    property string amount: ""
    property string status: ""
    height: 60; color: "#FAFAFA"
    Row { spacing: 20; padding: 10
        Text { text: date; font.pixelSize: 14 }
        Text { text: amount; font.pixelSize: 14; color: "#E53935" }
        Text { text: status; font.pixelSize: 14; color: "#43A047" }
    }
}
