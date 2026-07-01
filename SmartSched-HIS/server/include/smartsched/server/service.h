/**
 * @file service.h
 * @brief 业务服务层
 * 
 * 业务视角: 实现挂号、排队、就诊、B超调度
 * 多角色视角: 支持患者、医生、管理员操作
 */

#pragma once

#include "smartsched/server/database.h"
#include "smartsched/server/tcpserver.h"
#include "../../common/include/smartsched/protocol/commands.h"
#include "../../common/include/smartsched/utils/json_helper.h"
#include <QObject>
#include <QMap>
#include <QQueue>
#include <QMutex>
#include <QTimer>

namespace smartsched {
namespace server {

// =============================================================================
// 业务实体定义
// =============================================================================

// 科室信息
struct Department {
    int dept_id;
    QString dept_name;
    QString description;
    int queue_capacity;  // 最大排队人数
    bool is_active;
};

// 医生信息
struct Doctor {
    int doctor_id;
    int dept_id;
    QString name;
    QString title;  // 职称
    bool is_active;
    bool is_available;  // 是否空闲
    int current_patient_count;
};

// 患者信息
struct Patient {
    int patient_id;
    QString name;
    int age;
    QString gender;
    QString phone;
    QString id_card;
};

// 挂号记录
struct Registration {
    int reg_id;
    int patient_id;
    int dept_id;
    int doctor_id;
    QString queue_number;
    qint64 register_time;
    int status;  // 0=待诊, 1=就诊中, 2=已完成, 3=已取消
};

// 排队项
struct QueueItem {
    int queue_id;
    int patient_id;
    int dept_id;
    int doctor_id;
    int position;  // 当前位置
    QString queue_number;
    qint64 join_time;
    qint64 estimated_start_time;
    int status;  // 0=等待, 1=就诊中, 2=已完成
};

// B超预约
struct UltrasoundAppointment {
    int appt_id;
    int patient_id;
    int machine_id;  // 1, 2, 3
    qint64 appointment_time;
    qint64 start_time;
    qint64 end_time;
    int status;  // 0=预约, 1=进行中, 2=已完成
};

// 就诊记录
struct ConsultationRecord {
    int record_id;
    int patient_id;
    int doctor_id;
    qint64 start_time;
    qint64 end_time;
    int duration;  // 秒
    QString diagnosis;
    bool need_ultrasound;
};

// =============================================================================
// 业务服务类
// =============================================================================
class BusinessService : public QObject {
    Q_OBJECT
    
public:
    explicit BusinessService(std::shared_ptr<ConnectionPool> db_pool, QObject* parent = nullptr);
    ~BusinessService() override;
    
    // 初始化
    bool initialize();
    void shutdown();
    
    // 命令处理
    utils::JsonValue handleCommand(const QString& connection_id, 
                                    CommandID cmd, 
                                    const utils::JsonValue& params);
    
signals:
    void queueUpdated(int dept_id);
    void patientCalled(const QString& queue_number);
    void consultationStarted(int patient_id, int doctor_id);
    void consultationEnded(int patient_id, int doctor_id);

private:
    // 科室服务
    utils::JsonValue getDepartmentList();
    utils::JsonValue getDepartmentInfo(int dept_id);
    
    // 医生服务
    utils::JsonValue getDoctorList(int dept_id);
    utils::JsonValue getDoctorInfo(int doctor_id);
    
    // 挂号服务
    utils::JsonValue registerPatient(const utils::JsonValue& params);
    utils::JsonValue cancelRegistration(int reg_id);
    utils::JsonValue getQueueNumber(int patient_id, int dept_id);
    
    // 排队服务
    utils::JsonValue getQueueStatus(int patient_id);
    utils::JsonValue getEstimatedWaitTime(int patient_id);
    utils::JsonValue getAllQueueStatus(int dept_id);
    
    // 医生服务
    utils::JsonValue startConsultation(int doctor_id, int patient_id);
    utils::JsonValue endConsultation(int doctor_id, int patient_id);
    utils::JsonValue pauseConsultation(int doctor_id);
    utils::JsonValue getPatientInfo(int patient_id);
    utils::JsonValue getPatientHistory(int patient_id);
    
    // B超服务
    utils::JsonValue requestUltrasound(int patient_id, int doctor_id);
    utils::JsonValue getUltrasoundStatus();
    utils::JsonValue completeUltrasound(int appt_id);
    
    // 叫号服务
    utils::JsonValue callNextPatient(int doctor_id);
    utils::JsonValue callSpecificPatient(int doctor_id, const QString& queue_number);
    utils::JsonValue recallPatient(int doctor_id);
    utils::JsonValue skipPatient(int doctor_id);
    
    // 统计服务
    utils::JsonValue getStatistics();
    utils::JsonValue getDailyStatistics(const QString& date);
    utils::JsonValue getMonthlyStatistics(int year, int month);
    
    // 管理员服务
    utils::JsonValue addDoctor(const utils::JsonValue& params);
    utils::JsonValue updateDoctor(const utils::JsonValue& params);
    utils::JsonValue deleteDoctor(int doctor_id);
    
private:
    // 排队队列管理
    QString generateQueueNumber(int dept_id);
    int calculateEstimatedWait(int dept_id, int doctor_id);
    void updateQueuePositions(int dept_id);
    void advanceQueue(int dept_id, int doctor_id);
    
    // 数据库访问
    std::shared_ptr<ConnectionPool> db_pool_;
    
    // 内存中的排队队列
    QMap<int, QQueue<QueueItem>> department_queues_;  // dept_id -> queue
    QMap<QString, QueueItem> all_queue_items_;  // queue_number -> item
    QMutex queue_mutex_;
    
    // 当前接诊映射
    QMap<int, QueueItem> current_consultations_;  // doctor_id -> queue_item
    QMap<int, int> patient_doctor_map_;  // patient_id -> doctor_id
    
    // B超室状态
    QMap<int, UltrasoundAppointment> ultrasound_appointments_;  // appt_id -> appt
    QVector<bool> ultrasound_machines_available_;  // 3台机器状态
    
    // 医生状态
    QMap<int, Doctor> doctors_;
    
    // 定时任务
    QTimer* queue_update_timer_;
    
    // 统计
    int total_registrations_today_;
    int total_consultations_today_;
};

// =============================================================================
// 消息路由器
// =============================================================================
class MessageRouter : public MessageHandler {
    Q_OBJECT
    
public:
    explicit MessageRouter(BusinessService* service, QObject* parent = nullptr);
    ~MessageRouter() override;
    
    void handleMessage(const QString& connection_id, const QString& message) override;
    void onClientConnected(const QString& connection_id) override;
    void onClientDisconnected(const QString& connection_id) override;
    void onHeartbeat(const QString& connection_id) override;
    
    // 发送响应
    void sendResponse(const QString& connection_id, uint32_t sequence, 
                     StatusCode code, const utils::JsonValue& data);
    void sendError(const QString& connection_id, uint32_t sequence, StatusCode code);
    
signals:
    void responseReady(const QString& connection_id, const QString& message);

private:
    utils::JsonValue parseCommand(const QString& message);
    uint32_t getSequence(const utils::JsonValue& json);
    
    BusinessService* service_;
    QMap<QString, QString> connection_sessions_;  // connection_id -> session_id
    QMutex sessions_mutex_;
    
    // 统计
    QAtomicInt total_messages_;
    QAtomicInt error_messages_;
};

// =============================================================================
// 内联实现
// =============================================================================

inline MessageRouter::MessageRouter(BusinessService* service, QObject* parent)
    : MessageHandler(parent)
    , service_(service)
{
}

inline MessageRouter::~MessageRouter() {}

inline void MessageRouter::onClientConnected(const QString& connection_id) {
    LOG_INFO("Client connected for routing: " + connection_id);
}

inline void MessageRouter::onClientDisconnected(const QString& connection_id) {
    QMutexLocker lock(&sessions_mutex_);
    connection_sessions_.remove(connection_id);
    LOG_INFO("Client disconnected from routing: " + connection_id);
}

inline void MessageRouter::onHeartbeat(const QString& connection_id) {
    LOG_DEBUG("Heartbeat from: " + connection_id);
}

inline utils::JsonValue MessageRouter::parseCommand(const QString& message) {
    try {
        return utils::JsonValue::parse(message.toStdString());
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to parse message: " + QString(e.what()));
        return utils::JsonValue();
    }
}

inline uint32_t MessageRouter::getSequence(const utils::JsonValue& json) {
    if (json.isObject() && json.has("seq")) {
        return json["seq"].asInt();
    }
    return 0;
}

} // namespace server
} // namespace smartsched
