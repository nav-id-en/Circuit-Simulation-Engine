#include "proteus/components/Components.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace proteus {
namespace {

PropertyDefinition numberProperty(std::string key, std::string title,
                                  double value, double minimum,
                                  double maximum, double step,
                                  std::string unit = {},
                                  bool runtimeEditable = false) {
    return {std::move(key), std::move(title), PropertyKind::Number, value,
            minimum, maximum, step, std::move(unit), {}, runtimeEditable};
}

PropertyDefinition integerProperty(std::string key, std::string title,
                                   int value, int minimum, int maximum,
                                   bool runtimeEditable = false) {
    return {std::move(key), std::move(title), PropertyKind::Integer, value,
            static_cast<double>(minimum), static_cast<double>(maximum), 1.0,
            {}, {}, runtimeEditable};
}

PropertyDefinition booleanProperty(std::string key, std::string title,
                                   bool value, bool runtimeEditable = false) {
    return {std::move(key), std::move(title), PropertyKind::Boolean, value,
            0.0, 1.0, 1.0, {}, {}, runtimeEditable};
}

PropertyDefinition textProperty(std::string key, std::string title,
                                std::string value,
                                PropertyKind kind = PropertyKind::Text,
                                bool runtimeEditable = false) {
    return {std::move(key), std::move(title), kind, std::move(value),
            0.0, 0.0, 1.0, {}, {}, runtimeEditable};
}

PinDefinition pin(std::string id, std::string label, PinDirection direction,
                  SignalDomain domain = SignalDomain::Mixed,
                  bool required = true) {
    return {std::move(id), std::move(label), direction, domain, required};
}

double safeResistance(double value) {
    return std::clamp(std::abs(value), 1.0e-6, 1.0e15);
}

double safeTimeStep(double value) {
    return std::max(value, 1.0e-12);
}

Signal logicSignal(LogicLevel level, double highVoltage = 5.0) {
    if (level == LogicLevel::High) {
        return Signal::high(highVoltage);
    }
    if (level == LogicLevel::Low) {
        return Signal::low();
    }
    return Signal::undefined();
}

std::string gateTypeId(GateKind kind) {
    switch (kind) {
    case GateKind::And:
        return "and_gate";
    case GateKind::Or:
        return "or_gate";
    case GateKind::Not:
        return "not_gate";
    case GateKind::Xor:
        return "xor_gate";
    case GateKind::Nand:
        return "nand_gate";
    }
    return "logic_gate";
}

std::string gateDisplayName(GateKind kind) {
    switch (kind) {
    case GateKind::And:
        return "AND Gate";
    case GateKind::Or:
        return "OR Gate";
    case GateKind::Not:
        return "NOT Gate";
    case GateKind::Xor:
        return "XOR Gate";
    case GateKind::Nand:
        return "NAND Gate";
    }
    return "Logic Gate";
}

std::uint8_t readDigitalByte(const SimulationView& view,
                             const Component& component,
                             const std::string& prefix) {
    std::uint8_t value = 0;
    for (int bit = 0; bit < 8; ++bit) {
        if (view.signalAt(component.pinRef(prefix + std::to_string(bit))).logic
            == LogicLevel::High) {
            value |= static_cast<std::uint8_t>(1U << bit);
        }
    }
    return value;
}

} // namespace

GroundComponent::GroundComponent(std::string id, std::string label)
    : Component(std::move(id), std::move(label)) {
    addPin(pin("G", "GND", PinDirection::Power, SignalDomain::Analog, false));
}

std::string GroundComponent::typeId() const { return "ground"; }
std::string GroundComponent::displayName() const { return "Ground"; }
std::string GroundComponent::category() const { return "Sources"; }
std::unique_ptr<Component> GroundComponent::clone() const {
    auto result = std::make_unique<GroundComponent>(id(), label());
    copyCommonStateTo(*result);
    return result;
}
void GroundComponent::stampAnalog(AnalogStampCollector& collector, double) {
    collector.markGround(pinRef("G"));
}

DcSourceComponent::DcSourceComponent(std::string id, std::string label,
                                     std::string concreteType)
    : Component(std::move(id), std::move(label)),
      concreteType_(std::move(concreteType)) {
    addPin(pin("P", "+", PinDirection::Power, SignalDomain::Analog));
    addPin(pin("N", "-", PinDirection::Power, SignalDomain::Analog));
    defineProperty(numberProperty("voltage", "Voltage", 5.0, -1000.0,
                                  1000.0, 0.1, "V"));
}

std::string DcSourceComponent::typeId() const { return concreteType_; }
std::string DcSourceComponent::displayName() const {
    return "DC Voltage Source";
}
std::string DcSourceComponent::category() const { return "Sources"; }
std::unique_ptr<Component> DcSourceComponent::clone() const {
    auto result =
        std::make_unique<DcSourceComponent>(id(), label(), concreteType_);
    copyCommonStateTo(*result);
    return result;
}
void DcSourceComponent::stampAnalog(AnalogStampCollector& collector, double) {
    collector.addVoltageSource(
        {pinRef("P"), pinRef("N"), property("voltage", 5.0),
         id() + ":source"});
}

BatteryComponent::BatteryComponent(std::string id, std::string label)
    : DcSourceComponent(std::move(id), std::move(label), "battery") {
    setProperty("voltage", 9.0);
    defineProperty(numberProperty("internalResistance", "Internal resistance",
                                  0.1, 0.0, 1000.0, 0.01, "ohm"));
}
std::string BatteryComponent::typeId() const { return "battery"; }
std::string BatteryComponent::displayName() const { return "Battery"; }
std::unique_ptr<Component> BatteryComponent::clone() const {
    auto result = std::make_unique<BatteryComponent>(id(), label());
    copyCommonStateTo(*result);
    return result;
}
void BatteryComponent::stampAnalog(AnalogStampCollector& collector, double) {
    const auto resistance =
        safeResistance(property("internalResistance", 0.1));
    const auto conductance = 1.0 / resistance;
    collector.addConductance(
        {pinRef("P"), pinRef("N"), conductance});
    collector.addCurrent(
        {pinRef("N"), pinRef("P"),
         property("voltage", 9.0) * conductance});
}

ClockComponent::ClockComponent(std::string id, std::string label)
    : Component(std::move(id), std::move(label)) {
    addPin(pin("OUT", "OUT", PinDirection::Output, SignalDomain::Digital));
    addPin(pin("GND", "GND", PinDirection::Power, SignalDomain::Digital,
               false));
    defineProperty(numberProperty("frequency", "Frequency", 1.0, 0.001,
                                  1.0e9, 0.1, "Hz"));
    defineProperty(numberProperty("dutyCycle", "Duty cycle", 50.0, 1.0,
                                  99.0, 1.0, "%"));
    defineProperty(numberProperty("highVoltage", "HIGH voltage", 5.0, 1.0,
                                  24.0, 0.1, "V"));
}
std::string ClockComponent::typeId() const { return "clock"; }
std::string ClockComponent::displayName() const { return "Clock Generator"; }
std::string ClockComponent::category() const { return "Sources"; }
std::unique_ptr<Component> ClockComponent::clone() const {
    auto result = std::make_unique<ClockComponent>(id(), label());
    copyCommonStateTo(*result);
    return result;
}
std::vector<DriveRequest>
ClockComponent::evaluate(const SimulationView& view) {
    const auto frequency = std::max(0.001, property("frequency", 1.0));
    const auto period = 1.0 / frequency;
    const auto phase = std::fmod(std::max(0.0, view.timeSeconds()), period);
    const auto high = phase < period * property("dutyCycle", 50.0) / 100.0;
    return {{"OUT", high ? Signal::high(property("highVoltage", 5.0))
                         : Signal::low(),
             0.0}};
}

ResistorComponent::ResistorComponent(std::string id, std::string label)
    : Component(std::move(id), std::move(label)) {
    addPin(pin("A", "A", PinDirection::Passive, SignalDomain::Analog));
    addPin(pin("B", "B", PinDirection::Passive, SignalDomain::Analog));
    defineProperty(numberProperty("resistance", "Resistance", 1000.0, 1.0e-3,
                                  1.0e12, 1.0, "ohm"));
}
std::string ResistorComponent::typeId() const { return "resistor"; }
std::string ResistorComponent::displayName() const { return "Resistor"; }
std::string ResistorComponent::category() const { return "Analog"; }
std::unique_ptr<Component> ResistorComponent::clone() const {
    auto result = std::make_unique<ResistorComponent>(id(), label());
    copyCommonStateTo(*result);
    return result;
}
void ResistorComponent::stampAnalog(AnalogStampCollector& collector, double) {
    collector.addConductance(
        {pinRef("A"), pinRef("B"),
         1.0 / safeResistance(property("resistance", 1000.0))});
}

CapacitorComponent::CapacitorComponent(std::string id, std::string label)
    : Component(std::move(id), std::move(label)) {
    addPin(pin("A", "A", PinDirection::Passive, SignalDomain::Analog));
    addPin(pin("B", "B", PinDirection::Passive, SignalDomain::Analog));
    defineProperty(numberProperty("capacitance", "Capacitance", 1.0e-6,
                                  1.0e-15, 1.0e3, 1.0e-6, "F"));
    defineProperty(numberProperty("initialVoltage", "Initial voltage", 0.0,
                                  -1000.0, 1000.0, 0.1, "V"));
    resetRuntime();
}
std::string CapacitorComponent::typeId() const { return "capacitor"; }
std::string CapacitorComponent::displayName() const { return "Capacitor"; }
std::string CapacitorComponent::category() const { return "Analog"; }
std::unique_ptr<Component> CapacitorComponent::clone() const {
    auto result = std::make_unique<CapacitorComponent>(id(), label());
    copyCommonStateTo(*result);
    return result;
}
void CapacitorComponent::resetRuntime() {
    Component::resetRuntime();
    setRuntimeValue("previousVoltage", property("initialVoltage", 0.0));
    setRuntimeValue("current", 0.0);
}
void CapacitorComponent::stampAnalog(AnalogStampCollector& collector,
                                     double dt) {
    const auto conductance =
        property("capacitance", 1.0e-6) / safeTimeStep(dt);
    const auto previous = runtimeValue("previousVoltage", 0.0);
    collector.addConductance({pinRef("A"), pinRef("B"), conductance});
    collector.addCurrent(
        {pinRef("A"), pinRef("B"), -conductance * previous});
}
void CapacitorComponent::acceptAnalogResult(const AnalogResultView& result,
                                            double dt) {
    const auto voltage = result.voltageAt(pinRef("A"))
        - result.voltageAt(pinRef("B"));
    const auto previous = runtimeValue("previousVoltage", 0.0);
    const auto current =
        property("capacitance", 1.0e-6) * (voltage - previous)
        / safeTimeStep(dt);
    setRuntimeValue("previousVoltage", voltage);
    setRuntimeValue("current", current);
}

InductorComponent::InductorComponent(std::string id, std::string label)
    : Component(std::move(id), std::move(label)) {
    addPin(pin("A", "A", PinDirection::Passive, SignalDomain::Analog));
    addPin(pin("B", "B", PinDirection::Passive, SignalDomain::Analog));
    defineProperty(numberProperty("inductance", "Inductance", 1.0e-3,
                                  1.0e-12, 1.0e6, 1.0e-3, "H"));
    defineProperty(numberProperty("initialCurrent", "Initial current", 0.0,
                                  -1.0e6, 1.0e6, 0.001, "A"));
    resetRuntime();
}
std::string InductorComponent::typeId() const { return "inductor"; }
std::string InductorComponent::displayName() const { return "Inductor"; }
std::string InductorComponent::category() const { return "Analog"; }
std::unique_ptr<Component> InductorComponent::clone() const {
    auto result = std::make_unique<InductorComponent>(id(), label());
    copyCommonStateTo(*result);
    return result;
}
void InductorComponent::resetRuntime() {
    Component::resetRuntime();
    setRuntimeValue("previousCurrent", property("initialCurrent", 0.0));
    setRuntimeValue("current", property("initialCurrent", 0.0));
}
void InductorComponent::stampAnalog(AnalogStampCollector& collector,
                                    double dt) {
    const auto conductance =
        safeTimeStep(dt) / std::max(property("inductance", 1.0e-3), 1.0e-15);
    collector.addConductance({pinRef("A"), pinRef("B"), conductance});
    collector.addCurrent(
        {pinRef("A"), pinRef("B"),
         runtimeValue("previousCurrent", 0.0)});
}
void InductorComponent::acceptAnalogResult(const AnalogResultView& result,
                                           double dt) {
    const auto voltage = result.voltageAt(pinRef("A"))
        - result.voltageAt(pinRef("B"));
    const auto current = runtimeValue("previousCurrent", 0.0)
        + safeTimeStep(dt) / std::max(property("inductance", 1.0e-3), 1.0e-15)
            * voltage;
    setRuntimeValue("previousCurrent", current);
    setRuntimeValue("current", current);
}

PotentiometerComponent::PotentiometerComponent(std::string id,
                                               std::string label)
    : Component(std::move(id), std::move(label)) {
    addPin(pin("A", "A", PinDirection::Passive, SignalDomain::Analog));
    addPin(pin("W", "W", PinDirection::Passive, SignalDomain::Analog));
    addPin(pin("B", "B", PinDirection::Passive, SignalDomain::Analog));
    defineProperty(numberProperty("resistance", "Total resistance", 10000.0,
                                  1.0, 1.0e9, 100.0, "ohm"));
    defineProperty(numberProperty("wiper", "Wiper", 50.0, 0.1, 99.9, 1.0,
                                  "%", true));
}
std::string PotentiometerComponent::typeId() const { return "potentiometer"; }
std::string PotentiometerComponent::displayName() const {
    return "Potentiometer";
}
std::string PotentiometerComponent::category() const { return "Analog"; }
std::unique_ptr<Component> PotentiometerComponent::clone() const {
    auto result = std::make_unique<PotentiometerComponent>(id(), label());
    copyCommonStateTo(*result);
    return result;
}
void PotentiometerComponent::stampAnalog(AnalogStampCollector& collector,
                                         double) {
    const auto total = safeResistance(property("resistance", 10000.0));
    const auto ratio = property("wiper", 50.0) / 100.0;
    collector.addConductance(
        {pinRef("A"), pinRef("W"), 1.0 / safeResistance(total * ratio)});
    collector.addConductance(
        {pinRef("W"), pinRef("B"),
         1.0 / safeResistance(total * (1.0 - ratio))});
}

SwitchComponent::SwitchComponent(std::string id, std::string label,
                                 std::string concreteType)
    : Component(std::move(id), std::move(label)),
      concreteType_(std::move(concreteType)) {
    addPin(pin("A", "A", PinDirection::Passive, SignalDomain::Mixed));
    addPin(pin("B", "B", PinDirection::Passive, SignalDomain::Mixed));
    defineProperty(booleanProperty("closed", "Closed", false, true));
}
std::string SwitchComponent::typeId() const { return concreteType_; }
std::string SwitchComponent::displayName() const { return "Switch"; }
std::string SwitchComponent::category() const { return "Interactive"; }
std::unique_ptr<Component> SwitchComponent::clone() const {
    auto result =
        std::make_unique<SwitchComponent>(id(), label(), concreteType_);
    copyCommonStateTo(*result);
    return result;
}
void SwitchComponent::stampAnalog(AnalogStampCollector& collector, double) {
    const auto resistance = property("closed", false) ? 1.0e-6 : 1.0e12;
    collector.addConductance(
        {pinRef("A"), pinRef("B"), 1.0 / resistance});
}

PushButtonComponent::PushButtonComponent(std::string id, std::string label)
    : SwitchComponent(std::move(id), std::move(label), "push_button") {
}
std::string PushButtonComponent::typeId() const { return "push_button"; }
std::string PushButtonComponent::displayName() const { return "Push Button"; }
std::unique_ptr<Component> PushButtonComponent::clone() const {
    auto result = std::make_unique<PushButtonComponent>(id(), label());
    copyCommonStateTo(*result);
    return result;
}

LedComponent::LedComponent(std::string id, std::string label)
    : Component(std::move(id), std::move(label)) {
    addPin(pin("A", "A", PinDirection::Passive, SignalDomain::Analog));
    addPin(pin("K", "K", PinDirection::Passive, SignalDomain::Analog));
    defineProperty(numberProperty("forwardVoltage", "Forward voltage", 1.8,
                                  0.1, 10.0, 0.1, "V"));
    defineProperty(numberProperty("seriesResistance", "Series resistance",
                                  330.0, 1.0, 1.0e6, 1.0, "ohm"));
    defineProperty(textProperty("color", "Color", "#ff3b30",
                                PropertyKind::Color));
    setRuntimeValue("lit", false);
}
std::string LedComponent::typeId() const { return "led"; }
std::string LedComponent::displayName() const { return "LED"; }
std::string LedComponent::category() const { return "Outputs"; }
std::unique_ptr<Component> LedComponent::clone() const {
    auto result = std::make_unique<LedComponent>(id(), label());
    copyCommonStateTo(*result);
    return result;
}
void LedComponent::stampAnalog(AnalogStampCollector& collector, double) {
    collector.addConductance(
        {pinRef("A"), pinRef("K"),
         1.0 / safeResistance(property("seriesResistance", 330.0))});
}
void LedComponent::acceptAnalogResult(const AnalogResultView& result, double) {
    const auto voltage = result.voltageAt(pinRef("A"))
        - result.voltageAt(pinRef("K"));
    setRuntimeValue("lit", voltage >= property("forwardVoltage", 1.8));
    setRuntimeValue(
        "current",
        std::max(0.0, voltage - property("forwardVoltage", 1.8))
            / safeResistance(property("seriesResistance", 330.0)));
}

SevenSegmentComponent::SevenSegmentComponent(std::string id,
                                             std::string label)
    : Component(std::move(id), std::move(label)) {
    for (const auto* segment : {"A", "B", "C", "D", "E", "F", "G", "DP"}) {
        addPin(pin(segment, segment, PinDirection::Input,
                   SignalDomain::Digital));
    }
    addPin(pin("COM", "COM", PinDirection::Power, SignalDomain::Digital));
    defineProperty(booleanProperty("commonAnode", "Common anode", false));
    setRuntimeValue("segments", 0);
}
std::string SevenSegmentComponent::typeId() const { return "seven_segment"; }
std::string SevenSegmentComponent::displayName() const {
    return "7-Segment Display";
}
std::string SevenSegmentComponent::category() const { return "Outputs"; }
std::unique_ptr<Component> SevenSegmentComponent::clone() const {
    auto result = std::make_unique<SevenSegmentComponent>(id(), label());
    copyCommonStateTo(*result);
    return result;
}
std::vector<DriveRequest>
SevenSegmentComponent::evaluate(const SimulationView& view) {
    int mask = 0;
    const auto commonAnode = property("commonAnode", false);
    const std::array<std::string, 8> names{
        "A", "B", "C", "D", "E", "F", "G", "DP"};
    for (int index = 0; index < 8; ++index) {
        const auto level = view.signalAt(pinRef(names[index])).logic;
        const auto on = commonAnode ? level == LogicLevel::Low
                                   : level == LogicLevel::High;
        if (on) {
            mask |= 1 << index;
        }
    }
    setRuntimeValue("segments", mask);
    return {};
}

LogicGateComponent::LogicGateComponent(std::string id, std::string label,
                                       GateKind kind, int inputCount)
    : Component(std::move(id), std::move(label)), kind_(kind) {
    const auto maximumInputs = kind == GateKind::Not ? 1 : 4;
    for (int index = 0; index < maximumInputs; ++index) {
        addPin(pin("IN" + std::to_string(index),
                   "IN" + std::to_string(index + 1), PinDirection::Input,
                   SignalDomain::Digital, index < inputCount));
    }
    addPin(pin("OUT", "OUT", PinDirection::Output, SignalDomain::Digital));
    defineProperty(integerProperty("inputCount", "Input count",
                                   kind == GateKind::Not ? 1 : inputCount,
                                   kind == GateKind::Not ? 1 : 2,
                                   maximumInputs));
    defineProperty(numberProperty("propagationDelay", "Propagation delay",
                                  1.0e-6, 0.0, 10.0, 1.0e-6, "s"));
    defineProperty(numberProperty("highVoltage", "HIGH voltage", 5.0, 1.0,
                                  24.0, 0.1, "V"));
}
std::string LogicGateComponent::typeId() const {
    return gateTypeId(kind_);
}
std::string LogicGateComponent::displayName() const {
    return gateDisplayName(kind_);
}
std::string LogicGateComponent::category() const { return "Digital"; }
std::unique_ptr<Component> LogicGateComponent::clone() const {
    auto result = std::make_unique<LogicGateComponent>(
        id(), label(), kind_, property("inputCount", 2));
    copyCommonStateTo(*result);
    return result;
}
std::vector<DriveRequest>
LogicGateComponent::evaluate(const SimulationView& view) {
    const auto count = kind_ == GateKind::Not
        ? 1
        : std::clamp(property("inputCount", 2), 2, 4);
    std::vector<LogicLevel> inputs;
    inputs.reserve(static_cast<std::size_t>(count));
    for (int index = 0; index < count; ++index) {
        const auto level =
            view.signalAt(pinRef("IN" + std::to_string(index))).logic;
        inputs.push_back(level);
        if (level == LogicLevel::Undefined) {
            view.logWarning(label(), "Floating input detected.");
            return {{"OUT", Signal::undefined(),
                     property("propagationDelay", 1.0e-6)}};
        }
    }

    bool value = false;
    if (kind_ == GateKind::And || kind_ == GateKind::Nand) {
        value = std::all_of(inputs.begin(), inputs.end(), [](LogicLevel level) {
            return level == LogicLevel::High;
        });
        if (kind_ == GateKind::Nand) {
            value = !value;
        }
    } else if (kind_ == GateKind::Or) {
        value = std::any_of(inputs.begin(), inputs.end(), [](LogicLevel level) {
            return level == LogicLevel::High;
        });
    } else if (kind_ == GateKind::Xor) {
        value = std::count(inputs.begin(), inputs.end(), LogicLevel::High) % 2
            == 1;
    } else {
        value = inputs.front() == LogicLevel::Low;
    }

    return {{"OUT",
             value ? Signal::high(property("highVoltage", 5.0))
                   : Signal::low(),
             property("propagationDelay", 1.0e-6)}};
}
GateKind LogicGateComponent::kind() const noexcept { return kind_; }

DFlipFlopComponent::DFlipFlopComponent(std::string id, std::string label)
    : Component(std::move(id), std::move(label)) {
    addPin(pin("D", "D", PinDirection::Input, SignalDomain::Digital));
    addPin(pin("CLK", "CLK", PinDirection::Input, SignalDomain::Digital));
    addPin(pin("Q", "Q", PinDirection::Output, SignalDomain::Digital));
    defineProperty(numberProperty("propagationDelay", "Propagation delay",
                                  1.0e-6, 0.0, 10.0, 1.0e-6, "s"));
    resetRuntime();
}
std::string DFlipFlopComponent::typeId() const { return "d_flip_flop"; }
std::string DFlipFlopComponent::displayName() const { return "D Flip-Flop"; }
std::string DFlipFlopComponent::category() const { return "Digital"; }
std::unique_ptr<Component> DFlipFlopComponent::clone() const {
    auto result = std::make_unique<DFlipFlopComponent>(id(), label());
    copyCommonStateTo(*result);
    return result;
}
void DFlipFlopComponent::resetRuntime() {
    Component::resetRuntime();
    setRuntimeValue("previousClock", static_cast<int>(LogicLevel::Low));
    setRuntimeValue("q", static_cast<int>(LogicLevel::Low));
}
std::vector<DriveRequest>
DFlipFlopComponent::evaluate(const SimulationView& view) {
    const auto clock = view.signalAt(pinRef("CLK")).logic;
    const auto previous = static_cast<LogicLevel>(
        runtimeValue("previousClock", static_cast<int>(LogicLevel::Low)));
    auto q = static_cast<LogicLevel>(
        runtimeValue("q", static_cast<int>(LogicLevel::Low)));
    if (clock == LogicLevel::Undefined) {
        view.logWarning(label(), "Floating clock input detected.");
        q = LogicLevel::Undefined;
    } else if (previous == LogicLevel::Low && clock == LogicLevel::High) {
        q = view.signalAt(pinRef("D")).logic;
        if (q == LogicLevel::Undefined) {
            view.logWarning(label(), "Floating input detected.");
        }
    }
    setRuntimeValue("previousClock", static_cast<int>(clock));
    setRuntimeValue("q", static_cast<int>(q));
    return {{"Q", logicSignal(q),
             property("propagationDelay", 1.0e-6)}};
}

AdcComponent::AdcComponent(std::string id, std::string label, int bitCount)
    : Component(std::move(id), std::move(label)) {
    addPin(pin("VIN", "VIN", PinDirection::Input, SignalDomain::Analog));
    addPin(pin("VREFP", "VREF+", PinDirection::Input, SignalDomain::Analog));
    addPin(pin("VREFN", "VREF-", PinDirection::Input, SignalDomain::Analog));
    for (int bit = 0; bit < 16; ++bit) {
        addPin(pin("D" + std::to_string(bit), "D" + std::to_string(bit),
                   PinDirection::Output, SignalDomain::Digital, false));
    }
    defineProperty(integerProperty("bitCount", "Resolution", bitCount, 2, 16));
    defineProperty(numberProperty("conversionDelay", "Conversion delay",
                                  10.0e-6, 0.0, 10.0, 1.0e-6, "s"));
}
std::string AdcComponent::typeId() const { return "adc"; }
std::string AdcComponent::displayName() const { return "ADC"; }
std::string AdcComponent::category() const { return "Advanced"; }
std::unique_ptr<Component> AdcComponent::clone() const {
    auto result =
        std::make_unique<AdcComponent>(id(), label(), property("bitCount", 8));
    copyCommonStateTo(*result);
    return result;
}
std::vector<DriveRequest>
AdcComponent::evaluate(const SimulationView& view) {
    const auto bits = std::clamp(property("bitCount", 8), 2, 16);
    const auto minimum = view.voltageAt(pinRef("VREFN"));
    const auto maximum = view.voltageAt(pinRef("VREFP"));
    const auto input = view.voltageAt(pinRef("VIN"));
    if (!std::isfinite(minimum) || !std::isfinite(maximum)
        || !std::isfinite(input) || maximum <= minimum) {
        view.logWarning(label(), "ADC reference or input is undefined.");
        std::vector<DriveRequest> undefined;
        for (int bit = 0; bit < bits; ++bit) {
            undefined.push_back(
                {"D" + std::to_string(bit), Signal::undefined(),
                 property("conversionDelay", 10.0e-6)});
        }
        return undefined;
    }
    const auto clamped = std::clamp(input, minimum, maximum);
    const auto maximumCode = (1U << bits) - 1U;
    const auto normalized = (clamped - minimum) / (maximum - minimum);
    const auto code = static_cast<unsigned int>(
        std::llround(normalized * static_cast<double>(maximumCode)));
    std::vector<DriveRequest> result;
    for (int bit = 0; bit < bits; ++bit) {
        result.push_back(
            {"D" + std::to_string(bit),
             (code & (1U << bit)) ? Signal::high() : Signal::low(),
             property("conversionDelay", 10.0e-6)});
    }
    setRuntimeValue("code", static_cast<int>(code));
    return result;
}

DacComponent::DacComponent(std::string id, std::string label, int bitCount)
    : Component(std::move(id), std::move(label)) {
    for (int bit = 0; bit < 16; ++bit) {
        addPin(pin("D" + std::to_string(bit), "D" + std::to_string(bit),
                   PinDirection::Input, SignalDomain::Digital, false));
    }
    addPin(pin("VREFP", "VREF+", PinDirection::Input, SignalDomain::Analog));
    addPin(pin("VREFN", "VREF-", PinDirection::Input, SignalDomain::Analog));
    addPin(pin("OUT", "OUT", PinDirection::Output, SignalDomain::Analog));
    defineProperty(integerProperty("bitCount", "Resolution", bitCount, 2, 16));
    defineProperty(numberProperty("conversionDelay", "Conversion delay",
                                  10.0e-6, 0.0, 10.0, 1.0e-6, "s"));
}
std::string DacComponent::typeId() const { return "dac"; }
std::string DacComponent::displayName() const { return "DAC"; }
std::string DacComponent::category() const { return "Advanced"; }
std::unique_ptr<Component> DacComponent::clone() const {
    auto result =
        std::make_unique<DacComponent>(id(), label(), property("bitCount", 8));
    copyCommonStateTo(*result);
    return result;
}
std::vector<DriveRequest>
DacComponent::evaluate(const SimulationView& view) {
    const auto bits = std::clamp(property("bitCount", 8), 2, 16);
    unsigned int code = 0;
    for (int bit = 0; bit < bits; ++bit) {
        const auto level =
            view.signalAt(pinRef("D" + std::to_string(bit))).logic;
        if (level == LogicLevel::Undefined) {
            view.logWarning(label(), "DAC digital input is undefined.");
            return {{"OUT", Signal::undefined(),
                     property("conversionDelay", 10.0e-6)}};
        }
        if (level == LogicLevel::High) {
            code |= 1U << bit;
        }
    }
    const auto minimum = view.voltageAt(pinRef("VREFN"));
    const auto maximum = view.voltageAt(pinRef("VREFP"));
    if (!std::isfinite(minimum) || !std::isfinite(maximum)
        || maximum <= minimum) {
        return {{"OUT", Signal::undefined(),
                 property("conversionDelay", 10.0e-6)}};
    }
    const auto maximumCode = (1U << bits) - 1U;
    const auto voltage = minimum
        + (maximum - minimum) * static_cast<double>(code)
            / static_cast<double>(maximumCode);
    setRuntimeValue("outputVoltage", voltage);
    return {{"OUT", Signal::analog(voltage),
             property("conversionDelay", 10.0e-6)}};
}

MicrocontrollerComponent::MicrocontrollerComponent(std::string id,
                                                   std::string label)
    : Component(std::move(id), std::move(label)) {
    for (const auto* port : {"PA", "PB"}) {
        for (int bit = 0; bit < 8; ++bit) {
            addPin(pin(std::string(port) + std::to_string(bit),
                       std::string(port) + std::to_string(bit),
                       PinDirection::Bidirectional, SignalDomain::Digital,
                       false));
        }
    }
    addPin(pin("VCC", "VCC", PinDirection::Power, SignalDomain::Analog));
    addPin(pin("GND", "GND", PinDirection::Power, SignalDomain::Analog));
    defineProperty(numberProperty("clockHz", "CPU clock", 1000.0, 1.0,
                                  1.0e9, 100.0, "Hz"));
    defineProperty(textProperty("firmwarePath", "Firmware (.hex)", {},
                                PropertyKind::FilePath));
    resetRuntime();
}
std::string MicrocontrollerComponent::typeId() const {
    return "microcontroller";
}
std::string MicrocontrollerComponent::displayName() const {
    return "Microcontroller";
}
std::string MicrocontrollerComponent::category() const { return "Advanced"; }
std::unique_ptr<Component> MicrocontrollerComponent::clone() const {
    auto result = std::make_unique<MicrocontrollerComponent>(id(), label());
    copyCommonStateTo(*result);
    result->flash_ = flash_;
    result->ram_ = ram_;
    result->registers_ = registers_;
    result->ports_ = ports_;
    result->pc_ = pc_;
    result->halted_ = halted_;
    result->nextInstructionTime_ = nextInstructionTime_;
    return result;
}
void MicrocontrollerComponent::resetRuntime() {
    Component::resetRuntime();
    ram_.fill(0);
    registers_.fill(0);
    ports_.fill(0);
    pc_ = 0;
    halted_ = false;
    nextInstructionTime_ = 0.0;
    setRuntimeValue("pc", 0);
    setRuntimeValue("halted", false);
}
std::vector<DriveRequest>
MicrocontrollerComponent::evaluate(const SimulationView& view) {
    const auto frequency = std::max(1.0, property("clockHz", 1000.0));
    int guard = 0;
    while (!halted_ && view.timeSeconds() + 1.0e-15
               >= nextInstructionTime_
           && guard++ < 64) {
        executeInstruction(view);
        nextInstructionTime_ += 1.0 / frequency;
    }

    std::vector<DriveRequest> result;
    for (int port = 0; port < 2; ++port) {
        for (int bit = 0; bit < 8; ++bit) {
            result.push_back(
                {std::string(port == 0 ? "PA" : "PB") + std::to_string(bit),
                 (ports_[port] & (1U << bit)) ? Signal::high()
                                              : Signal::low(),
                 0.0});
        }
    }
    setRuntimeValue("pc", static_cast<int>(pc_));
    setRuntimeValue("accumulator", static_cast<int>(registers_[0]));
    setRuntimeValue("portA", static_cast<int>(ports_[0]));
    setRuntimeValue("portB", static_cast<int>(ports_[1]));
    setRuntimeValue("halted", halted_);
    return result;
}
void MicrocontrollerComponent::loadFlash(std::vector<std::uint8_t> bytes) {
    flash_ = std::move(bytes);
    resetRuntime();
}
void MicrocontrollerComponent::restoreExecutionStateFromRuntime() {
    pc_ = static_cast<std::size_t>(
        std::max(0, runtimeValue("pc", 0)));
    registers_[0] = static_cast<std::uint8_t>(
        std::clamp(runtimeValue("accumulator", 0), 0, 255));
    ports_[0] = static_cast<std::uint8_t>(
        std::clamp(runtimeValue("portA", 0), 0, 255));
    ports_[1] = static_cast<std::uint8_t>(
        std::clamp(runtimeValue("portB", 0), 0, 255));
    halted_ = runtimeValue("halted", false);
    nextInstructionTime_ = 0.0;
}
const std::vector<std::uint8_t>&
MicrocontrollerComponent::flash() const noexcept {
    return flash_;
}
std::size_t MicrocontrollerComponent::programCounter() const noexcept {
    return pc_;
}
const std::array<std::uint8_t, 8>&
MicrocontrollerComponent::registers() const noexcept {
    return registers_;
}
void MicrocontrollerComponent::executeInstruction(
    const SimulationView& view) {
    if (pc_ >= flash_.size()) {
        halted_ = true;
        return;
    }
    const auto opcode = flash_[pc_++];
    const auto readByte = [this]() -> std::uint8_t {
        if (pc_ >= flash_.size()) {
            halted_ = true;
            return 0;
        }
        return flash_[pc_++];
    };

    switch (opcode) {
    case 0x00: // NOP
        break;
    case 0x01: { // MOV Rn, imm
        const auto reg = readByte() & 0x07U;
        registers_[reg] = readByte();
        break;
    }
    case 0x02: { // ADD Rn, imm
        const auto reg = readByte() & 0x07U;
        registers_[reg] =
            static_cast<std::uint8_t>(registers_[reg] + readByte());
        break;
    }
    case 0x03: { // JMP address16
        const auto high = readByte();
        const auto low = readByte();
        pc_ = static_cast<std::size_t>((high << 8U) | low);
        break;
    }
    case 0x04: { // SETB port, bit
        const auto port = readByte() & 0x01U;
        const auto bit = readByte() & 0x07U;
        ports_[port] |= static_cast<std::uint8_t>(1U << bit);
        break;
    }
    case 0x05: { // CLR port, bit
        const auto port = readByte() & 0x01U;
        const auto bit = readByte() & 0x07U;
        ports_[port] &= static_cast<std::uint8_t>(~(1U << bit));
        break;
    }
    case 0x06: { // IN Rn, port
        const auto reg = readByte() & 0x07U;
        const auto port = readByte() & 0x01U;
        registers_[reg] =
            readDigitalByte(view, *this, port == 0 ? "PA" : "PB");
        break;
    }
    case 0x07: { // OUT port, Rn
        const auto port = readByte() & 0x01U;
        const auto reg = readByte() & 0x07U;
        ports_[port] = registers_[reg];
        break;
    }
    case 0x08: { // STORE address, Rn
        const auto address = readByte();
        const auto reg = readByte() & 0x07U;
        ram_[address] = registers_[reg];
        break;
    }
    case 0x09: { // LOAD Rn, address
        const auto reg = readByte() & 0x07U;
        const auto address = readByte();
        registers_[reg] = ram_[address];
        break;
    }
    case 0xFF:
        halted_ = true;
        break;
    default:
        view.logWarning(label(), "Unknown MCU opcode "
                             + std::to_string(opcode) + ".");
        halted_ = true;
        break;
    }
}

ExternalMemoryComponent::ExternalMemoryComponent(std::string id,
                                                 std::string label)
    : Component(std::move(id), std::move(label)), storage_(256, 0) {
    for (int bit = 0; bit < 16; ++bit) {
        addPin(pin("A" + std::to_string(bit), "A" + std::to_string(bit),
                   PinDirection::Input, SignalDomain::Digital));
    }
    for (int bit = 0; bit < 8; ++bit) {
        addPin(pin("D" + std::to_string(bit), "D" + std::to_string(bit),
                   PinDirection::Bidirectional, SignalDomain::Digital));
    }
    addPin(pin("RD", "RD", PinDirection::Input, SignalDomain::Digital));
    addPin(pin("WR", "WR", PinDirection::Input, SignalDomain::Digital));
    defineProperty(integerProperty("size", "Memory bytes", 256, 16, 65536));
}
std::string ExternalMemoryComponent::typeId() const {
    return "external_memory";
}
std::string ExternalMemoryComponent::displayName() const {
    return "External RAM / EEPROM";
}
std::string ExternalMemoryComponent::category() const { return "Advanced"; }
std::unique_ptr<Component> ExternalMemoryComponent::clone() const {
    auto result = std::make_unique<ExternalMemoryComponent>(id(), label());
    copyCommonStateTo(*result);
    result->storage_ = storage_;
    result->previousWrite_ = previousWrite_;
    return result;
}
void ExternalMemoryComponent::resetRuntime() {
    Component::resetRuntime();
    std::fill(storage_.begin(), storage_.end(), 0);
    previousWrite_ = LogicLevel::Low;
}
std::vector<DriveRequest>
ExternalMemoryComponent::evaluate(const SimulationView& view) {
    const auto configuredSize = static_cast<std::size_t>(
        std::clamp(property("size", 256), 16, 65536));
    if (storage_.size() != configuredSize) {
        storage_.resize(configuredSize, 0);
    }
    std::uint16_t address = 0;
    for (int bit = 0; bit < 16; ++bit) {
        if (view.signalAt(pinRef("A" + std::to_string(bit))).logic
            == LogicLevel::High) {
            address |= static_cast<std::uint16_t>(1U << bit);
        }
    }
    const auto read = view.signalAt(pinRef("RD")).logic;
    const auto write = view.signalAt(pinRef("WR")).logic;
    if (previousWrite_ == LogicLevel::Low && write == LogicLevel::High) {
        storage_[address % storage_.size()] =
            readDigitalByte(view, *this, "D");
    }
    previousWrite_ = write;

    std::vector<DriveRequest> result;
    if (read == LogicLevel::High) {
        const auto value = storage_[address % storage_.size()];
        for (int bit = 0; bit < 8; ++bit) {
            result.push_back(
                {"D" + std::to_string(bit),
                 (value & (1U << bit)) ? Signal::high() : Signal::low(),
                 0.0});
        }
    }
    return result;
}

LcdComponent::LcdComponent(std::string id, std::string label)
    : Component(std::move(id), std::move(label)) {
    for (int bit = 0; bit < 8; ++bit) {
        addPin(pin("D" + std::to_string(bit), "D" + std::to_string(bit),
                   PinDirection::Input, SignalDomain::Digital));
    }
    addPin(pin("RS", "RS", PinDirection::Input, SignalDomain::Digital));
    addPin(pin("RW", "RW", PinDirection::Input, SignalDomain::Digital));
    addPin(pin("E", "E", PinDirection::Input, SignalDomain::Digital));
    resetRuntime();
}
std::string LcdComponent::typeId() const { return "lcd_16x2"; }
std::string LcdComponent::displayName() const { return "LCD 16x2"; }
std::string LcdComponent::category() const { return "Advanced"; }
std::unique_ptr<Component> LcdComponent::clone() const {
    auto result = std::make_unique<LcdComponent>(id(), label());
    copyCommonStateTo(*result);
    result->characters_ = characters_;
    result->cursor_ = cursor_;
    result->previousEnable_ = previousEnable_;
    return result;
}
void LcdComponent::resetRuntime() {
    Component::resetRuntime();
    characters_.fill(' ');
    cursor_ = 0;
    previousEnable_ = LogicLevel::Low;
    setRuntimeValue("line1", std::string(16, ' '));
    setRuntimeValue("line2", std::string(16, ' '));
}
std::vector<DriveRequest>
LcdComponent::evaluate(const SimulationView& view) {
    const auto enable = view.signalAt(pinRef("E")).logic;
    const auto readWrite = view.signalAt(pinRef("RW")).logic;
    if (previousEnable_ == LogicLevel::Low && enable == LogicLevel::High
        && readWrite != LogicLevel::High) {
        const auto value = readDigitalByte(view, *this, "D");
        if (view.signalAt(pinRef("RS")).logic == LogicLevel::High) {
            writeCharacter(static_cast<char>(value));
        } else {
            handleCommand(value);
        }
    }
    previousEnable_ = enable;
    const auto currentLines = lines();
    setRuntimeValue("line1", currentLines[0]);
    setRuntimeValue("line2", currentLines[1]);
    return {};
}
std::array<std::string, 2> LcdComponent::lines() const {
    return {
        std::string(characters_.begin(), characters_.begin() + 16),
        std::string(characters_.begin() + 16, characters_.end())};
}
void LcdComponent::handleCommand(std::uint8_t command) {
    if (command == 0x01) {
        characters_.fill(' ');
        cursor_ = 0;
    } else if (command == 0x02) {
        cursor_ = 0;
    } else if (command & 0x80U) {
        const auto address = command & 0x7FU;
        cursor_ = address >= 0x40U ? 16U + (address - 0x40U) : address;
        cursor_ = std::min<std::size_t>(31, cursor_);
    }
}
void LcdComponent::writeCharacter(char character) {
    characters_[cursor_ % characters_.size()] = character;
    cursor_ = (cursor_ + 1) % characters_.size();
}

KeypadComponent::KeypadComponent(std::string id, std::string label)
    : Component(std::move(id), std::move(label)) {
    for (int index = 0; index < 4; ++index) {
        addPin(pin("R" + std::to_string(index), "R" + std::to_string(index),
                   PinDirection::Input, SignalDomain::Digital));
        addPin(pin("C" + std::to_string(index), "C" + std::to_string(index),
                   PinDirection::Output, SignalDomain::Digital));
    }
    defineProperty(integerProperty("pressedKey", "Pressed key", -1, -1, 15,
                                   true));
}
std::string KeypadComponent::typeId() const { return "keypad_4x4"; }
std::string KeypadComponent::displayName() const { return "Keypad 4x4"; }
std::string KeypadComponent::category() const { return "Advanced"; }
std::unique_ptr<Component> KeypadComponent::clone() const {
    auto result = std::make_unique<KeypadComponent>(id(), label());
    copyCommonStateTo(*result);
    return result;
}
std::vector<DriveRequest>
KeypadComponent::evaluate(const SimulationView& view) {
    const auto key = property("pressedKey", -1);
    std::vector<DriveRequest> result;
    for (int column = 0; column < 4; ++column) {
        Signal output = Signal::high();
        if (key >= 0) {
            const auto selectedRow = key / 4;
            const auto selectedColumn = key % 4;
            if (column == selectedColumn
                && view.signalAt(pinRef("R" + std::to_string(selectedRow)))
                       .logic
                    == LogicLevel::Low) {
                output = Signal::low();
            }
        }
        result.push_back({"C" + std::to_string(column), output, 0.0});
    }
    return result;
}

VoltmeterComponent::VoltmeterComponent(std::string id, std::string label)
    : Component(std::move(id), std::move(label)) {
    addPin(pin("P", "+", PinDirection::Input, SignalDomain::Analog));
    addPin(pin("N", "-", PinDirection::Input, SignalDomain::Analog));
    setRuntimeValue("reading", 0.0);
}
std::string VoltmeterComponent::typeId() const { return "voltmeter"; }
std::string VoltmeterComponent::displayName() const {
    return "Digital Voltmeter";
}
std::string VoltmeterComponent::category() const { return "Instruments"; }
std::unique_ptr<Component> VoltmeterComponent::clone() const {
    auto result = std::make_unique<VoltmeterComponent>(id(), label());
    copyCommonStateTo(*result);
    return result;
}
void VoltmeterComponent::acceptAnalogResult(const AnalogResultView& result,
                                            double) {
    setRuntimeValue("reading",
                    result.voltageAt(pinRef("P"))
                        - result.voltageAt(pinRef("N")));
}

AmmeterComponent::AmmeterComponent(std::string id, std::string label)
    : Component(std::move(id), std::move(label)) {
    addPin(pin("A", "A", PinDirection::Passive, SignalDomain::Analog));
    addPin(pin("B", "B", PinDirection::Passive, SignalDomain::Analog));
    defineProperty(numberProperty("shuntResistance", "Shunt resistance",
                                  1.0e-3, 1.0e-9, 1.0, 1.0e-3, "ohm"));
    setRuntimeValue("reading", 0.0);
}
std::string AmmeterComponent::typeId() const { return "ammeter"; }
std::string AmmeterComponent::displayName() const {
    return "Digital Ammeter";
}
std::string AmmeterComponent::category() const { return "Instruments"; }
std::unique_ptr<Component> AmmeterComponent::clone() const {
    auto result = std::make_unique<AmmeterComponent>(id(), label());
    copyCommonStateTo(*result);
    return result;
}
void AmmeterComponent::stampAnalog(AnalogStampCollector& collector, double) {
    collector.addConductance(
        {pinRef("A"), pinRef("B"),
         1.0 / safeResistance(property("shuntResistance", 1.0e-3))});
}
void AmmeterComponent::acceptAnalogResult(const AnalogResultView& result,
                                          double) {
    setRuntimeValue(
        "reading",
        (result.voltageAt(pinRef("A")) - result.voltageAt(pinRef("B")))
            / safeResistance(property("shuntResistance", 1.0e-3)));
}

std::unique_ptr<Component>
ComponentFactory::create(const std::string& type, const std::string& id,
                         const std::string& label) {
    if (type == "ground") return std::make_unique<GroundComponent>(id, label);
    if (type == "dc_source") return std::make_unique<DcSourceComponent>(id, label);
    if (type == "battery") return std::make_unique<BatteryComponent>(id, label);
    if (type == "clock") return std::make_unique<ClockComponent>(id, label);
    if (type == "resistor") return std::make_unique<ResistorComponent>(id, label);
    if (type == "capacitor") return std::make_unique<CapacitorComponent>(id, label);
    if (type == "inductor") return std::make_unique<InductorComponent>(id, label);
    if (type == "potentiometer") return std::make_unique<PotentiometerComponent>(id, label);
    if (type == "switch") return std::make_unique<SwitchComponent>(id, label);
    if (type == "push_button") return std::make_unique<PushButtonComponent>(id, label);
    if (type == "led") return std::make_unique<LedComponent>(id, label);
    if (type == "seven_segment") return std::make_unique<SevenSegmentComponent>(id, label);
    if (type == "and_gate") return std::make_unique<LogicGateComponent>(id, label, GateKind::And);
    if (type == "or_gate") return std::make_unique<LogicGateComponent>(id, label, GateKind::Or);
    if (type == "not_gate") return std::make_unique<LogicGateComponent>(id, label, GateKind::Not, 1);
    if (type == "xor_gate") return std::make_unique<LogicGateComponent>(id, label, GateKind::Xor);
    if (type == "nand_gate") return std::make_unique<LogicGateComponent>(id, label, GateKind::Nand);
    if (type == "d_flip_flop") return std::make_unique<DFlipFlopComponent>(id, label);
    if (type == "adc") return std::make_unique<AdcComponent>(id, label);
    if (type == "dac") return std::make_unique<DacComponent>(id, label);
    if (type == "microcontroller") return std::make_unique<MicrocontrollerComponent>(id, label);
    if (type == "external_memory") return std::make_unique<ExternalMemoryComponent>(id, label);
    if (type == "lcd_16x2") return std::make_unique<LcdComponent>(id, label);
    if (type == "keypad_4x4") return std::make_unique<KeypadComponent>(id, label);
    if (type == "voltmeter") return std::make_unique<VoltmeterComponent>(id, label);
    if (type == "ammeter") return std::make_unique<AmmeterComponent>(id, label);
    throw std::invalid_argument("Unsupported component type: " + type);
}

std::vector<std::string> ComponentFactory::supportedTypes() {
    return {
        "ground", "dc_source", "battery", "clock",
        "resistor", "capacitor", "inductor", "potentiometer",
        "switch", "push_button", "led", "seven_segment",
        "and_gate", "or_gate", "not_gate", "xor_gate", "nand_gate",
        "d_flip_flop", "adc", "dac", "microcontroller",
        "external_memory", "lcd_16x2", "keypad_4x4",
        "voltmeter", "ammeter"};
}

std::string ComponentFactory::suggestedPrefix(const std::string& type) {
    if (type == "ground") return "GND";
    if (type == "dc_source") return "V";
    if (type == "battery") return "BAT";
    if (type == "clock") return "CLK";
    if (type == "resistor") return "R";
    if (type == "capacitor") return "C";
    if (type == "inductor") return "L";
    if (type == "potentiometer") return "POT";
    if (type == "switch") return "SW";
    if (type == "push_button") return "BTN";
    if (type == "led") return "LED";
    if (type == "seven_segment") return "SEG";
    if (type == "d_flip_flop") return "FF";
    if (type == "adc") return "ADC";
    if (type == "dac") return "DAC";
    if (type == "microcontroller") return "MCU";
    if (type == "external_memory") return "MEM";
    if (type == "lcd_16x2") return "LCD";
    if (type == "keypad_4x4") return "KEY";
    if (type == "voltmeter") return "VM";
    if (type == "ammeter") return "AM";
    return "U";
}

} // namespace proteus
