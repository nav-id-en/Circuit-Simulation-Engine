#pragma once

#include <cmath>
#include <limits>
#include <string>

namespace proteus {

enum class LogicLevel {
    Low,
    High,
    Undefined
};

struct Signal {
    double voltage = std::numeric_limits<double>::quiet_NaN();
    LogicLevel logic = LogicLevel::Undefined;

    [[nodiscard]] static Signal low(double level = 0.0) {
        return {level, LogicLevel::Low};
    }

    [[nodiscard]] static Signal high(double level = 5.0) {
        return {level, LogicLevel::High};
    }

    [[nodiscard]] static Signal analog(double value,
                                       double lowMaximum = 1.5,
                                       double highMinimum = 3.5) {
        if (!std::isfinite(value)) {
            return undefined();
        }
        if (value <= lowMaximum) {
            return {value, LogicLevel::Low};
        }
        if (value >= highMinimum) {
            return {value, LogicLevel::High};
        }
        return {value, LogicLevel::Undefined};
    }

    [[nodiscard]] static Signal undefined() {
        return {};
    }

    [[nodiscard]] bool hasVoltage() const {
        return std::isfinite(voltage);
    }

    [[nodiscard]] bool approximatelyEquals(const Signal& other,
                                           double epsilon = 1.0e-9) const {
        if (logic != other.logic) {
            return false;
        }
        if (!hasVoltage() && !other.hasVoltage()) {
            return true;
        }
        return hasVoltage() && other.hasVoltage()
            && std::abs(voltage - other.voltage) <= epsilon;
    }
};

[[nodiscard]] inline std::string toString(LogicLevel level) {
    switch (level) {
    case LogicLevel::Low:
        return "LOW";
    case LogicLevel::High:
        return "HIGH";
    case LogicLevel::Undefined:
        return "UNDEFINED";
    }
    return "UNDEFINED";
}

} // namespace proteus
