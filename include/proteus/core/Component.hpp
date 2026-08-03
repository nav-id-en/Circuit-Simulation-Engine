#pragma once

#include "proteus/core/Pin.hpp"
#include "proteus/core/Signal.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace proteus {

using PropertyValue = std::variant<double, int, bool, std::string>;
using PropertyMap = std::map<std::string, PropertyValue>;

enum class PropertyKind {
    Number,
    Integer,
    Boolean,
    Text,
    FilePath,
    Color,
    Choice
};

struct PropertyDefinition {
    std::string key;
    std::string title;
    PropertyKind kind = PropertyKind::Text;
    PropertyValue defaultValue = std::string{};
    double minimum = 0.0;
    double maximum = 0.0;
    double step = 1.0;
    std::string unit;
    std::vector<std::string> choices;
    bool runtimeEditable = false;
};

struct Point {
    double x = 0.0;
    double y = 0.0;
};

struct DriveRequest {
    std::string pinId;
    Signal signal;
    double delaySeconds = 0.0;
};

struct ConductanceStamp {
    PinRef first;
    PinRef second;
    double siemens = 0.0;
};

struct CurrentStamp {
    PinRef from;
    PinRef to;
    double amperes = 0.0;
};

struct VoltageSourceStamp {
    PinRef positive;
    PinRef negative;
    double volts = 0.0;
    std::string branchKey;
};

class AnalogStampCollector {
public:
    virtual ~AnalogStampCollector() = default;
    virtual void addConductance(const ConductanceStamp& stamp) = 0;
    virtual void addCurrent(const CurrentStamp& stamp) = 0;
    virtual void addVoltageSource(const VoltageSourceStamp& stamp) = 0;
    virtual void markGround(const PinRef& pin) = 0;
};

class AnalogResultView {
public:
    virtual ~AnalogResultView() = default;
    [[nodiscard]] virtual double voltageAt(const PinRef& pin) const = 0;
    [[nodiscard]] virtual double branchCurrent(const std::string& branchKey) const = 0;
};

class SimulationView {
public:
    virtual ~SimulationView() = default;
    [[nodiscard]] virtual double timeSeconds() const = 0;
    [[nodiscard]] virtual double timeStepSeconds() const = 0;
    [[nodiscard]] virtual Signal signalAt(const PinRef& pin) const = 0;
    [[nodiscard]] virtual double voltageAt(const PinRef& pin) const = 0;
    virtual void logWarning(const std::string& source,
                            const std::string& message) const = 0;
};

class Component {
public:
    Component(std::string id, std::string label);
    virtual ~Component() = default;

    Component(const Component&) = delete;
    Component& operator=(const Component&) = delete;
    Component(Component&&) noexcept = default;
    Component& operator=(Component&&) noexcept = default;

    [[nodiscard]] virtual std::string typeId() const = 0;
    [[nodiscard]] virtual std::string displayName() const = 0;
    [[nodiscard]] virtual std::string category() const = 0;
    [[nodiscard]] virtual std::unique_ptr<Component> clone() const = 0;

    [[nodiscard]] const std::string& id() const noexcept;
    [[nodiscard]] const std::string& label() const noexcept;
    void setLabel(std::string label);

    [[nodiscard]] Point position() const noexcept;
    void setPosition(Point position) noexcept;
    [[nodiscard]] int rotationDegrees() const noexcept;
    void setRotationDegrees(int degrees) noexcept;
    [[nodiscard]] bool mirroredHorizontally() const noexcept;
    [[nodiscard]] bool mirroredVertically() const noexcept;
    void setMirroredHorizontally(bool mirrored) noexcept;
    void setMirroredVertically(bool mirrored) noexcept;

    [[nodiscard]] const std::vector<PinDefinition>& pins() const noexcept;
    [[nodiscard]] const PinDefinition* findPin(const std::string& pinId) const;
    [[nodiscard]] PinRef pinRef(const std::string& pinId) const;

    [[nodiscard]] const PropertyMap& properties() const noexcept;
    [[nodiscard]] const PropertyMap& runtimeState() const noexcept;
    [[nodiscard]] virtual std::vector<PropertyDefinition> propertyDefinitions() const;
    virtual bool setProperty(const std::string& key, PropertyValue value);
    void setRuntimeValue(const std::string& key, PropertyValue value);
    void replaceRuntimeState(PropertyMap state);

    template <typename T>
    [[nodiscard]] T property(const std::string& key, T fallback) const {
        const auto it = properties_.find(key);
        if (it == properties_.end()) {
            return fallback;
        }
        if (const auto* value = std::get_if<T>(&it->second)) {
            return *value;
        }
        return fallback;
    }

    template <typename T>
    [[nodiscard]] T runtimeValue(const std::string& key, T fallback) const {
        const auto it = runtimeState_.find(key);
        if (it == runtimeState_.end()) {
            return fallback;
        }
        if (const auto* value = std::get_if<T>(&it->second)) {
            return *value;
        }
        return fallback;
    }

    virtual void resetRuntime();
    virtual void stampAnalog(AnalogStampCollector& collector,
                             double timeStepSeconds);
    virtual void acceptAnalogResult(const AnalogResultView& result,
                                    double timeStepSeconds);
    [[nodiscard]] virtual std::vector<DriveRequest>
    evaluate(const SimulationView& view);

    [[nodiscard]] virtual std::optional<std::string> validationError() const;

protected:
    void addPin(PinDefinition pin);
    void defineProperty(PropertyDefinition definition);
    void copyCommonStateTo(Component& target) const;

private:
    std::string id_;
    std::string label_;
    Point position_;
    int rotationDegrees_ = 0;
    bool mirrorHorizontal_ = false;
    bool mirrorVertical_ = false;
    std::vector<PinDefinition> pins_;
    std::vector<PropertyDefinition> propertyDefinitions_;

protected:
    PropertyMap properties_;
    PropertyMap runtimeState_;
};

} // namespace proteus
