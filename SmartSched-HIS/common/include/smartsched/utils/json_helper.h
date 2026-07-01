/**
 * @file json_helper.h
 * @brief JSON辅助工具
 * 
 * 网络视角: 便捷的JSON序列化/反序列化
 * 开发者视角: 简化协议消息处理
 */

#pragma once

#include "../common/compiler.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>

namespace smartsched {
namespace utils {

// =============================================================================
// JSON值类型
// =============================================================================
enum class JsonType {
    Null,
    Boolean,
    Number,
    String,
    Array,
    Object
};

// =============================================================================
// JSON值
// =============================================================================
class JsonValue {
public:
    JsonValue() : type_(JsonType::Null) {}
    
    explicit JsonValue(bool value) : type_(JsonType::Boolean) { bool_value_ = value; }
    explicit JsonValue(int value) : type_(JsonType::Number) { number_value_ = value; }
    explicit JsonValue(double value) : type_(JsonType::Number) { number_value_ = value; }
    explicit JsonValue(const std::string& value) : type_(JsonType::String) { string_value_ = value; }
    explicit JsonValue(const char* value) : type_(JsonType::String) { string_value_ = value; }
    
    // 数组
    static JsonValue array() { return JsonValue(JsonType::Array); }
    static JsonValue object() { return JsonValue(JsonType::Object); }
    
    JsonValue(std::vector<JsonValue> arr) : type_(JsonType::Array), array_value_(std::move(arr)) {}
    JsonValue(std::map<std::string, JsonValue> obj) : type_(JsonType::Object), object_value_(std::move(obj)) {}
    
    // 访问器
    JsonType type() const { return type_; }
    
    bool isNull() const { return type_ == JsonType::Null; }
    bool isBool() const { return type_ == JsonType::Boolean; }
    bool isNumber() const { return type_ == JsonType::Number; }
    bool isString() const { return type_ == JsonType::String; }
    bool isArray() const { return type_ == JsonType::Array; }
    bool isObject() const { return type_ == JsonType::Object; }
    
    // 值访问
    bool asBool() const { return bool_value_; }
    int asInt() const { return static_cast<int>(number_value_); }
    double asDouble() const { return number_value_; }
    const std::string& asString() const { return string_value_; }
    const std::vector<JsonValue>& asArray() const { return array_value_; }
    const std::map<std::string, JsonValue>& asObject() const { return object_value_; }
    
    // 下标访问（数组）
    JsonValue& operator[](size_t index) {
        if (type_ != JsonType::Array) {
            type_ = JsonType::Array;
        }
        if (index >= array_value_.size()) {
            array_value_.resize(index + 1);
        }
        return array_value_[index];
    }
    
    const JsonValue& operator[](size_t index) const {
        static JsonValue null_value;
        if (type_ != JsonType::Array || index >= array_value_.size()) {
            return null_value;
        }
        return array_value_[index];
    }
    
    // 下标访问（对象）
    JsonValue& operator[](const std::string& key) {
        if (type_ != JsonType::Object) {
            type_ = JsonType::Object;
        }
        return object_value_[key];
    }
    
    const JsonValue& operator[](const std::string& key) const {
        static JsonValue null_value;
        if (type_ != JsonType::Object) return null_value;
        auto it = object_value_.find(key);
        if (it == object_value_.end()) return null_value;
        return it->second;
    }
    
    // 添加元素（数组）
    void add(const JsonValue& value) {
        if (type_ != JsonType::Array) {
            type_ = JsonType::Array;
        }
        array_value_.push_back(value);
    }
    
    // 获取大小
    size_t size() const {
        if (type_ == JsonType::Array) return array_value_.size();
        if (type_ == JsonType::Object) return object_value_.size();
        return 0;
    }
    
    bool empty() const { return size() == 0; }
    
    // 检查键存在
    bool has(const std::string& key) const {
        if (type_ != JsonType::Object) return false;
        return object_value_.find(key) != object_value_.end();
    }
    
    // 序列化
    std::string serialize(bool pretty = false) const {
        std::ostringstream oss;
        serializeTo(oss, 0, pretty);
        return oss.str();
    }
    
    // 静态解析
    static JsonValue parse(const std::string& json) {
        Parser parser(json);
        return parser.parse();
    }
    
private:
    explicit JsonValue(JsonType type) : type_(type) {}
    
    void serializeTo(std::ostringstream& oss, int indent, bool pretty) const {
        switch (type_) {
            case JsonType::Null:
                oss << "null";
                break;
            case JsonType::Boolean:
                oss << (bool_value_ ? "true" : "false");
                break;
            case JsonType::Number:
                oss << number_value_;
                break;
            case JsonType::String:
                oss << "\"" << escapeString(string_value_) << "\"";
                break;
            case JsonType::Array: {
                oss << "[";
                if (!array_value_.empty()) {
                    if (pretty) oss << "\n";
                    for (size_t i = 0; i < array_value_.size(); ++i) {
                        if (pretty) oss << std::string(indent + 2, ' ');
                        array_value_[i].serializeTo(oss, indent + 2, pretty);
                        if (i < array_value_.size() - 1) oss << ",";
                        if (pretty) oss << "\n";
                    }
                    if (pretty) oss << std::string(indent, ' ');
                }
                oss << "]";
                break;
            }
            case JsonType::Object: {
                oss << "{";
                bool first = true;
                if (!object_value_.empty()) {
                    if (pretty) oss << "\n";
                    for (const auto& [key, value] : object_value_) {
                        if (!first) {
                            oss << ",";
                            if (pretty) oss << "\n";
                        }
                        if (pretty) oss << std::string(indent + 2, ' ');
                        oss << "\"" << escapeString(key) << "\":";
                        value.serializeTo(oss, indent + 2, pretty);
                        first = false;
                    }
                    if (pretty) oss << "\n" << std::string(indent, ' ');
                }
                oss << "}";
                break;
            }
        }
    }
    
    static std::string escapeString(const std::string& s) {
        std::string result;
        for (char c : s) {
            switch (c) {
                case '"': result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\b': result += "\\b"; break;
                case '\f': result += "\\f"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default: result += c; break;
            }
        }
        return result;
    }
    
    // 简化解析器
    class Parser {
    public:
        explicit Parser(const std::string& json) : json_(json), pos_(0) {}
        
        JsonValue parse() {
            skipWhitespace();
            return parseValue();
        }
        
    private:
        void skipWhitespace() {
            while (pos_ < json_.size() && std::isspace(json_[pos_])) {
                ++pos_;
            }
        }
        
        JsonValue parseValue() {
            skipWhitespace();
            if (pos_ >= json_.size()) throw std::runtime_error("Unexpected end");
            
            char c = json_[pos_];
            if (c == '{') return parseObject();
            if (c == '[') return parseArray();
            if (c == '"') return parseString();
            if (c == 't' || c == 'f') return parseBool();
            if (c == 'n') return parseNull();
            if (c == '-' || std::isdigit(c)) return parseNumber();
            
            throw std::runtime_error("Invalid JSON at position " + std::to_string(pos_));
        }
        
        JsonValue parseObject() {
            std::map<std::string, JsonValue> obj;
            ++pos_; // skip {
            skipWhitespace();
            
            if (pos_ < json_.size() && json_[pos_] == '}') {
                ++pos_;
                return JsonValue(std::move(obj));
            }
            
            while (true) {
                skipWhitespace();
                if (json_[pos_] != '"') throw std::runtime_error("Expected key");
                
                auto key = parseString().asString();
                skipWhitespace();
                if (json_[pos_] != ':') throw std::runtime_error("Expected :");
                ++pos_;
                
                obj[key] = parseValue();
                skipWhitespace();
                
                if (json_[pos_] == '}') {
                    ++pos_;
                    break;
                }
                if (json_[pos_] != ',') throw std::runtime_error("Expected ,");
                ++pos_;
            }
            
            return JsonValue(std::move(obj));
        }
        
        JsonValue parseArray() {
            std::vector<JsonValue> arr;
            ++pos_; // skip [
            skipWhitespace();
            
            if (pos_ < json_.size() && json_[pos_] == ']') {
                ++pos_;
                return JsonValue(std::move(arr));
            }
            
            while (true) {
                arr.push_back(parseValue());
                skipWhitespace();
                
                if (json_[pos_] == ']') {
                    ++pos_;
                    break;
                }
                if (json_[pos_] != ',') throw std::runtime_error("Expected ,");
                ++pos_;
            }
            
            return JsonValue(std::move(arr));
        }
        
        JsonValue parseString() {
            ++pos_; // skip opening "
            std::string s;
            
            while (pos_ < json_.size() && json_[pos_] != '"') {
                if (json_[pos_] == '\\' && pos_ + 1 < json_.size()) {
                    ++pos_;
                    switch (json_[pos_]) {
                        case '"': s += '"'; break;
                        case '\\': s += '\\'; break;
                        case 'n': s += '\n'; break;
                        case 'r': s += '\r'; break;
                        case 't': s += '\t'; break;
                        default: s += json_[pos_]; break;
                    }
                } else {
                    s += json_[pos_];
                }
                ++pos_;
            }
            
            if (pos_ >= json_.size()) throw std::runtime_error("Unterminated string");
            ++pos_; // skip closing "
            
            return JsonValue(s);
        }
        
        JsonValue parseNumber() {
            size_t start = pos_;
            if (json_[pos_] == '-') ++pos_;
            
            while (pos_ < json_.size() && std::isdigit(json_[pos_])) ++pos_;
            if (pos_ < json_.size() && json_[pos_] == '.') ++pos_;
            while (pos_ < json_.size() && std::isdigit(json_[pos_])) ++pos_;
            
            std::string num_str = json_.substr(start, pos_ - start);
            double value = std::stod(num_str);
            return JsonValue(value);
        }
        
        JsonValue parseBool() {
            if (json_.substr(pos_, 4) == "true") {
                pos_ += 4;
                return JsonValue(true);
            }
            if (json_.substr(pos_, 5) == "false") {
                pos_ += 5;
                return JsonValue(false);
            }
            throw std::runtime_error("Invalid boolean");
        }
        
        JsonValue parseNull() {
            if (json_.substr(pos_, 4) == "null") {
                pos_ += 4;
                return JsonValue();
            }
            throw std::runtime_error("Invalid null");
        }
        
        const std::string& json_;
        size_t pos_;
    };
    
    JsonType type_;
    bool bool_value_ = false;
    double number_value_ = 0;
    std::string string_value_;
    std::vector<JsonValue> array_value_;
    std::map<std::string, JsonValue> object_value_;
};

// =============================================================================
// JSON构建器（简化创建复杂JSON）
// =============================================================================
class JsonBuilder {
public:
    JsonBuilder() : root_(JsonValue::object()) {}
    
    static JsonBuilder object() { return JsonBuilder(); }
    static JsonBuilder array() { return JsonBuilder(); }
    
    JsonBuilder& set(const std::string& key, bool value) {
        root_[key] = JsonValue(value);
        return *this;
    }
    
    JsonBuilder& set(const std::string& key, int value) {
        root_[key] = JsonValue(value);
        return *this;
    }
    
    JsonBuilder& set(const std::string& key, double value) {
        root_[key] = JsonValue(value);
        return *this;
    }
    
    JsonBuilder& set(const std::string& key, const std::string& value) {
        root_[key] = JsonValue(value);
        return *this;
    }
    
    JsonBuilder& set(const std::string& key, const char* value) {
        root_[key] = JsonValue(value);
        return *this;
    }
    
    JsonBuilder& set(const std::string& key, const JsonValue& value) {
        root_[key] = value;
        return *this;
    }
    
    JsonBuilder& push(const JsonValue& value) {
        root_.add(value);
        return *this;
    }
    
    JsonValue build() { return root_; }
    std::string serialize(bool pretty = false) { return root_.serialize(pretty); }

private:
    explicit JsonBuilder(JsonValue root) : root_(std::move(root)) {}
    
    JsonValue root_;
};

// =============================================================================
// 便捷函数
// =============================================================================
inline JsonValue parseJson(const std::string& json_str) {
    return JsonValue::parse(json_str);
}

inline std::string toJson(const JsonValue& value, bool pretty = false) {
    return value.serialize(pretty);
}

} // namespace utils
} // namespace smartsched
