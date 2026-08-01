#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace proteus {

class Json {
public:
    using Array = std::vector<Json>;
    using Object = std::map<std::string, Json>;

    Json();
    Json(std::nullptr_t);
    Json(bool value);
    Json(int value);
    Json(double value);
    Json(const char* value);
    Json(std::string value);
    Json(Array value);
    Json(Object value);

    [[nodiscard]] bool isNull() const;
    [[nodiscard]] bool isBool() const;
    [[nodiscard]] bool isNumber() const;
    [[nodiscard]] bool isString() const;
    [[nodiscard]] bool isArray() const;
    [[nodiscard]] bool isObject() const;

    [[nodiscard]] bool asBool(bool fallback = false) const;
    [[nodiscard]] int asInt(int fallback = 0) const;
    [[nodiscard]] double asNumber(double fallback = 0.0) const;
    [[nodiscard]] std::string asString(
        std::string fallback = {}) const;

    [[nodiscard]] const Array& asArray() const;
    [[nodiscard]] Array& asArray();
    [[nodiscard]] const Object& asObject() const;
    [[nodiscard]] Object& asObject();

    Json& operator[](const std::string& key);
    [[nodiscard]] const Json& operator[](const std::string& key) const;
    [[nodiscard]] bool contains(const std::string& key) const;

    void pushBack(Json value);
    [[nodiscard]] std::string dump(int indent = 2) const;
    [[nodiscard]] static Json parse(std::string_view text);

private:
    using Storage = std::variant<std::nullptr_t, bool, double, std::string,
                                 Array, Object>;
    Storage data_;
};

} // namespace proteus
