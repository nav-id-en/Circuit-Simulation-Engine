#include "proteus/components/Components.hpp"
#include "proteus/core/Circuit.hpp"
#include "proteus/core/Signal.hpp"
#include "proteus/persistence/CircuitSerializer.hpp"
#include "proteus/simulation/FirmwareLoader.hpp"
#include "proteus/simulation/SimulationEngine.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using namespace proteus;

class FakeSimulationView final : public SimulationView {
public:
    double now = 0.0;
    double step = 0.001;
    std::map<PinRef, Signal> signals;
    std::map<PinRef, double> voltages;
    mutable std::vector<std::string> warnings;

    [[nodiscard]] double timeSeconds() const override { return now; }
    [[nodiscard]] double timeStepSeconds() const override { return step; }

    [[nodiscard]] Signal signalAt(const PinRef& pin) const override {
        const auto it = signals.find(pin);
        return it == signals.end() ? Signal::undefined() : it->second;
    }

    [[nodiscard]] double voltageAt(const PinRef& pin) const override {
        const auto it = voltages.find(pin);
        return it == voltages.end()
            ? std::numeric_limits<double>::quiet_NaN()
            : it->second;
    }

    void logWarning(const std::string& source,
                    const std::string& message) const override {
        warnings.push_back(source + ": " + message);
    }
};

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void requireNear(double actual, double expected, double tolerance,
                 const std::string& message) {
    if (!std::isfinite(actual)
        || std::abs(actual - expected) > tolerance) {
        throw std::runtime_error(
            message + " (expected " + std::to_string(expected)
            + ", got " + std::to_string(actual) + ")");
    }
}

void testSignalClassification() {
    require(Signal::analog(0.4).logic == LogicLevel::Low,
            "0.4 V must classify as LOW.");
    require(Signal::analog(4.2).logic == LogicLevel::High,
            "4.2 V must classify as HIGH.");
    require(Signal::analog(2.5).logic == LogicLevel::Undefined,
            "Threshold-band voltage must be undefined.");
    Circuit circuit;
    circuit.setLogicThresholds(0.8, 2.0);
    requireNear(circuit.logicLowMaximum(), 0.8, 1.0e-12,
                "Global LOW threshold was not stored.");
    requireNear(circuit.logicHighMinimum(), 2.0, 1.0e-12,
                "Global HIGH threshold was not stored.");
}

void testFirmwareParser() {
    const auto image = FirmwareLoader::parseIntelHex(
        ":0600000001002A0700BD0B\n"
        ":00000001FF\n");
    require(image.bytes.size() == 6, "HEX byte count was not preserved.");
    require(image.bytes[0] == 0x01 && image.bytes[2] == 0x2A
                && image.bytes[3] == 0x07,
            "HEX payload was decoded incorrectly.");
    const auto sampleFirmware = FirmwareLoader::parseIntelHex(
        ":07000000010055070000FF9D\n"
        ":00000001FF\n");
    require(sampleFirmware.bytes.size() == 7
                && sampleFirmware.bytes.back() == 0xFF,
            "Bundled MCU sample firmware is invalid.");

    bool rejected = false;
    try {
        static_cast<void>(FirmwareLoader::parseIntelHex(
            ":0100000001FF\n:00000001FF\n"));
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    require(rejected, "Invalid Intel HEX checksum must be rejected.");
}

void testAdcAndDac() {
    FakeSimulationView view;
    AdcComponent adc("adc", "ADC1", 4);
    view.voltages[adc.pinRef("VIN")] = 2.5;
    view.voltages[adc.pinRef("VREFP")] = 5.0;
    view.voltages[adc.pinRef("VREFN")] = 0.0;
    const auto adcResult = adc.evaluate(view);
    require(adcResult.size() == 4, "4-bit ADC must expose four outputs.");
    require(adc.runtimeValue("code", -1) == 8,
            "2.5 V on a 0..5 V 4-bit ADC must round to code 8.");

    DacComponent dac("dac", "DAC1", 4);
    for (int bit = 0; bit < 4; ++bit) {
        view.signals[dac.pinRef("D" + std::to_string(bit))] =
            (8U & (1U << bit)) ? Signal::high() : Signal::low();
    }
    view.voltages[dac.pinRef("VREFP")] = 5.0;
    view.voltages[dac.pinRef("VREFN")] = 0.0;
    const auto dacResult = dac.evaluate(view);
    require(dacResult.size() == 1, "DAC must produce one analog output.");
    requireNear(dacResult.front().signal.voltage, 8.0 * 5.0 / 15.0,
                1.0e-9, "DAC linear mapping is incorrect.");
}

void testMcuInstructionSet() {
    FakeSimulationView view;
    MicrocontrollerComponent mcu("mcu", "MCU1");
    mcu.setProperty("clockHz", 1000.0);
    mcu.loadFlash({
        0x01, 0x00, 0x2A, // MOV R0, 42
        0x02, 0x00, 0x01, // ADD R0, 1
        0x07, 0x00, 0x00, // OUT PA, R0
        0xFF              // HALT
    });
    view.now = 0.01;
    const auto drives = mcu.evaluate(view);
    require(mcu.registers()[0] == 43,
            "MCU MOV/ADD instructions produced the wrong accumulator.");
    require(mcu.runtimeValue("portA", -1) == 43,
            "MCU OUT instruction did not update port A.");
    require(drives.size() == 16,
            "MCU must drive all exposed I/O pins.");
}

void testFlipFlopAndUndefinedLogic() {
    FakeSimulationView view;
    DFlipFlopComponent flipFlop("ff", "FF1");
    view.signals[flipFlop.pinRef("D")] = Signal::high();
    view.signals[flipFlop.pinRef("CLK")] = Signal::low();
    require(flipFlop.evaluate(view).front().signal.logic
                == LogicLevel::Low,
            "D flip-flop changed without a rising edge.");

    view.signals[flipFlop.pinRef("CLK")] = Signal::high();
    require(flipFlop.evaluate(view).front().signal.logic
                == LogicLevel::High,
            "D flip-flop did not latch D on a rising edge.");
    view.signals[flipFlop.pinRef("D")] = Signal::low();
    require(flipFlop.evaluate(view).front().signal.logic
                == LogicLevel::High,
            "D flip-flop must hold Q while CLK remains high.");

    LogicGateComponent gate("gate", "AND1", GateKind::And, 2);
    view.signals[gate.pinRef("IN0")] = Signal::high();
    view.signals[gate.pinRef("IN1")] = Signal::undefined();
    require(gate.evaluate(view).front().signal.logic
                == LogicLevel::Undefined,
            "Undefined logic must propagate through a gate.");
    require(!view.warnings.empty(),
            "Undefined gate input must create a warning.");
}

void testExternalMemoryBus() {
    FakeSimulationView view;
    ExternalMemoryComponent memory("mem", "MEM1");
    memory.setProperty("size", 1024);
    for (int bit = 0; bit < 16; ++bit) {
        view.signals[memory.pinRef("A" + std::to_string(bit))] =
            bit == 8 ? Signal::high() : Signal::low();
    }
    constexpr int value = 0xA5;
    for (int bit = 0; bit < 8; ++bit) {
        view.signals[memory.pinRef("D" + std::to_string(bit))] =
            value & (1 << bit) ? Signal::high() : Signal::low();
    }
    view.signals[memory.pinRef("RD")] = Signal::low();
    view.signals[memory.pinRef("WR")] = Signal::low();
    static_cast<void>(memory.evaluate(view));
    view.signals[memory.pinRef("WR")] = Signal::high();
    static_cast<void>(memory.evaluate(view));
    view.signals[memory.pinRef("WR")] = Signal::low();
    view.signals[memory.pinRef("RD")] = Signal::high();
    const auto result = memory.evaluate(view);
    require(result.size() == 8,
            "External memory read must drive an 8-bit data bus.");
    int decoded = 0;
    for (int bit = 0; bit < 8; ++bit) {
        if (result[static_cast<std::size_t>(bit)].signal.logic
            == LogicLevel::High) {
            decoded |= 1 << bit;
        }
    }
    require(decoded == value,
            "External memory did not preserve the addressed byte.");
}

void testAnalogSolverAndDrc() {
    Circuit circuit;
    auto ground = std::make_unique<GroundComponent>("gnd", "GND1");
    auto source = std::make_unique<DcSourceComponent>("source", "V1");
    source->setProperty("voltage", 5.0);
    auto resistor = std::make_unique<ResistorComponent>("resistor", "R1");
    resistor->setProperty("resistance", 1000.0);
    circuit.addComponent(std::move(ground));
    circuit.addComponent(std::move(source));
    circuit.addComponent(std::move(resistor));
    circuit.addWire({"w1", {"source", "N"}, {"gnd", "G"}, {}});
    circuit.addWire({"w2", {"source", "P"}, {"resistor", "A"}, {}});
    circuit.addWire({"w3", {"resistor", "B"}, {"gnd", "G"}, {}});

    SimulationEngine engine(circuit);
    const auto issues = engine.validateDesign();
    require(issues.size() == 1
                && issues.front().severity == IssueSeverity::Information,
            "Valid analog circuit should pass DRC.");
    require(engine.step(), "Valid analog circuit should execute a step.");
    requireNear(engine.voltageAt({"source", "P"}), 5.0, 1.0e-8,
                "Voltage-source MNA stamp is incorrect.");
    requireNear(std::abs(engine.branchCurrent("source:source")), 0.005,
                1.0e-7, "Ohm-law current is incorrect.");
}

void testDrcFloatingAndConflictingOutputs() {
    Circuit circuit;
    circuit.addComponent(
        std::make_unique<GroundComponent>("gnd", "GND1"));
    circuit.addComponent(std::make_unique<LogicGateComponent>(
        "gate1", "AND1", GateKind::And, 2));
    circuit.addComponent(
        std::make_unique<ClockComponent>("clock1", "CLK1"));
    circuit.addComponent(
        std::make_unique<ClockComponent>("clock2", "CLK2"));
    circuit.addWire({"w1", {"clock1", "OUT"}, {"clock2", "OUT"}, {}});
    circuit.addWire({"w2", {"clock1", "GND"}, {"gnd", "G"}, {}});

    SimulationEngine engine(circuit);
    const auto issues = engine.validateDesign();
    bool floating = false;
    bool shorted = false;
    for (const auto& issue : issues) {
        floating |= issue.code == "FLOATING_INPUT";
        shorted |= issue.code == "OUTPUT_SHORT";
    }
    require(floating, "DRC must detect floating logic inputs.");
    require(shorted, "DRC must detect conflicting output drivers.");
    require(!engine.run(), "DRC errors must block Run.");
}

void testDrcConflictingPowerSources() {
    Circuit circuit;
    circuit.addComponent(
        std::make_unique<GroundComponent>("gnd", "GND1"));
    circuit.addComponent(
        std::make_unique<DcSourceComponent>("v1", "V1"));
    circuit.addComponent(
        std::make_unique<DcSourceComponent>("v2", "V2"));
    circuit.addWire({"w1", {"v1", "N"}, {"gnd", "G"}, {}});
    circuit.addWire({"w2", {"v2", "N"}, {"gnd", "G"}, {}});
    circuit.addWire({"w3", {"v1", "P"}, {"v2", "P"}, {}});
    SimulationEngine engine(circuit);
    const auto issues = engine.validateDesign();
    require(std::any_of(
                issues.begin(), issues.end(),
                [](const DrcIssue& issue) {
                    return issue.code == "OUTPUT_SHORT";
                }),
            "DRC must detect two power-source positive pins on one net.");
}

void testDrcConnectedButUndrivenInput() {
    Circuit circuit;
    circuit.addComponent(
        std::make_unique<GroundComponent>("gnd", "GND1"));
    circuit.addComponent(std::make_unique<LogicGateComponent>(
        "gate", "AND1", GateKind::And, 2));
    circuit.addWire({"w1", {"gate", "IN0"}, {"gate", "IN1"}, {}});
    SimulationEngine engine(circuit);
    const auto issues = engine.validateDesign();
    require(std::any_of(
                issues.begin(), issues.end(),
                [](const DrcIssue& issue) {
                    return issue.code == "FLOATING_INPUT";
                }),
            "A wired net without a driver must still be considered floating.");
}

void testOwnershipCleanup() {
    Circuit circuit;
    circuit.addComponent(
        std::make_unique<GroundComponent>("gnd", "GND1"));
    circuit.addComponent(
        std::make_unique<ResistorComponent>("r", "R1"));
    circuit.addWire({"w", {"r", "A"}, {"gnd", "G"}, {}});
    require(circuit.removeComponent("r"),
            "Existing component must be removable.");
    require(circuit.wires().empty(),
            "Deleting a component must delete connected wires.");

    Circuit loadedStyleCircuit;
    loadedStyleCircuit.addComponent(
        std::make_unique<ResistorComponent>(
            "resistor_00000001", "R1"));
    require(loadedStyleCircuit.createId("resistor")
                != "resistor_00000001",
            "Generated IDs must not collide with loaded project IDs.");
}

void testPersistenceRoundTrip() {
    Circuit original;
    original.setProjectName("Round Trip Test");
    auto ground =
        std::make_unique<GroundComponent>("gnd", "GND1");
    ground->setPosition({320.0, 420.0});
    auto source =
        std::make_unique<DcSourceComponent>("source", "V1");
    source->setPosition({120.0, 180.0});
    source->setProperty("voltage", 3.3);
    auto mcu =
        std::make_unique<MicrocontrollerComponent>("mcu", "MCU1");
    mcu->setPosition({520.0, 180.0});
    mcu->loadFlash({0x01, 0x00, 0x2A, 0xFF});
    original.addComponent(std::move(ground));
    original.addComponent(std::move(source));
    original.addComponent(std::move(mcu));
    original.addWire(
        {"w1", {"source", "N"}, {"gnd", "G"}, {{120.0, 420.0}}});
    original.addWire(
        {"w2", {"source", "P"}, {"mcu", "VCC"}, {{320.0, 180.0}}});

    SimulationEngine originalEngine(original);
    originalEngine.setTimeStepSeconds(0.0005);
    const auto text = CircuitSerializer::toString(
        original, &originalEngine, true, false);

    Circuit restored;
    const auto saved =
        CircuitSerializer::fromString(text, restored);
    require(restored.projectName() == "Round Trip Test",
            "Project name was not preserved by persistence.");
    require(restored.components().size() == 3
                && restored.wires().size() == 2,
            "Component or wire count changed after persistence.");
    requireNear(restored.component("source")->property("voltage", 0.0),
                3.3, 1.0e-12,
                "Component property changed after persistence.");
    const auto* restoredMcu =
        dynamic_cast<const MicrocontrollerComponent*>(
            restored.component("mcu"));
    require(restoredMcu && restoredMcu->flash().size() == 4
                && restoredMcu->flash()[2] == 0x2A,
            "MCU flash image was not preserved.");
    requireNear(saved.timeStepSeconds, 0.0005, 1.0e-12,
                "Simulation time step was not preserved.");

    bool malformedRejected = false;
    try {
        Circuit invalid;
        static_cast<void>(
            CircuitSerializer::fromString("{not-json", invalid));
    } catch (const std::runtime_error&) {
        malformedRejected = true;
    }
    require(malformedRejected, "Malformed JSON must be rejected.");
}

} // namespace

int main() {
    const std::vector<std::pair<std::string, void (*)()>> tests = {
        {"signal classification", testSignalClassification},
        {"Intel HEX parser", testFirmwareParser},
        {"ADC and DAC", testAdcAndDac},
        {"MCU instruction set", testMcuInstructionSet},
        {"flip-flop and undefined logic", testFlipFlopAndUndefinedLogic},
        {"external memory bus", testExternalMemoryBus},
        {"analog solver and DRC", testAnalogSolverAndDrc},
        {"DRC errors", testDrcFloatingAndConflictingOutputs},
        {"power-source conflict", testDrcConflictingPowerSources},
        {"undriven connected input", testDrcConnectedButUndrivenInput},
        {"ownership cleanup", testOwnershipCleanup},
        {"persistence round trip", testPersistenceRoundTrip},
    };

    int failures = 0;
    for (const auto& [name, test] : tests) {
        try {
            test();
            std::cout << "[PASS] " << name << '\n';
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "[FAIL] " << name << ": " << error.what() << '\n';
        }
    }
    std::cout << tests.size() - static_cast<std::size_t>(failures)
              << "/" << tests.size() << " tests passed.\n";
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
