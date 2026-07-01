/**
 * @file csv_exporter.h
 * @brief CSV导出器
 * 
 * 医院视角: 支持数据导出用于二次分析
 * 管理视角: 符合医疗数据导出规范
 */

#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <memory>

namespace smartsched {
namespace utils {

// =============================================================================
// CSV导出器
// =============================================================================
class CsvExporter {
public:
    CsvExporter();
    ~CsvExporter();
    
    // =============================================================================
    // 配置
    // =============================================================================
    
    void setDelimiter(char delimiter) { delimiter_ = delimiter; }
    void setEncoding(const std::string& encoding) { encoding_ = encoding; }
    void setIncludeHeader(bool include) { include_header_ = include; }
    
    // =============================================================================
    // 导出方法
    // =============================================================================
    
    /**
     * @brief 导出到文件
     * @param filepath 文件路径
     * @param headers 表头
     * @param rows 数据行
     * @return 是否成功
     */
    bool exportToFile(
        const std::string& filepath,
        const std::vector<std::string>& headers,
        const std::vector<std::vector<std::string>>& rows
    );
    
    /**
     * @brief 导出到字符串
     * @param headers 表头
     * @param rows 数据行
     * @return CSV字符串
     */
    std::string exportToString(
        const std::vector<std::string>& headers,
        const std::vector<std::vector<std::string>>& rows
    );
    
    /**
     * @brief 追加到文件
     * @param filepath 文件路径
     * @param rows 数据行
     * @return 是否成功
     */
    bool appendToFile(
        const std::string& filepath,
        const std::vector<std::vector<std::string>>& rows
    );
    
    // =============================================================================
    // 便捷方法
    // =============================================================================
    
    /**
     * @brief 导出统计数据
     */
    bool exportDailyStatistics(
        const std::string& filepath,
        const std::string& date,
        const struct DailyStats& stats
    );
    
    /**
     * @brief 导出医生排名
     */
    bool exportDoctorRankings(
        const std::string& filepath,
        const std::vector<struct DoctorRankEntry>& rankings
    );
    
    /**
     * @brief 导出患者列表
     */
    bool exportPatientList(
        const std::string& filepath,
        const std::vector<struct PatientEntry>& patients
    );

private:
    std::string escapeField(const std::string& field);
    std::string joinLine(const std::vector<std::string>& fields);
    
    char delimiter_;
    std::string encoding_;
    bool include_header_;
    
    // 辅助结构
public:
    struct DailyStats {
        std::string date;
        int total_registrations;
        int total_consultations;
        int avg_wait_time;
        int peak_queue;
    };
    
    struct DoctorRankEntry {
        int rank;
        int doctor_id;
        std::string name;
        std::string title;
        int consultation_count;
        double avg_time;
    };
    
    struct PatientEntry {
        int patient_id;
        std::string name;
        int age;
        std::string gender;
        std::string phone;
        int visit_count;
        std::string last_visit;
    };
};

// =============================================================================
// Excel导出器（简化版CSV格式）
// =============================================================================
class ExcelExporter {
public:
    ExcelExporter();
    ~ExcelExporter();
    
    /**
     * @brief 导出为XLSX格式（使用简单XML格式）
     * @param filepath 文件路径
     * @param sheet_name 工作表名称
     * @param headers 表头
     * @param rows 数据行
     * @return 是否成功
     */
    bool exportToXlsx(
        const std::string& filepath,
        const std::string& sheet_name,
        const std::vector<std::string>& headers,
        const std::vector<std::vector<std::string>>& rows
    );
    
    /**
     * @brief 导出为HTML表格（可用Excel打开）
     * @param filepath 文件路径
     * @param title 表格标题
     * @param headers 表头
     * @param rows 数据行
     * @return 是否成功
     */
    bool exportToHtml(
        const std::string& filepath,
        const std::string& title,
        const std::vector<std::string>& headers,
        const std::vector<std::vector<std::string>>& rows
    );

private:
    std::string escapeXml(const std::string& str);
};

// =============================================================================
// 内联实现
// =============================================================================

inline CsvExporter::CsvExporter() 
    : delimiter_(','), encoding_("UTF-8"), include_header_(true) {}

inline CsvExporter::~CsvExporter() {}

inline std::string CsvExporter::escapeField(const std::string& field) {
    std::string result;
    bool needs_quotes = false;
    
    // 检查是否需要引号
    for (char c : field) {
        if (c == '"' || c == delimiter_ || c == '\n' || c == '\r') {
            needs_quotes = true;
            break;
        }
    }
    
    if (needs_quotes) {
        result = "\"";
        for (char c : field) {
            if (c == '"') result += "\"\"";
            else result += c;
        }
        result += "\"";
    } else {
        result = field;
    }
    
    return result;
}

inline std::string CsvExporter::joinLine(const std::vector<std::string>& fields) {
    std::ostringstream oss;
    for (size_t i = 0; i < fields.size(); ++i) {
        if (i > 0) oss << delimiter_;
        oss << escapeField(fields[i]);
    }
    oss << "\n";
    return oss.str();
}

inline std::string CsvExporter::exportToString(
    const std::vector<std::string>& headers,
    const std::vector<std::vector<std::string>>& rows
) {
    std::ostringstream oss;
    
    // BOM标记（UTF-8）
    oss << "\xEF\xBB\xBF";
    
    // 表头
    if (include_header_ && !headers.empty()) {
        oss << joinLine(headers);
    }
    
    // 数据行
    for (const auto& row : rows) {
        oss << joinLine(row);
    }
    
    return oss.str();
}

inline bool CsvExporter::exportToFile(
    const std::string& filepath,
    const std::vector<std::string>& headers,
    const std::vector<std::vector<std::string>>& rows
) {
    std::ofstream file(filepath, std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    std::string content = exportToString(headers, rows);
    file.write(content.data(), content.size());
    file.close();
    
    return true;
}

inline bool CsvExporter::appendToFile(
    const std::string& filepath,
    const std::vector<std::vector<std::string>>& rows
) {
    std::ofstream file(filepath, std::ios::app | std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    for (const auto& row : rows) {
        file << joinLine(row);
    }
    
    file.close();
    return true;
}

// =============================================================================
// Excel导出器实现
// =============================================================================

inline ExcelExporter::ExcelExporter() {}
inline ExcelExporter::~ExcelExporter() {}

inline std::string ExcelExporter::escapeXml(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '<': result += "&lt;"; break;
            case '>': result += "&gt;"; break;
            case '&': result += "&amp;"; break;
            case '"': result += "&quot;"; break;
            default: result += c; break;
        }
    }
    return result;
}

inline bool ExcelExporter::exportToHtml(
    const std::string& filepath,
    const std::string& title,
    const std::vector<std::string>& headers,
    const std::vector<std::vector<std::string>>& rows
) {
    std::ofstream file(filepath, std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    file << "<!DOCTYPE html>\n";
    file << "<html>\n<head>\n";
    file << "<meta charset=\"UTF-8\">\n";
    file << "<title>" << escapeXml(title) << "</title>\n";
    file << "<style>\n";
    file << "table { border-collapse: collapse; width: 100%; }\n";
    file << "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }\n";
    file << "th { background-color: #1976D2; color: white; }\n";
    file << "tr:nth-child(even) { background-color: #f2f2f2; }\n";
    file << "h1 { color: #1976D2; }\n";
    file << ".info { color: #666; margin-bottom: 20px; }\n";
    file << "</style>\n";
    file << "</head>\n<body>\n";
    
    file << "<h1>" << escapeXml(title) << "</h1>\n";
    file << "<p class=\"info\">导出时间: " << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss").toStdString() << "</p>\n";
    
    file << "<table>\n";
    
    // 表头
    file << "<tr>\n";
    for (const auto& h : headers) {
        file << "<th>" << escapeXml(h) << "</th>\n";
    }
    file << "</tr>\n";
    
    // 数据行
    for (const auto& row : rows) {
        file << "<tr>\n";
        for (const auto& cell : row) {
            file << "<td>" << escapeXml(cell) << "</td>\n";
        }
        file << "</tr>\n";
    }
    
    file << "</table>\n</body>\n</html>";
    
    file.close();
    return true;
}

} // namespace utils
} // namespace smartsched
