// =============================================================================
// SmartSched-HIS 通用主题和组件
// =============================================================================

pragma Singleton
import QtQuick 2.15

// =============================================================================
// 颜色主题
// =============================================================================
QtObject {
    property color primaryColor: "#1976D2"        // 主色-蓝
    property color primaryDark: "#1565C0"        // 深蓝
    property color primaryLight: "#42A5F5"      // 浅蓝
    property color accentColor: "#FF5722"       // 强调色-橙
    
    property color backgroundColor: "#FAFAFA"    // 背景色
    property color surfaceColor: "#FFFFFF"      // 卡片背景
    property color dividerColor: "#E0E0E0"      // 分割线
    
    property color textPrimary: "#212121"        // 主文字
    property color textSecondary: "#757575"      // 次要文字
    property color textHint: "#9E9E9E"          // 提示文字
    
    property color successColor: "#4CAF50"      // 成功-绿
    property color warningColor: "#FFC107"       // 警告-黄
    property color errorColor: "#F44336"         // 错误-红
    property color infoColor: "#2196F3"          // 信息-蓝
    
    // 状态颜色
    property color waitingColor: "#FFC107"      // 等待中
    property color consultingColor: "#2196F3"   // 诊治中
    property color completedColor: "#4CAF50"    // 已完成
}

// =============================================================================
// 字体定义
// =============================================================================
QtObject {
    property font headLarge: FontLoader {
        name: "Microsoft YaHei"
    }
}
