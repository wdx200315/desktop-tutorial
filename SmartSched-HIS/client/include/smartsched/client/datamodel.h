/**
 * @file datamodel.h
 * @brief 数据模型
 */

#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QString>
#include "../../common/include/smartsched/utils/json_helper.h"

namespace smartsched {
namespace client {

// =============================================================================
// 科室模型
// =============================================================================
class DepartmentModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    
public:
    enum Roles {
        DeptIdRole = Qt::UserRole + 1,
        NameRole,
        DescriptionRole,
        QueueCapacityRole,
        CurrentQueueSizeRole
    };
    
    explicit DepartmentModel(QObject* parent = nullptr);
    
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    
    void setDepartments(const utils::JsonValue& departments);
    void clear();
    int count() const { return departments_.size(); }
    
    int getDeptId(int index) const;
    QString getName(int index) const;

private:
    QList<utils::JsonValue> departments_;
    
signals:
    void countChanged();
};

// =============================================================================
// 排队项模型
// =============================================================================
class QueueModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    
public:
    enum Roles {
        QueueNumberRole = Qt::UserRole + 1,
        PositionRole,
        PatientIdRole,
        DeptIdRole,
        StatusRole,
        JoinTimeRole,
        EstimatedWaitRole
    };
    
    explicit QueueModel(QObject* parent = nullptr);
    
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    
    void setQueue(const utils::JsonValue& queue);
    void addItem(const utils::JsonValue& item);
    void updateItem(int index, const utils::JsonValue& item);
    void removeItem(int index);
    void clear();
    int count() const { return items_.size(); }
    
    int getPosition(int index) const;
    QString getQueueNumber(int index) const;

private:
    QList<utils::JsonValue> items_;
    
signals:
    void countChanged();
    void patientCalled(const QString& queue_number);
};

// =============================================================================
// 患者模型
// =============================================================================
class PatientModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int currentPatientId READ currentPatientId WRITE setCurrentPatientId NOTIFY currentPatientIdChanged)
    
public:
    explicit PatientModel(QObject* parent = nullptr);
    
    void setCurrentPatient(const utils::JsonValue& patient);
    void setCurrentPatientId(int id) { current_patient_id_ = id; emit currentPatientIdChanged(); }
    int currentPatientId() const { return current_patient_id_; }
    
    QString name() const { return patient_name_; }
    int age() const { return patient_age_; }
    QString gender() const { return patient_gender_; }
    QString phone() const { return patient_phone_; }
    
private:
    int current_patient_id_;
    QString patient_name_;
    int patient_age_;
    QString patient_gender_;
    QString patient_phone_;
    
signals:
    void currentPatientIdChanged();
};

// =============================================================================
// 医生模型
// =============================================================================
class DoctorModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int currentDoctorId READ currentDoctorId WRITE setCurrentDoctorId NOTIFY currentDoctorIdChanged)
    
public:
    enum Roles {
        DoctorIdRole = Qt::UserRole + 1,
        NameRole,
        TitleRole,
        DeptIdRole,
        IsAvailableRole
    };
    
    explicit DoctorModel(QObject* parent = nullptr);
    
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    
    void setDoctors(const utils::JsonValue& doctors);
    void clear();
    
    void setCurrentDoctorId(int id) { current_doctor_id_ = id; emit currentDoctorIdChanged(); }
    int currentDoctorId() const { return current_doctor_id_; }
    bool isCurrentDoctorAvailable() const;
    
private:
    QList<utils::JsonValue> doctors_;
    int current_doctor_id_;
    
signals:
    void currentDoctorIdChanged();
};

// =============================================================================
// 内联实现
// =============================================================================

inline DepartmentModel::DepartmentModel(QObject* parent)
    : QAbstractListModel(parent) {}

inline int DepartmentModel::rowCount(const QModelIndex& parent) const {
    Q_UNUSED(parent);
    return departments_.size();
}

inline QVariant DepartmentModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= departments_.size()) {
        return QVariant();
    }
    
    const auto& dept = departments_[index.row()];
    
    switch (role) {
        case DeptIdRole:
        case Qt::DisplayRole:
            return dept["dept_id"].asInt();
        case NameRole:
            return QString::fromStdString(dept["dept_name"].asString());
        case DescriptionRole:
            return QString::fromStdString(dept["description"].asString());
        case QueueCapacityRole:
            return dept["queue_capacity"].asInt();
        case CurrentQueueSizeRole:
            return dept["current_queue_size"].asInt();
        default:
            return QVariant();
    }
}

inline QHash<int, QByteArray> DepartmentModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[DeptIdRole] = "deptId";
    roles[NameRole] = "name";
    roles[DescriptionRole] = "description";
    roles[QueueCapacityRole] = "queueCapacity";
    roles[CurrentQueueSizeRole] = "currentQueueSize";
    return roles;
}

inline void DepartmentModel::setDepartments(const utils::JsonValue& departments) {
    beginResetModel();
    departments_.clear();
    
    if (departments.isArray()) {
        for (const auto& dept : departments.asArray()) {
            departments_.append(dept);
        }
    }
    
    endResetModel();
    emit countChanged();
}

inline void DepartmentModel::clear() {
    beginResetModel();
    departments_.clear();
    endResetModel();
    emit countChanged();
}

inline int DepartmentModel::getDeptId(int index) const {
    if (index >= 0 && index < departments_.size()) {
        return departments_[index]["dept_id"].asInt();
    }
    return -1;
}

inline QString DepartmentModel::getName(int index) const {
    if (index >= 0 && index < departments_.size()) {
        return QString::fromStdString(departments_[index]["dept_name"].asString());
    }
    return QString();
}

// =============================================================================

inline QueueModel::QueueModel(QObject* parent)
    : QAbstractListModel(parent) {}

inline int QueueModel::rowCount(const QModelIndex& parent) const {
    Q_UNUSED(parent);
    return items_.size();
}

inline QVariant QueueModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= items_.size()) {
        return QVariant();
    }
    
    const auto& item = items_[index.row()];
    
    switch (role) {
        case QueueNumberRole:
        case Qt::DisplayRole:
            return QString::fromStdString(item["queue_number"].asString());
        case PositionRole:
            return item["position"].asInt();
        case PatientIdRole:
            return item["patient_id"].asInt();
        case DeptIdRole:
            return item["dept_id"].asInt();
        case StatusRole:
            return item["status"].asInt();
        case JoinTimeRole:
            return item["join_time"].asLong();
        default:
            return QVariant();
    }
}

inline QHash<int, QByteArray> QueueModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[QueueNumberRole] = "queueNumber";
    roles[PositionRole] = "position";
    roles[PatientIdRole] = "patientId";
    roles[DeptIdRole] = "deptId";
    roles[StatusRole] = "status";
    roles[JoinTimeRole] = "joinTime";
    return roles;
}

inline void QueueModel::setQueue(const utils::JsonValue& queue) {
    beginResetModel();
    items_.clear();
    
    if (queue.isArray()) {
        for (const auto& item : queue.asArray()) {
            items_.append(item);
        }
    }
    
    endResetModel();
    emit countChanged();
}

inline void QueueModel::addItem(const utils::JsonValue& item) {
    beginInsertRows(QModelIndex(), items_.size(), items_.size());
    items_.append(item);
    endInsertRows();
    emit countChanged();
}

inline void QueueModel::updateItem(int index, const utils::JsonValue& item) {
    if (index >= 0 && index < items_.size()) {
        items_[index] = item;
        emit dataChanged(index(index), index(index));
    }
}

inline void QueueModel::removeItem(int index) {
    if (index >= 0 && index < items_.size()) {
        beginRemoveRows(QModelIndex(), index, index);
        items_.removeAt(index);
        endRemoveRows();
        emit countChanged();
    }
}

inline void QueueModel::clear() {
    beginResetModel();
    items_.clear();
    endResetModel();
    emit countChanged();
}

inline int QueueModel::getPosition(int index) const {
    if (index >= 0 && index < items_.size()) {
        return items_[index]["position"].asInt();
    }
    return -1;
}

inline QString QueueModel::getQueueNumber(int index) const {
    if (index >= 0 && index < items_.size()) {
        return QString::fromStdString(items_[index]["queue_number"].asString());
    }
    return QString();
}

// =============================================================================

inline PatientModel::PatientModel(QObject* parent)
    : QAbstractListModel(parent)
    , current_patient_id_(-1)
    , patient_age_(0) {}

inline void PatientModel::setCurrentPatient(const utils::JsonValue& patient) {
    current_patient_id_ = patient["patient_id"].asInt();
    patient_name_ = QString::fromStdString(patient["name"].asString());
    patient_age_ = patient["age"].asInt();
    patient_gender_ = QString::fromStdString(patient["gender"].asString());
    patient_phone_ = QString::fromStdString(patient["phone"].asString());
    
    emit currentPatientIdChanged();
}

// =============================================================================

inline DoctorModel::DoctorModel(QObject* parent)
    : QAbstractListModel(parent)
    , current_doctor_id_(-1) {}

inline int DoctorModel::rowCount(const QModelIndex& parent) const {
    Q_UNUSED(parent);
    return doctors_.size();
}

inline QVariant DoctorModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= doctors_.size()) {
        return QVariant();
    }
    
    const auto& doctor = doctors_[index.row()];
    
    switch (role) {
        case DoctorIdRole:
        case Qt::DisplayRole:
            return doctor["doctor_id"].asInt();
        case NameRole:
            return QString::fromStdString(doctor["name"].asString());
        case TitleRole:
            return QString::fromStdString(doctor["title"].asString());
        case DeptIdRole:
            return doctor["dept_id"].asInt();
        case IsAvailableRole:
            return doctor["is_available"].asBool();
        default:
            return QVariant();
    }
}

inline QHash<int, QByteArray> DoctorModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[DoctorIdRole] = "doctorId";
    roles[NameRole] = "name";
    roles[TitleRole] = "title";
    roles[DeptIdRole] = "deptId";
    roles[IsAvailableRole] = "isAvailable";
    return roles;
}

inline void DoctorModel::setDoctors(const utils::JsonValue& doctors) {
    beginResetModel();
    doctors_.clear();
    
    if (doctors.isArray()) {
        for (const auto& doctor : doctors.asArray()) {
            doctors_.append(doctor);
        }
    }
    
    endResetModel();
}

inline void DoctorModel::clear() {
    beginResetModel();
    doctors_.clear();
    endResetModel();
}

inline bool DoctorModel::isCurrentDoctorAvailable() const {
    for (const auto& doctor : doctors_) {
        if (doctor["doctor_id"].asInt() == current_doctor_id_) {
            return doctor["is_available"].asBool();
        }
    }
    return false;
}

} // namespace client
} // namespace smartsched
