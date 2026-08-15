#pragma once

#include "proteus/core/Component.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace proteus {

class GroundComponent final : public Component {
public:
    GroundComponent(std::string id, std::string label);
    [[nodiscard]] std::string typeId() const override;
    [[nodiscard]] std::string displayName() const override;
    [[nodiscard]] std::string category() const override;
    [[nodiscard]] std::unique_ptr<Component> clone() const override;
    void stampAnalog(AnalogStampCollector& collector, double dt) override;
};

class DcSourceComponent : public Component {
public:
    DcSourceComponent(std::string id, std::string label,
                      std::string concreteType = "dc_source");
    [[nodiscard]] std::string typeId() const override;
    [[nodiscard]] std::string displayName() const override;
    [[nodiscard]] std::string category() const override;
    [[nodiscard]] std::unique_ptr<Component> clone() const override;
    void stampAnalog(AnalogStampCollector& collector, double dt) override;

protected:
    std::string concreteType_;
};

class BatteryComponent final : public DcSourceComponent {
public:
    BatteryComponent(std::string id, std::string label);
    [[nodiscard]] std::string typeId() const override;
    [[nodiscard]] std::string displayName() const override;
    [[nodiscard]] std::unique_ptr<Component> clone() const override;
    void stampAnalog(AnalogStampCollector& collector, double dt) override;
};

class ClockComponent final : public Component {
public:
    ClockComponent(std::string id, std::string label);
    [[nodiscard]] std::string typeId() const override;
    [[nodiscard]] std::string displayName() const override;
    [[nodiscard]] std::string category() const override;
    [[nodiscard]] std::unique_ptr<Component> clone() const override;
    [[nodiscard]] std::vector<DriveRequest> evaluate(const SimulationView& view) override;
};

class ResistorComponent final : public Component {
public:
    ResistorComponent(std::string id, std::string label);
    [[nodiscard]] std::string typeId() const override;
    [[nodiscard]] std::string displayName() const override;
    [[nodiscard]] std::string category() const override;
    [[nodiscard]] std::unique_ptr<Component> clone() const override;
    void stampAnalog(AnalogStampCollector& collector, double dt) override;
};

class CapacitorComponent final : public Component {
public:
    CapacitorComponent(std::string id, std::string label);
    [[nodiscard]] std::string typeId() const override;
    [[nodiscard]] std::string displayName() const override;
    [[nodiscard]] std::string category() const override;
    [[nodiscard]] std::unique_ptr<Component> clone() const override;
    void resetRuntime() override;
    void stampAnalog(AnalogStampCollector& collector, double dt) override;
    void acceptAnalogResult(const AnalogResultView& result, double dt) override;
};

class InductorComponent final : public Component {
public:
    InductorComponent(std::string id, std::string label);
    [[nodiscard]] std::string typeId() const override;
    [[nodiscard]] std::string displayName() const override;
    [[nodiscard]] std::string category() const override;
    [[nodiscard]] std::unique_ptr<Component> clone() const override;
    void resetRuntime() override;
    void stampAnalog(AnalogStampCollector& collector, double dt) override;
    void acceptAnalogResult(const AnalogResultView& result, double dt) override;
};

class PotentiometerComponent final : public Component {
public:
    PotentiometerComponent(std::string id, std::string label);
    [[nodiscard]] std::string typeId() const override;
    [[nodiscard]] std::string displayName() const override;
    [[nodiscard]] std::string category() const override;
    [[nodiscard]] std::unique_ptr<Component> clone() const override;
    void stampAnalog(AnalogStampCollector& collector, double dt) override;
};

class SwitchComponent : public Component {
public:
    SwitchComponent(std::string id, std::string label,
                    std::string concreteType = "switch");
    [[nodiscard]] std::string typeId() const override;
    [[nodiscard]] std::string displayName() const override;
    [[nodiscard]] std::string category() const override;
    [[nodiscard]] std::unique_ptr<Component> clone() const override;
    void stampAnalog(AnalogStampCollector& collector, double dt) override;

protected:
    std::string concreteType_;
};

class PushButtonComponent final : public SwitchComponent {
public:
    PushButtonComponent(std::string id, std::string label);
    [[nodiscard]] std::string typeId() const override;
    [[nodiscard]] std::string displayName() const override;
    [[nodiscard]] std::unique_ptr<Component> clone() const override;
};

class LedComponent final : public Component {
public:
    LedComponent(std::string id, std::string label);
    [[nodiscard]] std::string typeId() const override;
    [[nodiscard]] std::string displayName() const override;
    [[nodiscard]] std::string category() const override;
    [[nodiscard]] std::unique_ptr<Component> clone() const override;
    void stampAnalog(AnalogStampCollector& collector, double dt) override;
    void acceptAnalogResult(const AnalogResultView& result, double dt) override;
};

class SevenSegmentComponent final : public Component {
public:
    SevenSegmentComponent(std::string id, std::string label);
    [[nodiscard]] std::string typeId() const override;
    [[nodiscard]] std::string displayName() const override;
    [[nodiscard]] std::string category() const override;
    [[nodiscard]] std::unique_ptr<Component> clone() const override;
    [[nodiscard]] std::vector<DriveRequest> evaluate(const SimulationView& view) override;
};

enum class GateKind {
    And,
    Or,
    Not,
    Xor,
    Nand
};

class LogicGateComponent final : public Component {
public:
    LogicGateComponent(std::string id, std::string label,
                       GateKind kind, int inputCount = 2);
    [[nodiscard]] std::string typeId() const override;
    [[nodiscard]] std::string displayName() const override;
    [[nodiscard]] std::string category() const override;
    [[nodiscard]] std::unique_ptr<Component> clone() const override;
    [[nodiscard]] std::vector<DriveRequest> evaluate(const SimulationView& view) override;
    [[nodiscard]] GateKind kind() const noexcept;

private:
    GateKind kind_;
};

class DFlipFlopComponent final : public Component {
public:
    DFlipFlopComponent(std::string id, std::string label);
    [[nodiscard]] std::string typeId() const override;
    [[nodiscard]] std::string displayName() const override;
    [[nodiscard]] std::string category() const override;
    [[nodiscard]] std::unique_ptr<Component> clone() const override;
    void resetRuntime() override;
    [[nodiscard]] std::vector<DriveRequest> evaluate(const SimulationView& view) override;
};

class AdcComponent final : public Component {
public:
    AdcComponent(std::string id, std::string label, int bitCount = 8);
    [[nodiscard]] std::string typeId() const override;
    [[nodiscard]] std::string displayName() const override;
    [[nodiscard]] std::string category() const override;
    [[nodiscard]] std::unique_ptr<Component> clone() const override;
    [[nodiscard]] std::vector<DriveRequest> evaluate(const SimulationView& view) override;
};

class DacComponent final : public Component {
public:
    DacComponent(std::string id, std::string label, int bitCount = 8);
    [[nodiscard]] std::string typeId() const override;
    [[nodiscard]] std::string displayName() const override;
    [[nodiscard]] std::string category() const override;
    [[nodiscard]] std::unique_ptr<Component> clone() const override;
    [[nodiscard]] std::vector<DriveRequest> evaluate(const SimulationView& view) override;
};

class MicrocontrollerComponent final : public Component {
public:
    MicrocontrollerComponent(std::string id, std::string label);
    [[nodiscard]] std::string typeId() const override;
    [[nodiscard]] std::string displayName() const override;
    [[nodiscard]] std::string category() const override;
    [[nodiscard]] std::unique_ptr<Component> clone() const override;
    void resetRuntime() override;
    [[nodiscard]] std::vector<DriveRequest> evaluate(const SimulationView& view) override;

    void loadFlash(std::vector<std::uint8_t> bytes);
    void restoreExecutionStateFromRuntime();
    [[nodiscard]] const std::vector<std::uint8_t>& flash() const noexcept;
    [[nodiscard]] std::size_t programCounter() const noexcept;
    [[nodiscard]] const std::array<std::uint8_t, 8>& registers() const noexcept;

private:
    void executeInstruction(const SimulationView& view);
    std::vector<std::uint8_t> flash_;
    std::array<std::uint8_t, 256> ram_{};
    std::array<std::uint8_t, 8> registers_{};
    std::array<std::uint8_t, 2> ports_{};
    std::size_t pc_ = 0;
    bool halted_ = false;
    double nextInstructionTime_ = 0.0;
};

class ExternalMemoryComponent final : public Component {
public:
    ExternalMemoryComponent(std::string id, std::string label);
    [[nodiscard]] std::string typeId() const override;
    [[nodiscard]] std::string displayName() const override;
    [[nodiscard]] std::string category() const override;
    [[nodiscard]] std::unique_ptr<Component> clone() const override;
    void resetRuntime() override;
    [[nodiscard]] std::vector<DriveRequest> evaluate(const SimulationView& view) override;

private:
    std::vector<std::uint8_t> storage_;
    LogicLevel previousWrite_ = LogicLevel::Low;
};

class LcdComponent final : public Component {
public:
    LcdComponent(std::string id, std::string label);
    [[nodiscard]] std::string typeId() const override;
    [[nodiscard]] std::string displayName() const override;
    [[nodiscard]] std::string category() const override;
    [[nodiscard]] std::unique_ptr<Component> clone() const override;
    void resetRuntime() override;
    [[nodiscard]] std::vector<DriveRequest> evaluate(const SimulationView& view) override;
    [[nodiscard]] std::array<std::string, 2> lines() const;

private:
    void handleCommand(std::uint8_t command);
    void writeCharacter(char character);
    std::array<char, 32> characters_{};
    std::size_t cursor_ = 0;
    LogicLevel previousEnable_ = LogicLevel::Low;
};

class KeypadComponent final : public Component {
public:
    KeypadComponent(std::string id, std::string label);
    [[nodiscard]] std::string typeId() const override;
    [[nodiscard]] std::string displayName() const override;
    [[nodiscard]] std::string category() const override;
    [[nodiscard]] std::unique_ptr<Component> clone() const override;
    [[nodiscard]] std::vector<DriveRequest> evaluate(const SimulationView& view) override;
};

class VoltmeterComponent final : public Component {
public:
    VoltmeterComponent(std::string id, std::string label);
    [[nodiscard]] std::string typeId() const override;
    [[nodiscard]] std::string displayName() const override;
    [[nodiscard]] std::string category() const override;
    [[nodiscard]] std::unique_ptr<Component> clone() const override;
    void acceptAnalogResult(const AnalogResultView& result, double dt) override;
};

class AmmeterComponent final : public Component {
public:
    AmmeterComponent(std::string id, std::string label);
    [[nodiscard]] std::string typeId() const override;
    [[nodiscard]] std::string displayName() const override;
    [[nodiscard]] std::string category() const override;
    [[nodiscard]] std::unique_ptr<Component> clone() const override;
    void stampAnalog(AnalogStampCollector& collector, double dt) override;
    void acceptAnalogResult(const AnalogResultView& result, double dt) override;
};

class ComponentFactory {
public:
    [[nodiscard]] static std::unique_ptr<Component>
    create(const std::string& typeId, const std::string& id,
           const std::string& label);

    [[nodiscard]] static std::vector<std::string> supportedTypes();
    [[nodiscard]] static std::string suggestedPrefix(const std::string& typeId);
};

} // namespace proteus
