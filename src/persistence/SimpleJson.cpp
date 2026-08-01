#include "proteus/persistence/SimpleJson.hpp"

#include <charconv>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace proteus {
namespace {

const Json kNull;

std::string escapeString(const std::string& value) {
    std::ostringstream out;
    out << '"';
    for (const auto character : value) {
        switch (character) {
        case '"': out << "\\\""; break;
        case '\\': out << "\\\\"; break;
        case '\b': out << "\\b"; break;
        case '\f': out << "\\f"; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default:
            if (static_cast<unsigned char>(character) < 0x20U) {
                out << "\\u" << std::hex << std::setw(4)
                    << std::setfill('0')
                    << static_cast<int>(
                           static_cast<unsigned char>(character))
                    << std::dec;
            } else {
                out << character;
            }
        }
    }
    out << '"';
    return out.str();
}

void appendUtf8(std::string& target, unsigned codePoint) {
    if (codePoint <= 0x7FU) {
        target.push_back(static_cast<char>(codePoint));
    } else if (codePoint <= 0x7FFU) {
        target.push_back(static_cast<char>(0xC0U | (codePoint >> 6U)));
        target.push_back(static_cast<char>(0x80U | (codePoint & 0x3FU)));
    } else {
        target.push_back(static_cast<char>(0xE0U | (codePoint >> 12U)));
        target.push_back(
            static_cast<char>(0x80U | ((codePoint >> 6U) & 0x3FU)));
        target.push_back(static_cast<char>(0x80U | (codePoint & 0x3FU)));
    }
}

class Parser {
public:
    explicit Parser(std::string_view text) : text_(text) {
    }

    Json parseDocument() {
        skipSpace();
        auto value = parseValue();
        skipSpace();
        if (position_ != text_.size()) {
            fail("Unexpected characters after JSON value");
        }
        return value;
    }

private:
    [[noreturn]] void fail(const std::string& message) const {
        throw std::runtime_error(
            "JSON parse error at byte " + std::to_string(position_)
            + ": " + message + ".");
    }

    void skipSpace() {
        while (position_ < text_.size()) {
            const auto value = text_[position_];
            if (value != ' ' && value != '\t' && value != '\r'
                && value != '\n') {
                break;
            }
            ++position_;
        }
    }

    bool consume(char expected) {
        skipSpace();
        if (position_ < text_.size() && text_[position_] == expected) {
            ++position_;
            return true;
        }
        return false;
    }

    Json parseValue() {
        skipSpace();
        if (position_ >= text_.size()) {
            fail("Unexpected end of input");
        }
        const auto marker = text_[position_];
        if (marker == '{') return parseObject();
        if (marker == '[') return parseArray();
        if (marker == '"') return Json(parseString());
        if (marker == 't') return parseLiteral("true", Json(true));
        if (marker == 'f') return parseLiteral("false", Json(false));
        if (marker == 'n') return parseLiteral("null", Json());
        if (marker == '-' || (marker >= '0' && marker <= '9')) {
            return parseNumber();
        }
        fail("Unknown value");
    }

    Json parseLiteral(std::string_view literal, Json value) {
        if (text_.substr(position_, literal.size()) != literal) {
            fail("Invalid literal");
        }
        position_ += literal.size();
        return value;
    }

    Json parseObject() {
        consume('{');
        Json::Object object;
        skipSpace();
        if (consume('}')) return Json(std::move(object));
        for (;;) {
            skipSpace();
            if (position_ >= text_.size() || text_[position_] != '"') {
                fail("Object key must be a string");
            }
            const auto key = parseString();
            if (!consume(':')) fail("Missing ':' after object key");
            object.insert_or_assign(key, parseValue());
            if (consume('}')) break;
            if (!consume(',')) fail("Missing ',' between object entries");
        }
        return Json(std::move(object));
    }

    Json parseArray() {
        consume('[');
        Json::Array array;
        skipSpace();
        if (consume(']')) return Json(std::move(array));
        for (;;) {
            array.push_back(parseValue());
            if (consume(']')) break;
            if (!consume(',')) fail("Missing ',' between array items");
        }
        return Json(std::move(array));
    }

    static unsigned hexDigit(char value) {
        if (value >= '0' && value <= '9') {
            return static_cast<unsigned>(value - '0');
        }
        if (value >= 'a' && value <= 'f') {
            return static_cast<unsigned>(value - 'a' + 10);
        }
        if (value >= 'A' && value <= 'F') {
            return static_cast<unsigned>(value - 'A' + 10);
        }
        return 16U;
    }

    std::string parseString() {
        if (!consume('"')) fail("Missing opening quote");
        std::string result;
        while (position_ < text_.size()) {
            const auto value = text_[position_++];
            if (value == '"') return result;
            if (value != '\\') {
                result.push_back(value);
                continue;
            }
            if (position_ >= text_.size()) fail("Incomplete escape");
            const auto escaped = text_[position_++];
            switch (escaped) {
            case '"': result.push_back('"'); break;
            case '\\': result.push_back('\\'); break;
            case '/': result.push_back('/'); break;
            case 'b': result.push_back('\b'); break;
            case 'f': result.push_back('\f'); break;
            case 'n': result.push_back('\n'); break;
            case 'r': result.push_back('\r'); break;
            case 't': result.push_back('\t'); break;
            case 'u': {
                if (position_ + 4 > text_.size()) {
                    fail("Incomplete Unicode escape");
                }
                unsigned codePoint = 0;
                for (int index = 0; index < 4; ++index) {
                    const auto digit = hexDigit(text_[position_++]);
                    if (digit > 15U) fail("Invalid Unicode escape");
                    codePoint = codePoint * 16U + digit;
                }
                appendUtf8(result, codePoint);
                break;
            }
            default: fail("Invalid escape sequence");
            }
        }
        fail("Unterminated string");
    }

    Json parseNumber() {
        const auto start = position_;
        if (text_[position_] == '-') ++position_;
        if (position_ >= text_.size()) fail("Incomplete number");
        if (text_[position_] == '0') {
            ++position_;
        } else {
            while (position_ < text_.size()
                   && text_[position_] >= '0' && text_[position_] <= '9') {
                ++position_;
            }
        }
        if (position_ < text_.size() && text_[position_] == '.') {
            ++position_;
            while (position_ < text_.size()
                   && text_[position_] >= '0' && text_[position_] <= '9') {
                ++position_;
            }
        }
        if (position_ < text_.size()
            && (text_[position_] == 'e' || text_[position_] == 'E')) {
            ++position_;
            if (position_ < text_.size()
                && (text_[position_] == '+' || text_[position_] == '-')) {
                ++position_;
            }
            while (position_ < text_.size()
                   && text_[position_] >= '0' && text_[position_] <= '9') {
                ++position_;
            }
        }
        const auto token = std::string(text_.substr(start, position_ - start));
        char* end = nullptr;
        const auto number = std::strtod(token.c_str(), &end);
        if (!end || *end != '\0' || !std::isfinite(number)) {
            fail("Invalid number");
        }
        return Json(number);
    }

    std::string_view text_;
    std::size_t position_ = 0;
};

void dumpValue(const Json& value, std::ostringstream& out, int indent,
               int depth) {
    const auto lineIndent = [&out, indent](int level) {
        if (indent > 0) out << std::string(level * indent, ' ');
    };
    if (value.isNull()) {
        out << "null";
    } else if (value.isBool()) {
        out << (value.asBool() ? "true" : "false");
    } else if (value.isNumber()) {
        out << std::setprecision(17) << value.asNumber();
    } else if (value.isString()) {
        out << escapeString(value.asString());
    } else if (value.isArray()) {
        const auto& array = value.asArray();
        out << '[';
        for (std::size_t index = 0; index < array.size(); ++index) {
            if (index > 0) out << ',';
            if (indent > 0) {
                out << '\n';
                lineIndent(depth + 1);
            }
            dumpValue(array[index], out, indent, depth + 1);
        }
        if (!array.empty() && indent > 0) {
            out << '\n';
            lineIndent(depth);
        }
        out << ']';
    } else {
        const auto& object = value.asObject();
        out << '{';
        std::size_t index = 0;
        for (const auto& [key, child] : object) {
            if (index++ > 0) out << ',';
            if (indent > 0) {
                out << '\n';
                lineIndent(depth + 1);
            }
            out << escapeString(key) << (indent > 0 ? ": " : ":");
            dumpValue(child, out, indent, depth + 1);
        }
        if (!object.empty() && indent > 0) {
            out << '\n';
            lineIndent(depth);
        }
        out << '}';
    }
}

} // namespace

Json::Json() : data_(nullptr) {
}
Json::Json(std::nullptr_t) : data_(nullptr) {
}
Json::Json(bool value) : data_(value) {
}
Json::Json(int value) : data_(static_cast<double>(value)) {
}
Json::Json(double value) : data_(value) {
}
Json::Json(const char* value) : data_(std::string(value ? value : "")) {
}
Json::Json(std::string value) : data_(std::move(value)) {
}
Json::Json(Array value) : data_(std::move(value)) {
}
Json::Json(Object value) : data_(std::move(value)) {
}

bool Json::isNull() const {
    return std::holds_alternative<std::nullptr_t>(data_);
}
bool Json::isBool() const {
    return std::holds_alternative<bool>(data_);
}
bool Json::isNumber() const {
    return std::holds_alternative<double>(data_);
}
bool Json::isString() const {
    return std::holds_alternative<std::string>(data_);
}
bool Json::isArray() const {
    return std::holds_alternative<Array>(data_);
}
bool Json::isObject() const {
    return std::holds_alternative<Object>(data_);
}

bool Json::asBool(bool fallback) const {
    const auto* value = std::get_if<bool>(&data_);
    return value ? *value : fallback;
}
int Json::asInt(int fallback) const {
    const auto* value = std::get_if<double>(&data_);
    return value ? static_cast<int>(*value) : fallback;
}
double Json::asNumber(double fallback) const {
    const auto* value = std::get_if<double>(&data_);
    return value ? *value : fallback;
}
std::string Json::asString(std::string fallback) const {
    const auto* value = std::get_if<std::string>(&data_);
    return value ? *value : std::move(fallback);
}

const Json::Array& Json::asArray() const {
    const auto* value = std::get_if<Array>(&data_);
    if (!value) throw std::runtime_error("JSON value is not an array.");
    return *value;
}
Json::Array& Json::asArray() {
    auto* value = std::get_if<Array>(&data_);
    if (!value) throw std::runtime_error("JSON value is not an array.");
    return *value;
}
const Json::Object& Json::asObject() const {
    const auto* value = std::get_if<Object>(&data_);
    if (!value) throw std::runtime_error("JSON value is not an object.");
    return *value;
}
Json::Object& Json::asObject() {
    auto* value = std::get_if<Object>(&data_);
    if (!value) throw std::runtime_error("JSON value is not an object.");
    return *value;
}

Json& Json::operator[](const std::string& key) {
    if (!isObject()) data_ = Object{};
    return std::get<Object>(data_)[key];
}
const Json& Json::operator[](const std::string& key) const {
    const auto* object = std::get_if<Object>(&data_);
    if (!object) return kNull;
    const auto found = object->find(key);
    return found == object->end() ? kNull : found->second;
}
bool Json::contains(const std::string& key) const {
    const auto* object = std::get_if<Object>(&data_);
    return object && object->contains(key);
}
void Json::pushBack(Json value) {
    if (!isArray()) data_ = Array{};
    std::get<Array>(data_).push_back(std::move(value));
}
std::string Json::dump(int indent) const {
    std::ostringstream output;
    dumpValue(*this, output, indent, 0);
    return output.str();
}
Json Json::parse(std::string_view text) {
    return Parser(text).parseDocument();
}

} // namespace proteus
