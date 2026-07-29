#pragma once

#include <cstddef>
#include <functional>
#include <string>

namespace proteus {

enum class PinDirection {
    Input,
    Output,
    Bidirectional,
    Passive,
    Power
};

enum class SignalDomain {
    Analog,
    Digital,
    Mixed
};

struct PinDefinition {
    std::string id;
    std::string label;
    PinDirection direction = PinDirection::Passive;
    SignalDomain domain = SignalDomain::Mixed;
    bool required = true;
};

struct PinRef {
    std::string componentId;
    std::string pinId;

    [[nodiscard]] bool operator==(const PinRef& other) const noexcept {
        return componentId == other.componentId && pinId == other.pinId;
    }

    [[nodiscard]] bool operator!=(const PinRef& other) const noexcept {
        return !(*this == other);
    }

    [[nodiscard]] bool operator<(const PinRef& other) const noexcept {
        if (componentId != other.componentId) {
            return componentId < other.componentId;
        }
        return pinId < other.pinId;
    }
};

struct PinRefHash {
    [[nodiscard]] std::size_t operator()(const PinRef& ref) const noexcept {
        const auto h1 = std::hash<std::string>{}(ref.componentId);
        const auto h2 = std::hash<std::string>{}(ref.pinId);
        return h1 ^ (h2 << 1U);
    }
};

} // namespace proteus
