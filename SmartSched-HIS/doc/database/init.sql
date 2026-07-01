-- SmartSched-HIS 数据库初始化脚本
-- 版本: 1.0.0
-- 描述: 创建医院门诊智慧调度系统的所有表结构和初始数据

-- 创建数据库
CREATE DATABASE IF NOT EXISTS smartsched 
    CHARACTER SET utf8mb4 
    COLLATE utf8mb4_unicode_ci;

USE smartsched;

-- =============================================================================
-- 1. 科室表 (departments)
-- =============================================================================
DROP TABLE IF EXISTS departments;
CREATE TABLE departments (
    dept_id INT AUTO_INCREMENT PRIMARY KEY COMMENT '科室ID',
    name VARCHAR(100) NOT NULL COMMENT '科室名称',
    description TEXT COMMENT '科室描述',
    queue_capacity INT DEFAULT 50 COMMENT '最大排队人数',
    is_active BOOLEAN DEFAULT TRUE COMMENT '是否启用',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    INDEX idx_name (name),
    INDEX idx_active (is_active)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='科室表';

-- =============================================================================
-- 2. 医生表 (doctors)
-- =============================================================================
DROP TABLE IF EXISTS doctors;
CREATE TABLE doctors (
    doctor_id INT AUTO_INCREMENT PRIMARY KEY COMMENT '医生ID',
    dept_id INT NOT NULL COMMENT '所属科室ID',
    name VARCHAR(50) NOT NULL COMMENT '医生姓名',
    title VARCHAR(50) COMMENT '职称(主任医师/副主任医师/主治医师)',
    specialty VARCHAR(200) COMMENT '专长',
    is_active BOOLEAN DEFAULT TRUE COMMENT '是否在职',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    FOREIGN KEY (dept_id) REFERENCES departments(dept_id) ON DELETE RESTRICT,
    INDEX idx_dept (dept_id),
    INDEX idx_name (name),
    INDEX idx_active (is_active)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='医生表';

-- =============================================================================
-- 3. 患者表 (patients)
-- =============================================================================
DROP TABLE IF EXISTS patients;
CREATE TABLE patients (
    patient_id INT AUTO_INCREMENT PRIMARY KEY COMMENT '患者ID',
    name VARCHAR(50) NOT NULL COMMENT '患者姓名',
    age INT COMMENT '年龄',
    gender VARCHAR(10) COMMENT '性别',
    phone VARCHAR(20) COMMENT '联系电话',
    id_card VARCHAR(20) COMMENT '身份证号',
    medical_card_no VARCHAR(30) COMMENT '就诊卡号',
    allergy_history TEXT COMMENT '过敏史',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    INDEX idx_name (name),
    INDEX idx_phone (phone),
    INDEX idx_idcard (id_card)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='患者表';

-- =============================================================================
-- 4. 排队队列表 (queue)
-- =============================================================================
DROP TABLE IF EXISTS queue;
CREATE TABLE queue (
    queue_id INT AUTO_INCREMENT PRIMARY KEY COMMENT '排队ID',
    patient_id INT NOT NULL COMMENT '患者ID',
    dept_id INT NOT NULL COMMENT '科室ID',
    doctor_id INT COMMENT '指定医生ID(可为空)',
    queue_number VARCHAR(20) NOT NULL COMMENT '排队号码',
    position INT NOT NULL COMMENT '当前排队位置',
    join_time BIGINT NOT NULL COMMENT '加入排队时间戳',
    estimated_start_time BIGINT COMMENT '预计开始时间戳',
    status TINYINT DEFAULT 0 COMMENT '状态: 0=等待, 1=就诊中, 2=已完成, 3=已取消',
    call_count INT DEFAULT 0 COMMENT '被叫次数',
    last_call_time BIGINT COMMENT '最后呼叫时间',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    FOREIGN KEY (patient_id) REFERENCES patients(patient_id) ON DELETE CASCADE,
    FOREIGN KEY (dept_id) REFERENCES departments(dept_id) ON DELETE RESTRICT,
    INDEX idx_dept_status (dept_id, status),
    INDEX idx_patient (patient_id),
    INDEX idx_queue_number (queue_number),
    INDEX idx_join_time (join_time)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='排队队列表';

-- =============================================================================
-- 5. 就诊记录表 (records)
-- =============================================================================
DROP TABLE IF EXISTS records;
CREATE TABLE records (
    record_id INT AUTO_INCREMENT PRIMARY KEY COMMENT '记录ID',
    patient_id INT NOT NULL COMMENT '患者ID',
    doctor_id INT NOT NULL COMMENT '医生ID',
    queue_id INT COMMENT '对应的排队ID',
    start_time BIGINT NOT NULL COMMENT '开始时间戳',
    end_time BIGINT COMMENT '结束时间戳',
    duration INT COMMENT '就诊时长(秒)',
    diagnosis TEXT COMMENT '诊断',
    prescription TEXT COMMENT '处方',
    need_ultrasound BOOLEAN DEFAULT FALSE COMMENT '是否需要B超',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    FOREIGN KEY (patient_id) REFERENCES patients(patient_id) ON DELETE CASCADE,
    FOREIGN KEY (doctor_id) REFERENCES doctors(doctor_id) ON DELETE RESTRICT,
    FOREIGN KEY (queue_id) REFERENCES queue(queue_id) ON DELETE SET NULL,
    INDEX idx_patient (patient_id),
    INDEX idx_doctor (doctor_id),
    INDEX idx_start_time (start_time)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='就诊记录表';

-- =============================================================================
-- 6. B超记录表 (ultrasound)
-- =============================================================================
DROP TABLE IF EXISTS ultrasound;
CREATE TABLE ultrasound (
    appt_id INT AUTO_INCREMENT PRIMARY KEY COMMENT '预约ID',
    patient_id INT NOT NULL COMMENT '患者ID',
    record_id INT COMMENT '对应的就诊记录ID',
    machine_id INT NOT NULL COMMENT 'B超机器ID(1/2/3)',
    appointment_time BIGINT COMMENT '预约时间戳',
    start_time BIGINT COMMENT '开始时间戳',
    end_time BIGINT COMMENT '结束时间戳',
    result TEXT COMMENT '检查结果',
    status TINYINT DEFAULT 0 COMMENT '状态: 0=预约, 1=进行中, 2=已完成, 3=已取消',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    FOREIGN KEY (patient_id) REFERENCES patients(patient_id) ON DELETE CASCADE,
    FOREIGN KEY (record_id) REFERENCES records(record_id) ON DELETE SET NULL,
    INDEX idx_patient (patient_id),
    INDEX idx_machine_status (machine_id, status),
    INDEX idx_appointment_time (appointment_time)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='B超记录表';

-- =============================================================================
-- 7. 系统配置表 (config)
-- =============================================================================
DROP TABLE IF EXISTS config;
CREATE TABLE config (
    config_id INT AUTO_INCREMENT PRIMARY KEY COMMENT '配置ID',
    config_key VARCHAR(100) NOT NULL UNIQUE COMMENT '配置键',
    config_value TEXT COMMENT '配置值',
    description VARCHAR(200) COMMENT '配置描述',
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    INDEX idx_key (config_key)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='系统配置表';

-- =============================================================================
-- 8. 操作日志表 (operation_log)
-- =============================================================================
DROP TABLE IF EXISTS operation_log;
CREATE TABLE operation_log (
    log_id BIGINT AUTO_INCREMENT PRIMARY KEY COMMENT '日志ID',
    user_type VARCHAR(20) COMMENT '用户类型(patient/doctor/admin)',
    user_id INT COMMENT '用户ID',
    action VARCHAR(50) NOT NULL COMMENT '操作类型',
    target_type VARCHAR(50) COMMENT '操作对象类型',
    target_id INT COMMENT '操作对象ID',
    details JSON COMMENT '操作详情',
    ip_address VARCHAR(45) COMMENT 'IP地址',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    INDEX idx_user (user_type, user_id),
    INDEX idx_action (action),
    INDEX idx_created_at (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='操作日志表';

-- =============================================================================
-- 9. 统计报表表 (statistics)
-- =============================================================================
DROP TABLE IF EXISTS statistics;
CREATE TABLE statistics (
    stat_id BIGINT AUTO_INCREMENT PRIMARY KEY COMMENT '统计ID',
    stat_date DATE NOT NULL COMMENT '统计日期',
    dept_id INT COMMENT '科室ID',
    total_registrations INT DEFAULT 0 COMMENT '总挂号数',
    total_consultations INT DEFAULT 0 COMMENT '总就诊数',
    avg_wait_time INT COMMENT '平均等待时间(秒)',
    avg_consult_time INT COMMENT '平均就诊时间(秒)',
    peak_queue_size INT COMMENT '峰值排队人数',
    ultrasound_count INT DEFAULT 0 COMMENT 'B超检查数',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    INDEX idx_date (stat_date),
    INDEX idx_dept (dept_id),
    UNIQUE KEY uk_date_dept (stat_date, dept_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='统计报表表';

-- =============================================================================
-- 初始数据
-- =============================================================================

-- 插入科室数据（15个科室）
INSERT INTO departments (name, description, queue_capacity) VALUES
('内科', 'Internal Medicine - 诊治内科常见疾病', 50),
('外科', 'General Surgery - 普通外科疾病', 40),
('儿科', 'Pediatrics - 儿童疾病诊治', 60),
('妇科', 'Gynecology - 妇科疾病诊治', 45),
('骨科', 'Orthopedics - 骨科疾病诊治', 35),
('神经内科', 'Neurology - 神经系统疾病', 30),
('神经外科', 'Neurosurgery - 神经外科手术', 20),
('心血管内科', 'Cardiology - 心脏血管疾病', 35),
('呼吸内科', 'Pulmonology - 呼吸系统疾病', 40),
('消化内科', 'Gastroenterology - 消化系统疾病', 40),
('眼科', 'Ophthalmology - 眼科疾病诊治', 50),
('耳鼻喉科', 'ENT - 耳鼻喉疾病', 45),
('皮肤科', 'Dermatology - 皮肤疾病诊治', 50),
('口腔科', 'Stomatology - 口腔疾病诊治', 40),
('中医科', 'Traditional Chinese Medicine - 中医诊疗', 35);

-- 插入医生数据（每个科室3名医生）
INSERT INTO doctors (dept_id, name, title, specialty) VALUES
-- 内科 3名医生
(1, '张明华', '主任医师', '心血管疾病、老年病'),
(1, '李秀英', '副主任医师', '消化系统疾病'),
(1, '王建国', '主治医师', '呼吸系统疾病'),
-- 外科 3名医生
(2, '赵伟东', '主任医师', '微创手术、腹腔镜'),
(2, '孙丽娟', '副主任医师', '甲状腺乳腺疾病'),
(2, '周海涛', '主治医师', '普外科常见疾病'),
-- 儿科 3名医生
(3, '吴静怡', '主任医师', '新生儿疾病'),
(3, '郑晓峰', '副主任医师', '儿童呼吸系统'),
(3, '陈美玲', '主治医师', '儿童消化系统'),
-- 妇科 3名医生
(4, '刘芳华', '主任医师', '妇科肿瘤'),
(4, '黄玉珍', '副主任医师', '产科高危妊娠'),
(4, '林秀云', '主治医师', '妇科内分泌'),
-- 骨科 3名医生
(5, '杨志强', '主任医师', '脊柱外科'),
(5, '徐海波', '副主任医师', '关节外科'),
(5, '马晓东', '主治医师', '运动损伤'),
-- 神经内科 3名医生
(6, '胡文杰', '主任医师', '脑血管疾病'),
(6, '朱晓燕', '副主任医师', '癫痫治疗'),
(6, '许志刚', '主治医师', '头痛眩晕'),
-- 神经外科 3名医生
(7, '高建新', '主任医师', '脑肿瘤手术'),
(7, '曹艳红', '副主任医师', '脑血管介入'),
(7, '冯志远', '主治医师', '颅脑外伤'),
-- 心血管内科 3名医生
(8, '何建华', '主任医师', '冠心病介入'),
(8, '田秀芳', '副主任医师', '心律失常'),
(8, '梁文博', '主治医师', '高血压病'),
-- 呼吸内科 3名医生
(9, '丁玉梅', '主任医师', '肺部感染'),
(9, '康志明', '副主任医师', '慢阻肺'),
(9, '宋晓华', '主治医师', '哮喘'),
-- 消化内科 3名医生
(10, '杜建华', '主任医师', '胃肠镜检查'),
(10, '贺美丽', '副主任医师', '肝胆疾病'),
(10, '钱志涛', '主治医师', '炎症性肠病'),
-- 眼科 3名医生
(11, '汤建国', '主任医师', '白内障手术'),
(11, '姜秀兰', '副主任医师', '眼底病'),
(11, '龚志峰', '主治医师', '青光眼'),
-- 耳鼻喉科 3名医生
(12, '万玉珍', '主任医师', '鼻窦炎治疗'),
(12, '谢明华', '副主任医师', '中耳炎治疗'),
(12, '卢秀英', '主治医师', '咽喉疾病'),
-- 皮肤科 3名医生
(13, '汪志强', '主任医师', '皮肤肿瘤'),
(13, '汤晓燕', '副主任医师', '过敏性疾病'),
(13, '贺志刚', '主治医师', '激光美容'),
-- 口腔科 3名医生
(14, '段秀芳', '主任医师', '口腔种植'),
(14, '侯玉珍', '副主任医师', '口腔正畸'),
(14, '雷秀英', '主治医师', '口腔修复'),
-- 中医科 3名医生
(15, '申屠华', '主任医师', '中医内科'),
(15, '欧阳明', '副主任医师', '针灸推拿'),
(15, '司马静', '主治医师', '中医妇科');

-- 插入测试患者数据
INSERT INTO patients (name, age, gender, phone, id_card, medical_card_no) VALUES
('张三', 45, '男', '13800138001', '110101197801011234', 'MC20240001'),
('李四', 32, '女', '13800138002', '110101199201021234', 'MC20240002'),
('王五', 58, '男', '13800138003', '110101196601031234', 'MC20240003'),
('赵六', 28, '女', '13800138004', '110101199601041234', 'MC20240004'),
('孙七', 65, '男', '13800138005', '110101195901051234', 'MC20240005'),
('周八', 42, '女', '13800138006', '110101198201061234', 'MC20240006'),
('吴九', 35, '男', '13800138007', '110101198901071234', 'MC20240007'),
('郑十', 50, '女', '13800138008', '110101197401081234', 'MC20240008');

-- 插入系统配置
INSERT INTO config (config_key, config_value, description) VALUES
('hospital_name', '智序医院', '医院名称'),
('avg_consult_time', '300', '平均就诊时间(秒)'),
('max_queue_size', '100', '单科室最大排队人数'),
('call_timeout', '120', '呼叫超时时间(秒)'),
('heartbeat_interval', '30', '心跳间隔(秒)'),
('server_port', '8888', '服务端端口'),
('queue_number_prefix', 'D', '排队号前缀');

-- =============================================================================
-- 视图定义
-- =============================================================================

-- 今日排队视图
CREATE OR REPLACE VIEW v_today_queue AS
SELECT 
    q.queue_id,
    q.queue_number,
    p.name AS patient_name,
    p.phone,
    d.name AS dept_name,
    doc.name AS doctor_name,
    q.position,
    q.status,
    q.join_time,
    CASE q.status
        WHEN 0 THEN '等待中'
        WHEN 1 THEN '就诊中'
        WHEN 2 THEN '已完成'
        WHEN 3 THEN '已取消'
    END AS status_text
FROM queue q
JOIN patients p ON q.patient_id = p.patient_id
JOIN departments d ON q.dept_id = d.dept_id
LEFT JOIN doctors doc ON q.doctor_id = doc.doctor_id
WHERE DATE(FROM_UNIXTIME(q.join_time/1000)) = CURDATE();

-- 医生今日接诊统计
CREATE OR REPLACE VIEW v_doctor_today_stats AS
SELECT 
    doc.doctor_id,
    doc.name AS doctor_name,
    d.name AS dept_name,
    COUNT(r.record_id) AS today_consultations,
    IFNULL(SUM(r.duration), 0) AS total_duration,
    IFNULL(AVG(r.duration), 0) AS avg_duration
FROM doctors doc
JOIN departments d ON doc.dept_id = d.dept_id
LEFT JOIN records r ON doc.doctor_id = r.doctor_id 
    AND DATE(FROM_UNIXTIME(r.start_time/1000)) = CURDATE()
WHERE doc.is_active = 1
GROUP BY doc.doctor_id;

-- =============================================================================
-- 存储过程
-- =============================================================================

-- 更新排队位置
DELIMITER //
CREATE PROCEDURE sp_update_queue_position(IN p_dept_id INT)
BEGIN
    SET @pos = 0;
    UPDATE queue 
    SET position = (@pos := @pos + 1)
    WHERE dept_id = p_dept_id 
      AND status = 0 
      AND DATE(FROM_UNIXTIME(join_time/1000)) = CURDATE()
    ORDER BY join_time;
END //
DELIMITER ;

-- 生成排队号码
DELIMITER //
CREATE PROCEDURE sp_generate_queue_number(IN p_dept_id INT, OUT p_queue_number VARCHAR(20))
BEGIN
    DECLARE v_date VARCHAR(8);
    DECLARE v_counter INT;
    
    SET v_date = DATE_FORMAT(CURDATE(), '%y%m%d');
    
    SELECT IFNULL(MAX(CAST(SUBSTRING(queue_number, 8) AS UNSIGNED)), 0) + 1
    INTO v_counter
    FROM queue
    WHERE dept_id = p_dept_id 
      AND DATE(FROM_UNIXTIME(join_time/1000)) = CURDATE();
    
    SET p_queue_number = CONCAT('D', LPAD(p_dept_id, 2, '0'), '-', v_date, LPAD(v_counter, 4, '0'));
END //
DELIMITER ;

-- =============================================================================
-- 完成提示
-- =============================================================================

SELECT '数据库初始化完成!' AS message;
SELECT CONCAT('共创建表: ', COUNT(*), ' 个') AS table_count 
FROM information_schema.tables 
WHERE table_schema = 'smartsched';
