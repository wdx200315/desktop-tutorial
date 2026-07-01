/**
 * @file service.cpp
 * @brief 业务服务实现
 */

#include "smartsched/server/service.h"
#include <QDateTime>
#include <QMutexLocker>

namespace smartsched {
namespace server {

// =============================================================================
// BusinessService实现
// =============================================================================

BusinessService::BusinessService(std::shared_ptr<ConnectionPool> db_pool, QObject* parent)
    : QObject(parent)
    , db_pool_(db_pool)
{
    // 初始化B超机器状态
    ultrasound_machines_available_.resize(4);  // 索引1-3使用
    for (int i = 1; i <= 3; ++i) {
        ultrasound_machines_available_[i] = true;
    }
    
    // 定时器：每秒更新排队状态
    queue_update_timer_ = new QTimer(this);
    connect(queue_update_timer_, &QTimer::timeout, this, &BusinessService::updateQueuePositions);
}

BusinessService::~BusinessService() {
    shutdown();
}

bool BusinessService::initialize() {
    LOG_INFO("Initializing BusinessService...");
    
    // 从数据库加载科室信息
    auto conn = db_pool_->getConnection();
    if (!conn) {
        LOG_ERROR("Failed to get database connection for initialization");
        return false;
    }
    
    // 加载医生信息
    if (conn->query("SELECT doctor_id, dept_id, name, title, is_active FROM doctors WHERE is_active = 1")) {
        while (conn->nextRow()) {
            Doctor doc;
            doc.doctor_id = conn->getInt("doctor_id");
            doc.dept_id = conn->getInt("dept_id");
            doc.name = QString::fromStdString(conn->getString("name"));
            doc.title = QString::fromStdString(conn->getString("title"));
            doc.is_active = true;
            doc.is_available = true;
            doc.current_patient_count = 0;
            doctors_[doc.doctor_id] = doc;
        }
    }
    
    // 加载当前排队信息
    if (conn->query("SELECT * FROM queue WHERE status = 0 ORDER BY join_time")) {
        while (conn->nextRow()) {
            QueueItem item;
            item.queue_id = conn->getInt("queue_id");
            item.patient_id = conn->getInt("patient_id");
            item.dept_id = conn->getInt("dept_id");
            item.doctor_id = conn->getInt("doctor_id");
            item.queue_number = QString::fromStdString(conn->getString("queue_number"));
            item.position = conn->getInt("position");
            item.join_time = conn->getLong("join_time");
            item.status = conn->getInt("status");
            
            department_queues_[item.dept_id].push_back(item);
            all_queue_items_[item.queue_number] = item;
        }
    }
    
    // 启动排队更新定时器
    queue_update_timer_->start(1000);
    
    LOG_INFO("BusinessService initialized successfully");
    return true;
}

void BusinessService::shutdown() {
    if (queue_update_timer_) {
        queue_update_timer_->stop();
    }
    LOG_INFO("BusinessService shutdown");
}

utils::JsonValue BusinessService::handleCommand(const QString& connection_id, 
                                                CommandID cmd, 
                                                const utils::JsonValue& params) {
    LOG_DEBUG("Handling command: " + QString::fromStdString(commandToString(cmd)));
    
    switch (cmd) {
        // 科室
        case CommandID::DEPT_LIST:
            return getDepartmentList();
        case CommandID::DEPT_INFO:
            return utils::JsonValue::object()[QString::number(cmd).toStdString()] = 
                   getDepartmentInfo(params["dept_id"].asInt()).asObject(), utils::JsonValue::object();
        
        // 医生
        case CommandID::DOCTOR_LIST:
            return getDoctorList(params["dept_id"].asInt());
        case CommandID::DOCTOR_INFO:
            return getDoctorInfo(params["doctor_id"].asInt());
        
        // 挂号
        case CommandID::REGISTER:
            return registerPatient(params);
        case CommandID::CANCEL_REGISTER:
            return cancelRegistration(params["reg_id"].asInt());
        case CommandID::GET_QUEUE_NUM:
            return getQueueNumber(params["patient_id"].asInt(), params["dept_id"].asInt());
        
        // 排队
        case CommandID::QUEUE_STATUS:
            return getQueueStatus(params["patient_id"].asInt());
        case CommandID::ESTIMATED_WAIT:
            return getEstimatedWaitTime(params["patient_id"].asInt());
        
        // 医生接诊
        case CommandID::CONSULT_START:
            return startConsultation(params["doctor_id"].asInt(), params["patient_id"].asInt());
        case CommandID::CONSULT_END:
            return endConsultation(params["doctor_id"].asInt(), params["patient_id"].asInt());
        case CommandID::CONSULT_PAUSE:
            return pauseConsultation(params["doctor_id"].asInt());
        
        // 患者信息
        case CommandID::PATIENT_INFO:
            return getPatientInfo(params["patient_id"].asInt());
        case CommandID::PATIENT_HISTORY:
            return getPatientHistory(params["patient_id"].asInt());
        
        // B超
        case CommandID::ULTRASOUND_REQUEST:
            return requestUltrasound(params["patient_id"].asInt(), params["doctor_id"].asInt());
        case CommandID::ULTRASOUND_STATUS:
            return getUltrasoundStatus();
        case CommandID::ULTRASOUND_COMPLETE:
            return completeUltrasound(params["appt_id"].asInt());
        
        // 叫号
        case CommandID::CALL_NEXT:
            return callNextPatient(params["doctor_id"].asInt());
        case CommandID::CALL_SPECIFIC:
            return callSpecificPatient(params["doctor_id"].asInt(), 
                                     params["queue_number"].asString().c_str());
        case CommandID::RECALL:
            return recallPatient(params["doctor_id"].asInt());
        case CommandID::SKIP:
            return skipPatient(params["doctor_id"].asInt());
        
        // 统计
        case CommandID::STATISTICS:
            return getStatistics();
        case CommandID::STATISTICS_DAILY:
            return getDailyStatistics(params["date"].asString().c_str());
        
        // 管理员
        case CommandID::DOCTOR_ADD:
            return addDoctor(params);
        case CommandID::DOCTOR_UPDATE:
            return updateDoctor(params);
        case CommandID::DOCTOR_DELETE:
            return deleteDoctor(params["doctor_id"].asInt());
        
        default:
            LOG_WARN("Unhandled command: " + QString::number(static_cast<int>(cmd)));
            return utils::JsonValue::object();
    }
}

utils::JsonValue BusinessService::getDepartmentList() {
    utils::JsonValue result = utils::JsonValue::array();
    
    auto conn = db_pool_->getConnection();
    if (!conn) return result;
    
    if (conn->query("SELECT * FROM departments WHERE is_active = 1")) {
        int idx = 0;
        while (conn->nextRow()) {
            result[idx] = utils::JsonValue::object();
            result[idx]["dept_id"] = conn->getInt("dept_id");
            result[idx]["dept_name"] = conn->getString("name").c_str();
            result[idx]["description"] = conn->getString("description").c_str();
            result[idx]["queue_capacity"] = conn->getInt("queue_capacity");
            result[idx]["current_queue_size"] = department_queues_[conn->getInt("dept_id")].size();
            idx++;
        }
    }
    
    return result;
}

utils::JsonValue BusinessService::getDepartmentInfo(int dept_id) {
    utils::JsonValue result = utils::JsonValue::object();
    
    auto conn = db_pool_->getConnection();
    if (!conn) return result;
    
    std::string sql = "SELECT * FROM departments WHERE dept_id = " + std::to_string(dept_id);
    if (conn->query(sql) && conn->nextRow()) {
        result["dept_id"] = conn->getInt("dept_id");
        result["dept_name"] = conn->getString("name").c_str();
        result["description"] = conn->getString("description").c_str();
        result["queue_capacity"] = conn->getInt("queue_capacity");
        result["current_queue_size"] = department_queues_[dept_id].size();
    }
    
    return result;
}

utils::JsonValue BusinessService::getDoctorList(int dept_id) {
    utils::JsonValue result = utils::JsonValue::array();
    
    auto conn = db_pool_->getConnection();
    if (!conn) return result;
    
    std::string sql = "SELECT * FROM doctors WHERE dept_id = " + std::to_string(dept_id) + 
                      " AND is_active = 1";
    if (conn->query(sql)) {
        int idx = 0;
        while (conn->nextRow()) {
            result[idx] = utils::JsonValue::object();
            result[idx]["doctor_id"] = conn->getInt("doctor_id");
            result[idx]["name"] = conn->getString("name").c_str();
            result[idx]["title"] = conn->getString("title").c_str();
            result[idx]["is_available"] = doctors_.value(conn->getInt("doctor_id")).is_available;
            idx++;
        }
    }
    
    return result;
}

utils::JsonValue BusinessService::getDoctorInfo(int doctor_id) {
    utils::JsonValue result = utils::JsonValue::object();
    
    if (doctors_.contains(doctor_id)) {
        const Doctor& doc = doctors_[doctor_id];
        result["doctor_id"] = doc.doctor_id;
        result["name"] = doc.name.toStdString();
        result["title"] = doc.title.toStdString();
        result["dept_id"] = doc.dept_id;
        result["is_available"] = doc.is_available;
    }
    
    return result;
}

utils::JsonValue BusinessService::registerPatient(const utils::JsonValue& params) {
    utils::JsonValue result = utils::JsonValue::object();
    
    int patient_id = params["patient_id"].asInt();
    int dept_id = params["dept_id"].asInt();
    int doctor_id = params["doctor_id"].asInt();
    
    QMutexLocker lock(&queue_mutex_);
    
    // 检查科室排队是否已满
    if (department_queues_[dept_id].size() >= 50) {  // 默认容量
        result["success"] = false;
        result["error"] = "Queue is full";
        return result;
    }
    
    // 生成排队号
    QString queue_number = generateQueueNumber(dept_id);
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    
    // 创建排队项
    QueueItem item;
    item.queue_id = 0;  // 将由数据库生成
    item.patient_id = patient_id;
    item.dept_id = dept_id;
    item.doctor_id = doctor_id;
    item.queue_number = queue_number;
    item.position = department_queues_[dept_id].size() + 1;
    item.join_time = now;
    item.estimated_start_time = now + calculateEstimatedWait(dept_id, doctor_id) * 1000;
    item.status = 0;
    
    // 保存到数据库
    auto conn = db_pool_->getConnection();
    if (!conn) {
        result["success"] = false;
        result["error"] = "Database error";
        return result;
    }
    
    std::string sql = "INSERT INTO queue (patient_id, dept_id, doctor_id, queue_number, " \
                      "position, join_time, status) VALUES (" +
                      std::to_string(patient_id) + ", " +
                      std::to_string(dept_id) + ", " +
                      std::to_string(doctor_id) + ", '" +
                      queue_number.toStdString() + "', " +
                      std::to_string(item.position) + ", " +
                      std::to_string(now) + ", 0)";
    
    if (conn->execute(sql)) {
        item.queue_id = static_cast<int>(mysql_insert_id(conn->getHandle()));
        department_queues_[dept_id].push_back(item);
        all_queue_items_[queue_number] = item;
        
        result["success"] = true;
        result["queue_number"] = queue_number.toStdString();
        result["position"] = item.position;
        result["estimated_wait"] = item.estimated_start_time - now;
        
        total_registrations_today_++;
    } else {
        result["success"] = false;
        result["error"] = "Failed to register";
    }
    
    return result;
}

utils::JsonValue BusinessService::cancelRegistration(int reg_id) {
    utils::JsonValue result = utils::JsonValue::object();
    
    QMutexLocker lock(&queue_mutex_);
    
    auto conn = db_pool_->getConnection();
    if (!conn) {
        result["success"] = false;
        result["error"] = "Database error";
        return result;
    }
    
    std::string sql = "UPDATE queue SET status = 3 WHERE queue_id = " + std::to_string(reg_id);
    if (conn->execute(sql)) {
        result["success"] = true;
    } else {
        result["success"] = false;
        result["error"] = "Failed to cancel";
    }
    
    return result;
}

utils::JsonValue BusinessService::getQueueNumber(int patient_id, int dept_id) {
    utils::JsonValue result = utils::JsonValue::object();
    
    QMutexLocker lock(&queue_mutex_);
    
    for (const QueueItem& item : department_queues_[dept_id]) {
        if (item.patient_id == patient_id && item.status == 0) {
            result["queue_number"] = item.queue_number.toStdString();
            result["position"] = item.position;
            result["success"] = true;
            return result;
        }
    }
    
    result["success"] = false;
    result["error"] = "Not in queue";
    return result;
}

utils::JsonValue BusinessService::getQueueStatus(int patient_id) {
    utils::JsonValue result = utils::JsonValue::object();
    
    QMutexLocker lock(&queue_mutex_);
    
    for (auto it = all_queue_items_.begin(); it != all_queue_items_.end(); ++it) {
        const QueueItem& item = it.value();
        if (item.patient_id == patient_id) {
            result["queue_number"] = item.queue_number.toStdString();
            result["position"] = item.position;
            result["dept_id"] = item.dept_id;
            result["status"] = item.status;
            result["join_time"] = item.join_time;
            result["estimated_wait"] = calculateEstimatedWait(item.dept_id, item.doctor_id);
            result["success"] = true;
            return result;
        }
    }
    
    result["success"] = false;
    result["error"] = "Not in queue";
    return result;
}

utils::JsonValue BusinessService::getEstimatedWaitTime(int patient_id) {
    auto status = getQueueStatus(patient_id);
    if (status["success"].asBool()) {
        utils::JsonValue result = utils::JsonValue::object();
        result["estimated_seconds"] = status["estimated_wait"];
        return result;
    }
    return status;
}

utils::JsonValue BusinessService::getAllQueueStatus(int dept_id) {
    utils::JsonValue result = utils::JsonValue::array();
    
    QMutexLocker lock(&queue_mutex_);
    
    int idx = 0;
    for (const QueueItem& item : department_queues_[dept_id]) {
        result[idx] = utils::JsonValue::object();
        result[idx]["queue_number"] = item.queue_number.toStdString();
        result[idx]["position"] = item.position;
        result[idx]["status"] = item.status;
        result[idx]["join_time"] = item.join_time;
        idx++;
    }
    
    return result;
}

utils::JsonValue BusinessService::startConsultation(int doctor_id, int patient_id) {
    utils::JsonValue result = utils::JsonValue::object();
    
    QMutexLocker lock(&queue_mutex_);
    
    // 查找排队项
    QString queue_number;
    int dept_id = 0;
    for (auto it = all_queue_items_.begin(); it != all_queue_items_.end(); ++it) {
        if (it.value().patient_id == patient_id && it.value().status == 0) {
            queue_number = it.key();
            dept_id = it.value().dept_id;
            break;
        }
    }
    
    if (queue_number.isEmpty()) {
        result["success"] = false;
        result["error"] = "Patient not in queue";
        return result;
    }
    
    // 更新状态
    QueueItem& item = all_queue_items_[queue_number];
    item.status = 1;  // 正在就诊
    
    // 从排队队列移到就诊中
    QQueue<QueueItem>& queue = department_queues_[dept_id];
    for (int i = 0; i < queue.size(); ++i) {
        if (queue[i].queue_number == queue_number) {
            queue.removeAt(i);
            break;
        }
    }
    
    current_consultations_[doctor_id] = item;
    patient_doctor_map_[patient_id] = doctor_id;
    
    // 更新医生状态
    if (doctors_.contains(doctor_id)) {
        doctors_[doctor_id].is_available = false;
    }
    
    result["success"] = true;
    result["consultation_start_time"] = QDateTime::currentMSecsSinceEpoch();
    
    total_consultations_today_++;
    
    emit consultationStarted(patient_id, doctor_id);
    
    return result;
}

utils::JsonValue BusinessService::endConsultation(int doctor_id, int patient_id) {
    utils::JsonValue result = utils::JsonValue::object();
    
    QMutexLocker lock(&queue_mutex_);
    
    qint64 end_time = QDateTime::currentMSecsSinceEpoch();
    
    // 更新就诊记录
    auto conn = db_pool_->getConnection();
    if (conn) {
        std::string sql = "INSERT INTO records (patient_id, doctor_id, start_time, end_time) " \
                         "VALUES (" +
                         std::to_string(patient_id) + ", " +
                         std::to_string(doctor_id) + ", " +
                         std::to_string(end_time - 300000) + ", " +  // 假设5分钟
                         std::to_string(end_time) + ")";
        conn->execute(sql);
    }
    
    // 清理
    current_consultations_.remove(doctor_id);
    patient_doctor_map_.remove(patient_id);
    
    // 更新医生状态
    if (doctors_.contains(doctor_id)) {
        doctors_[doctor_id].is_available = true;
    }
    
    // 从排队列表移除
    for (auto it = all_queue_items_.begin(); it != all_queue_items_.end(); ++it) {
        if (it.value().patient_id == patient_id) {
            all_queue_items_.erase(it);
            break;
        }
    }
    
    result["success"] = true;
    result["consultation_end_time"] = end_time;
    
    emit consultationEnded(patient_id, doctor_id);
    
    return result;
}

utils::JsonValue BusinessService::pauseConsultation(int doctor_id) {
    utils::JsonValue result = utils::JsonValue::object();
    
    QMutexLocker lock(&queue_mutex_);
    
    if (current_consultations_.contains(doctor_id)) {
        QueueItem& item = current_consultations_[doctor_id];
        item.status = 0;  // 恢复等待
        
        // 放回排队队列
        department_queues_[item.dept_id].push_front(item);
        current_consultations_.remove(doctor_id);
        
        if (doctors_.contains(doctor_id)) {
            doctors_[doctor_id].is_available = true;
        }
        
        result["success"] = true;
    } else {
        result["success"] = false;
        result["error"] = "No active consultation";
    }
    
    return result;
}

utils::JsonValue BusinessService::getPatientInfo(int patient_id) {
    utils::JsonValue result = utils::JsonValue::object();
    
    auto conn = db_pool_->getConnection();
    if (!conn) return result;
    
    std::string sql = "SELECT * FROM patients WHERE patient_id = " + std::to_string(patient_id);
    if (conn->query(sql) && conn->nextRow()) {
        result["patient_id"] = conn->getInt("patient_id");
        result["name"] = conn->getString("name").c_str();
        result["age"] = conn->getInt("age");
        result["gender"] = conn->getString("gender").c_str();
        result["phone"] = conn->getString("phone").c_str();
    }
    
    return result;
}

utils::JsonValue BusinessService::getPatientHistory(int patient_id) {
    utils::JsonValue result = utils::JsonValue::array();
    
    auto conn = db_pool_->getConnection();
    if (!conn) return result;
    
    std::string sql = "SELECT r.*, d.name as doctor_name, dept.dept_name " \
                     "FROM records r " \
                     "JOIN doctors d ON r.doctor_id = d.doctor_id " \
                     "JOIN departments dept ON d.dept_id = dept.dept_id " \
                     "WHERE r.patient_id = " + std::to_string(patient_id) + \
                     " ORDER BY r.start_time DESC LIMIT 10";
    
    if (conn->query(sql)) {
        int idx = 0;
        while (conn->nextRow()) {
            result[idx] = utils::JsonValue::object();
            result[idx]["record_id"] = conn->getInt("record_id");
            result[idx]["doctor_name"] = conn->getString("doctor_name").c_str();
            result[idx]["dept_name"] = conn->getString("dept_name").c_str();
            result[idx]["start_time"] = conn->getLong("start_time");
            result[idx]["end_time"] = conn->getLong("end_time");
            result[idx]["diagnosis"] = conn->getString("diagnosis").c_str();
            idx++;
        }
    }
    
    return result;
}

utils::JsonValue BusinessService::requestUltrasound(int patient_id, int doctor_id) {
    utils::JsonValue result = utils::JsonValue::object();
    
    QMutexLocker lock(&queue_mutex_);
    
    // 查找空闲的B超机器
    int machine_id = -1;
    for (int i = 1; i <= 3; ++i) {
        if (ultrasound_machines_available_[i]) {
            machine_id = i;
            break;
        }
    }
    
    if (machine_id == -1) {
        result["success"] = false;
        result["error"] = "No ultrasound machine available";
        result["queued"] = true;  // 标记需要排队
        return result;
    }
    
    // 创建B超预约
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    UltrasoundAppointment appt;
    appt.appt_id = 0;
    appt.patient_id = patient_id;
    appt.machine_id = machine_id;
    appt.appointment_time = now;
    appt.start_time = now;
    appt.status = 1;
    
    // 保存到数据库
    auto conn = db_pool_->getConnection();
    if (conn) {
        std::string sql = "INSERT INTO ultrasound (patient_id, machine_id, appointment_time, status) " \
                         "VALUES (" +
                         std::to_string(patient_id) + ", " +
                         std::to_string(machine_id) + ", " +
                         std::to_string(now) + ", 1)";
        if (conn->execute(sql)) {
            appt.appt_id = static_cast<int>(mysql_insert_id(conn->getHandle()));
            ultrasound_appointments_[appt.appt_id] = appt;
            ultrasound_machines_available_[machine_id] = false;
            
            result["success"] = true;
            result["appt_id"] = appt.appt_id;
            result["machine_id"] = machine_id;
            result["start_time"] = now;
        }
    }
    
    return result;
}

utils::JsonValue BusinessService::getUltrasoundStatus() {
    utils::JsonValue result = utils::JsonValue::array();
    
    QMutexLocker lock(&queue_mutex_);
    
    int idx = 0;
    for (const auto& [id, appt] : ultrasound_appointments_) {
        result[idx] = utils::JsonValue::object();
        result[idx]["appt_id"] = appt.appt_id;
        result[idx]["patient_id"] = appt.patient_id;
        result[idx]["machine_id"] = appt.machine_id;
        result[idx]["status"] = appt.status;
        result[idx]["start_time"] = appt.start_time;
        idx++;
    }
    
    return result;
}

utils::JsonValue BusinessService::completeUltrasound(int appt_id) {
    utils::JsonValue result = utils::JsonValue::object();
    
    QMutexLocker lock(&queue_mutex_);
    
    if (!ultrasound_appointments_.contains(appt_id)) {
        result["success"] = false;
        result["error"] = "Appointment not found";
        return result;
    }
    
    UltrasoundAppointment& appt = ultrasound_appointments_[appt_id];
    appt.end_time = QDateTime::currentMSecsSinceEpoch();
    appt.status = 2;
    
    // 释放B超机器
    ultrasound_machines_available_[appt.machine_id] = true;
    
    // 更新数据库
    auto conn = db_pool_->getConnection();
    if (conn) {
        std::string sql = "UPDATE ultrasound SET end_time = " +
                          std::to_string(appt.end_time) +
                          ", status = 2 WHERE appt_id = " +
                          std::to_string(appt_id);
        conn->execute(sql);
    }
    
    result["success"] = true;
    result["duration"] = (appt.end_time - appt.start_time) / 1000;
    
    return result;
}

utils::JsonValue BusinessService::callNextPatient(int doctor_id) {
    utils::JsonValue result = utils::JsonValue::object();
    
    QMutexLocker lock(&queue_mutex_);
    
    if (!doctors_.contains(doctor_id)) {
        result["success"] = false;
        result["error"] = "Doctor not found";
        return result;
    }
    
    int dept_id = doctors_[doctor_id].dept_id;
    
    if (department_queues_[dept_id].isEmpty()) {
        result["success"] = false;
        result["error"] = "No patients in queue";
        return result;
    }
    
    // 获取队首患者
    QueueItem item = department_queues_[dept_id].front();
    
    // 开始接诊
    result = startConsultation(doctor_id, item.patient_id);
    
    if (result["success"].asBool()) {
        emit patientCalled(item.queue_number);
    }
    
    return result;
}

utils::JsonValue BusinessService::callSpecificPatient(int doctor_id, const QString& queue_number) {
    utils::JsonValue result = utils::JsonValue::object();
    
    QMutexLocker lock(&queue_mutex_);
    
    if (!all_queue_items_.contains(queue_number)) {
        result["success"] = false;
        result["error"] = "Patient not found";
        return result;
    }
    
    QueueItem& item = all_queue_items_[queue_number];
    result = startConsultation(doctor_id, item.patient_id);
    
    if (result["success"].asBool()) {
        emit patientCalled(queue_number);
    }
    
    return result;
}

utils::JsonValue BusinessService::recallPatient(int doctor_id) {
    utils::JsonValue result = utils::JsonValue::object();
    
    if (!current_consultations_.contains(doctor_id)) {
        result["success"] = false;
        result["error"] = "No current patient";
        return result;
    }
    
    const QueueItem& item = current_consultations_[doctor_id];
    emit patientCalled(item.queue_number);
    
    result["success"] = true;
    result["queue_number"] = item.queue_number.toStdString();
    
    return result;
}

utils::JsonValue BusinessService::skipPatient(int doctor_id) {
    utils::JsonValue result = utils::JsonValue::object();
    
    QMutexLocker lock(&queue_mutex_);
    
    if (!current_consultations_.contains(doctor_id)) {
        result["success"] = false;
        result["error"] = "No current patient";
        return result;
    }
    
    QueueItem item = current_consultations_[doctor_id];
    item.status = 0;
    item.position = department_queues_[item.dept_id].size() + 1;
    
    // 移到队尾
    department_queues_[item.dept_id].push_back(item);
    all_queue_items_[item.queue_number] = item;
    current_consultations_.remove(doctor_id);
    
    if (doctors_.contains(doctor_id)) {
        doctors_[doctor_id].is_available = true;
    }
    
    result["success"] = true;
    result["queue_number"] = item.queue_number.toStdString();
    result["new_position"] = item.position;
    
    return result;
}

utils::JsonValue BusinessService::getStatistics() {
    utils::JsonValue result = utils::JsonValue::object();
    
    result["today_registrations"] = total_registrations_today_;
    result["today_consultations"] = total_consultations_today_;
    result["total_queue_size"] = all_queue_items_.size();
    result["active_consultations"] = current_consultations_.size();
    result["ultrasound_available"] = 
        static_cast<int>(std::count(ultrasound_machines_available_.begin() + 1,
                                   ultrasound_machines_available_.end(), true));
    
    return result;
}

utils::JsonValue BusinessService::getDailyStatistics(const QString& date) {
    utils::JsonValue result = utils::JsonValue::object();
    
    auto conn = db_pool_->getConnection();
    if (!conn) return result;
    
    std::string sql = "SELECT COUNT(*) as total, " \
                     "SUM(CASE WHEN status = 2 THEN 1 ELSE 0 END) as completed " \
                     "FROM records WHERE DATE(FROM_UNIXTIME(start_time/1000)) = '" +
                     date.toStdString() + "'";
    
    if (conn->query(sql) && conn->nextRow()) {
        result["total_registrations"] = conn->getInt("total");
        result["completed_consultations"] = conn->getInt("completed");
    }
    
    return result;
}

utils::JsonValue BusinessService::getMonthlyStatistics(int year, int month) {
    utils::JsonValue result = utils::JsonValue::array();
    
    auto conn = db_pool_->getConnection();
    if (!conn) return result;
    
    std::string sql = "SELECT DATE(FROM_UNIXTIME(start_time/1000)) as date, " \
                     "COUNT(*) as count " \
                     "FROM records " \
                     "WHERE YEAR(DATE(FROM_UNIXTIME(start_time/1000))) = " +
                     std::to_string(year) + " AND " \
                     "MONTH(DATE(FROM_UNIXTIME(start_time/1000))) = " +
                     std::to_string(month) + " " \
                     "GROUP BY DATE(FROM_UNIXTIME(start_time/1000))";
    
    if (conn->query(sql)) {
        int idx = 0;
        while (conn->nextRow()) {
            result[idx] = utils::JsonValue::object();
            result[idx]["date"] = conn->getString("date").c_str();
            result[idx]["count"] = conn->getInt("count");
            idx++;
        }
    }
    
    return result;
}

utils::JsonValue BusinessService::addDoctor(const utils::JsonValue& params) {
    utils::JsonValue result = utils::JsonValue::object();
    
    auto conn = db_pool_->getConnection();
    if (!conn) {
        result["success"] = false;
        result["error"] = "Database error";
        return result;
    }
    
    std::string sql = "INSERT INTO doctors (dept_id, name, title, is_active) VALUES (" +
                     std::to_string(params["dept_id"].asInt()) + ", '" +
                     params["name"].asString() + "', '" +
                     params["title"].asString() + "', 1)";
    
    if (conn->execute(sql)) {
        int doctor_id = static_cast<int>(mysql_insert_id(conn->getHandle()));
        
        Doctor doc;
        doc.doctor_id = doctor_id;
        doc.dept_id = params["dept_id"].asInt();
        doc.name = params["name"].asString().c_str();
        doc.title = params["title"].asString().c_str();
        doc.is_active = true;
        doc.is_available = true;
        
        doctors_[doctor_id] = doc;
        
        result["success"] = true;
        result["doctor_id"] = doctor_id;
    } else {
        result["success"] = false;
        result["error"] = "Failed to add doctor";
    }
    
    return result;
}

utils::JsonValue BusinessService::updateDoctor(const utils::JsonValue& params) {
    utils::JsonValue result = utils::JsonValue::object();
    
    int doctor_id = params["doctor_id"].asInt();
    
    if (!doctors_.contains(doctor_id)) {
        result["success"] = false;
        result["error"] = "Doctor not found";
        return result;
    }
    
    auto conn = db_pool_->getConnection();
    if (!conn) {
        result["success"] = false;
        result["error"] = "Database error";
        return result;
    }
    
    std::string sql = "UPDATE doctors SET name = '" + params["name"].asString() +
                     "', title = '" + params["title"].asString() +
                     "' WHERE doctor_id = " + std::to_string(doctor_id);
    
    if (conn->execute(sql)) {
        Doctor& doc = doctors_[doctor_id];
        doc.name = params["name"].asString().c_str();
        doc.title = params["title"].asString().c_str();
        
        result["success"] = true;
    } else {
        result["success"] = false;
        result["error"] = "Failed to update";
    }
    
    return result;
}

utils::JsonValue BusinessService::deleteDoctor(int doctor_id) {
    utils::JsonValue result = utils::JsonValue::object();
    
    auto conn = db_pool_->getConnection();
    if (!conn) {
        result["success"] = false;
        result["error"] = "Database error";
        return result;
    }
    
    std::string sql = "UPDATE doctors SET is_active = 0 WHERE doctor_id = " +
                     std::to_string(doctor_id);
    
    if (conn->execute(sql)) {
        if (doctors_.contains(doctor_id)) {
            doctors_[doctor_id].is_active = false;
        }
        result["success"] = true;
    } else {
        result["success"] = false;
        result["error"] = "Failed to delete";
    }
    
    return result;
}

QString BusinessService::generateQueueNumber(int dept_id) {
    static QMap<int, int> counters;
    int counter = ++counters[dept_id];
    return QString("D%1-%2").arg(dept_id, 2, 10, QChar('0')).arg(counter, 4, 10, QChar('0'));
}

int BusinessService::calculateEstimatedWait(int dept_id, int doctor_id) {
    QMutexLocker lock(&queue_mutex_);
    
    int queue_size = department_queues_[dept_id].size();
    int avg_consult_time = 300;  // 假设平均5分钟
    
    return queue_size * avg_consult_time;
}

void BusinessService::updateQueuePositions(int dept_id) {
    QMutexLocker lock(&queue_mutex_);
    
    QQueue<QueueItem>& queue = department_queues_[dept_id];
    for (int i = 0; i < queue.size(); ++i) {
        queue[i].position = i + 1;
        QString queue_number = queue[i].queue_number;
        if (all_queue_items_.contains(queue_number)) {
            all_queue_items_[queue_number].position = i + 1;
        }
    }
}

// =============================================================================
// MessageRouter实现
// =============================================================================

void MessageRouter::handleMessage(const QString& connection_id, const QString& message) {
    total_messages_++;
    
    try {
        auto json = parseCommand(message);
        if (json.isNull()) {
            sendError(connection_id, 0, StatusCode::InvalidRequest);
            error_messages_++;
            return;
        }
        
        uint32_t seq = getSequence(json);
        int cmd_id = json["cmd"].asInt();
        auto cmd = static_cast<CommandID>(cmd_id);
        
        LOG_DEBUG("Routing command: " + QString::fromStdString(commandToString(cmd)));
        
        auto result = service_->handleCommand(connection_id, cmd, json["body"]);
        
        // 构建响应
        utils::JsonValue response = utils::JsonValue::object();
        response["seq"] = seq;
        response["cmd"] = cmd_id;
        response["data"] = result;
        
        QString resp_str = QString::fromStdString(response.serialize());
        emit responseReady(connection_id, resp_str);
        
    } catch (const std::exception& e) {
        LOG_ERROR("Message routing error: " + QString(e.what()));
        error_messages_++;
        sendError(connection_id, 0, StatusCode::InternalError);
    }
}

void MessageRouter::sendResponse(const QString& connection_id, uint32_t sequence, 
                                 StatusCode code, const utils::JsonValue& data) {
    utils::JsonValue response = utils::JsonValue::object();
    response["seq"] = static_cast<int>(sequence);
    response["status"] = static_cast<int>(code);
    response["data"] = data;
    
    emit responseReady(connection_id, QString::fromStdString(response.serialize()));
}

void MessageRouter::sendError(const QString& connection_id, uint32_t sequence, StatusCode code) {
    sendResponse(connection_id, sequence, code, utils::JsonValue::object());
}

} // namespace server
} // namespace smartsched
