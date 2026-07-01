#pragma once
#include <QString>

namespace CMD {
    // 用户认证
    const QString LOGIN           = "1001";
    const QString REGISTER        = "1002";
    const QString CHANGE_PWD      = "1003";
    const QString RESET_PWD       = "1004";
    const QString USER_INFO       = "1005";
    const QString UPDATE_USER     = "1006";
    const QString USER_LIST       = "1007";
    const QString USER_EDIT       = "1008";
    const QString VEHICLE_ADD     = "1009";
    const QString VEHICLE_LIST    = "1010";
    const QString VEHICLE_DEL     = "1011";

    // 充电桩
    const QString CHARGER_LIST    = "2001";
    const QString CHARGER_DETAIL  = "2002";
    const QString CHARGER_CTRL    = "2003";

    // 充电控制
    const QString START_CHARGE    = "3001";
    const QString STOP_CHARGE     = "3002";
    const QString CHARGE_MONITOR  = "3003";
    const QString RESERVE         = "3004";

    // 计费
    const QString RATE_LIST       = "4001";
    const QString FEE_ESTIMATE    = "4002";
    const QString REAL_BILLING    = "4003";
    const QString PAYMENT         = "4004";

    // 订单
    const QString ORDER_LIST      = "5001";
    const QString ORDER_DETAIL    = "5002";
    const QString ORDER_EXPORT    = "5003";

    // 告警
    const QString ALARM_LIST      = "6001";
    const QString ALARM_HANDLE    = "6002";

    // 报表
    const QString REPORT_DAILY    = "7001";
    const QString REPORT_MONTHLY  = "7002";

    // 系统
    const QString SYS_CONFIG_GET  = "8001";
    const QString SYS_CONFIG_SET  = "8002";
    const QString LOG_QUERY       = "8003";

    // 管理后台统计
    const QString ADMIN_GET_DASHBOARD_STATS = "8004";
    const QString ADMIN_GET_CHARGER_STATUS  = "8005";

    const QString HEARTBEAT       = "9999";

    // 消息中心
    const QString MESSAGE_LIST    = "9001";
    const QString MESSAGE_SEND    = "9002";
    const QString MESSAGE_READ    = "9003";
    const QString MESSAGE_DELETE  = "9004";

    // 优惠券
    const QString COUPON_LIST      = "9101";
    const QString COUPON_ADD       = "9102";
    const QString COUPON_EDIT      = "9103";
    const QString COUPON_DELETE    = "9104";
    const QString COUPON_CLAIM     = "9105";
    const QString COUPON_USER_LIST = "9106";
    const QString COUPON_USE       = "9107";

    // 会员
    const QString MEMBER_LEVELS   = "9201";
    const QString MEMBER_STATS    = "9202";
    const QString MEMBER_EDIT     = "9203";

    // 推送通知
    const QString PUSH_NOTIFICATION     = "9301";
    const QString DELETE_NOTIFICATION    = "9302";
    const QString NOTIFICATION_LIST      = "9303";

    // 车辆（已在上面定义）
}
