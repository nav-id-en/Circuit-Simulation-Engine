#include "proteus/simulation/SimulationEngine.hpp"

#include "proteus/components/Components.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace proteus {
namespace {

std::string pinKey(const PinRef& pin) {
    return pin.componentId + ":" + pin.pinId;
}

std::vector<double> solveLinearSystem(std::vector<std::vector<double>> matrix,
                                      std::vector<double> rightHandSide) {
    const auto size = matrix.size();
    if (rightHandSide.size() != size) {
        throw std::invalid_argument("Linear-system dimensions do not match.");
    }

    for (std::size_t column = 0; column < size; ++column) {
        auto pivot = column;
        for (std::size_t row = column + 1; row < size; ++row) {
            if (std::abs(matrix[row][column])
                > std::abs(matrix[pivot][column])) {
                pivot = row;
            }
        }
        if (std::abs(matrix[pivot][column]) < 1.0e-18) {
            matrix[pivot][column] = 1.0e-12;
        }
        if (pivot != column) {
            std::swap(matrix[pivot], matrix[column]);
            std::swap(rightHandSide[pivot], rightHandSide[column]);
        }

        const auto diagonal = matrix[column][column];
        for (std::size_t row = column + 1; row < size; ++row) {
            const auto factor = matrix[row][column] / diagonal;
            if (std::abs(factor) < 1.0e-30) {
                continue;
            }
            matrix[row][column] = 0.0;
            for (std::size_t inner = column + 1; inner < size; ++inner) {
                matrix[row][inner] -= factor * matrix[column][inner];
            }
            rightHandSide[row] -= factor * rightHandSide[column];
        }
    }

    std::vector<double> solution(size, 0.0);
    for (std::size_t offset = 0; offset < size; ++offset) {
        const auto row = size - 1 - offset;
        auto value = rightHandSide[row];
        for (std::size_t column = row + 1; column < size; ++column) {
            value -= matrix[row][column] * solution[column];
        }
        const auto diagonal = matrix[row][row];
        solution[row] =
            std::abs(diagonal) < 1.0e-18 ? 0.0 : value / diagonal;
    }
    return solution;
}

bool isStrongOutput(PinDirection direction) {
    return direction == PinDirection::Output;
}

bool isLogicGateType(const std::string& type) {
    return type == "and_gate" || type == "or_gate"
        || type == "not_gate" || type == "xor_gate"
        || type == "nand_gate";
}

bool isSourcePositivePin(const Component& component,
                         const PinRef& pin) {
    return (component.typeId() == "dc_source"
            || component.typeId() == "battery")
        && pin.pinId == "P";
}

} // namespace

class SimulationEngine::DisjointSet {
public:
    void add(const std::string& value) {
        parent_.try_emplace(value, value);
        rank_.try_emplace(value, 0);
    }

    std::string find(const std::string& value) {
        const auto it = parent_.find(value);
        if (it == parent_.end()) {
            return {};
        }
        if (it->second != value) {
            it->second = find(it->second);
        }
        return it->second;
    }

    void unite(const std::string& left, const std::string& right) {
        auto leftRoot = find(left);
        auto rightRoot = find(right);
        if (leftRoot.empty() || rightRoot.empty() || leftRoot == rightRoot) {
            return;
        }
        if (rank_[leftRoot] < rank_[rightRoot]) {
            std::swap(leftRoot, rightRoot);
        }
        parent_[rightRoot] = leftRoot;
        if (rank_[leftRoot] == rank_[rightRoot]) {
            ++rank_[leftRoot];
        }
    }

private:
    std::map<std::string, std::string> parent_;
    std::map<std::string, int> rank_;
};

SimulationEngine::SimulationEngine(Circuit& circuit) : circuit_(circuit) {
    rebuildGraph();
}

SimulationState SimulationEngine::state() const noexcept {
    return state_;
}

double SimulationEngine::timeSeconds() const {
    return timeSeconds_;
}

double SimulationEngine::timeStepSeconds() const {
    return timeStepSeconds_;
}

void SimulationEngine::setTimeStepSeconds(double seconds) {
    timeStepSeconds_ = std::clamp(seconds, 1.0e-9, 1.0);
}

std::vector<DrcIssue> SimulationEngine::validateDesign() {
    rebuildGraphIfNeeded();
    std::vector<DrcIssue> issues;
    const auto hasDigitalDriver = [this](const PinRef& input) {
        const auto net = rootForPin(input);
        const auto netIt = netToPins_.find(net);
        if (netIt == netToPins_.end()) {
            return false;
        }
        return std::any_of(
            netIt->second.begin(), netIt->second.end(),
            [this, &input](const PinRef& candidate) {
                if (candidate == input) {
                    return false;
                }
                const auto* owner =
                    circuit_.component(candidate.componentId);
                const auto* definition =
                    owner ? owner->findPin(candidate.pinId) : nullptr;
                if (!owner || !definition) {
                    return false;
                }
                return definition->direction == PinDirection::Output
                    || definition->direction
                        == PinDirection::Bidirectional
                    || owner->typeId() == "ground"
                    || owner->typeId() == "dc_source"
                    || owner->typeId() == "battery";
            });
    };
    if (circuit_.components().empty()) {
        issues.push_back({IssueSeverity::Error, "EMPTY_CIRCUIT", "Circuit",
                          "The circuit contains no components."});
    }
    if (!hasGround()) {
        issues.push_back({IssueSeverity::Error, "NO_GROUND", "Circuit",
                          "At least one GND component is required."});
    }

    for (const auto* component : circuit_.components()) {
        if (const auto error = component->validationError()) {
            issues.push_back({IssueSeverity::Error, "INVALID_COMPONENT",
                              component->label(), *error});
        }
        for (const auto& pinDefinition : component->pins()) {
            auto requiredInput =
                pinDefinition.required
                && pinDefinition.direction == PinDirection::Input;
            if (isLogicGateType(component->typeId())
                && pinDefinition.direction == PinDirection::Input
                && pinDefinition.id.starts_with("IN")) {
                const auto index =
                    std::stoi(pinDefinition.id.substr(2));
                requiredInput = index
                    < component->property("inputCount", 0);
            }
            if (component->typeId() == "dac"
                && pinDefinition.id.starts_with("D")) {
                const auto index =
                    std::stoi(pinDefinition.id.substr(1));
                requiredInput = index
                    < component->property("bitCount", 8);
            }
            if (!requiredInput) {
                continue;
            }
            const auto ref = component->pinRef(pinDefinition.id);
            if (!circuit_.isPinConnected(ref)
                || (pinDefinition.domain == SignalDomain::Digital
                    && !hasDigitalDriver(ref))) {
                issues.push_back(
                    {IssueSeverity::Error, "FLOATING_INPUT",
                     component->label() + "." + pinDefinition.label,
                     "Floating input detected."});
            }
        }
    }

    for (const auto& [wireId, wire] : circuit_.wires()) {
        if (!circuit_.containsPin(wire.start)
            || !circuit_.containsPin(wire.end)) {
            issues.push_back({IssueSeverity::Error, "BROKEN_WIRE", wireId,
                              "A wire endpoint does not exist."});
        }
    }

    for (const auto& [netId, pins] : netToPins_) {
        std::vector<std::string> strongOutputs;
        for (const auto& pinRef : pins) {
            const auto* component = circuit_.component(pinRef.componentId);
            const auto* definition =
                component ? component->findPin(pinRef.pinId) : nullptr;
            if (definition
                && (isStrongOutput(definition->direction)
                    || isSourcePositivePin(*component, pinRef))) {
                strongOutputs.push_back(component->label() + "."
                                        + definition->label);
            }
        }
        if (strongOutputs.size() > 1) {
            std::ostringstream message;
            message << "Conflicting output drivers on " << netId << ": ";
            for (std::size_t index = 0; index < strongOutputs.size(); ++index) {
                if (index) {
                    message << ", ";
                }
                message << strongOutputs[index];
            }
            issues.push_back({IssueSeverity::Error, "OUTPUT_SHORT", netId,
                              message.str()});
        }
    }

    if (issues.empty()) {
        issues.push_back({IssueSeverity::Information, "DRC_OK", "Circuit",
                          "Design-rule check completed with no errors."});
    }
    return issues;
}

bool SimulationEngine::run() {
    if (state_ == SimulationState::Paused) {
        state_ = SimulationState::Running;
        addLog(IssueSeverity::Information, "Simulation",
               "Simulation resumed.");
        return true;
    }
    if (state_ == SimulationState::Running) {
        return true;
    }

    clearLog();
    const auto issues = validateDesign();
    for (const auto& issue : issues) {
        addLog(issue.severity, issue.source, issue.message);
    }
    const auto hasError = std::any_of(
        issues.begin(), issues.end(), [](const DrcIssue& issue) {
            return issue.severity == IssueSeverity::Error;
        });
    if (hasError) {
        addLog(IssueSeverity::Error, "Simulation",
               "Run blocked because DRC reported an error.");
        return false;
    }

    reset();
    state_ = SimulationState::Running;
    addLog(IssueSeverity::Information, "Simulation", "Simulation started.");
    return true;
}

void SimulationEngine::pause() {
    if (state_ == SimulationState::Running) {
        state_ = SimulationState::Paused;
        addLog(IssueSeverity::Information, "Simulation",
               "Simulation paused; pending events were preserved.");
    }
}

void SimulationEngine::stop() {
    if (state_ == SimulationState::Stopped) {
        return;
    }
    state_ = SimulationState::Stopped;
    reset();
    addLog(IssueSeverity::Information, "Simulation",
           "Simulation stopped and runtime state was reset.");
}

bool SimulationEngine::step() {
    if (state_ == SimulationState::Stopped) {
        clearLog();
        const auto issues = validateDesign();
        for (const auto& issue : issues) {
            addLog(issue.severity, issue.source, issue.message);
        }
        if (std::any_of(issues.begin(), issues.end(),
                        [](const DrcIssue& issue) {
                            return issue.severity == IssueSeverity::Error;
                        })) {
            addLog(IssueSeverity::Error, "Simulation",
                   "Step blocked because DRC reported an error.");
            return false;
        }
        reset();
        state_ = SimulationState::Paused;
    } else if (state_ == SimulationState::Running) {
        state_ = SimulationState::Paused;
    }
    performTick(timeStepSeconds_);
    addLog(IssueSeverity::Information, "Simulation", "Single step executed.");
    return true;
}

void SimulationEngine::advance(double realSeconds) {
    if (state_ != SimulationState::Running || realSeconds <= 0.0) {
        return;
    }
    accumulatedRealSeconds_ += std::min(realSeconds, 0.25);
    int tickCount = 0;
    while (accumulatedRealSeconds_ + 1.0e-15 >= timeStepSeconds_
           && tickCount++ < 10000) {
        performTick(timeStepSeconds_);
        accumulatedRealSeconds_ -= timeStepSeconds_;
    }
}

void SimulationEngine::reset() {
    timeSeconds_ = 0.0;
    accumulatedRealSeconds_ = 0.0;
    activeDrivers_.clear();
    requestedDrivers_.clear();
    netVoltages_.clear();
    netSignals_.clear();
    branchCurrents_.clear();
    scopeSamples_.clear();
    events_ = {};
    eventSequence_ = 0;
    for (auto* component : circuit_.components()) {
        component->resetRuntime();
    }
    rebuildGraph();
}

void SimulationEngine::restoreSavedState(SimulationState state,
                                         double timeSeconds,
                                         double timeStepSeconds) {
    rebuildGraph();
    timeSeconds_ = std::max(0.0, timeSeconds);
    timeStepSeconds_ = std::clamp(timeStepSeconds, 1.0e-9, 1.0);
    accumulatedRealSeconds_ = 0.0;
    activeDrivers_.clear();
    requestedDrivers_.clear();
    events_ = {};
    eventSequence_ = 0;
    state_ = state == SimulationState::Running ? SimulationState::Paused
                                               : state;
    evaluateDigitalComponents();
    processDueEvents();
    solveAnalog(timeStepSeconds_);
    resolveNetSignals();
    addLog(IssueSeverity::Information, "Persistence",
           state == SimulationState::Running
               ? "Saved running state restored in Pause mode."
               : "Saved simulation state restored.");
}

Signal SimulationEngine::signalAt(const PinRef& pin) const {
    const auto net = rootForPin(pin);
    const auto it = netSignals_.find(net);
    return it == netSignals_.end() ? Signal::undefined() : it->second;
}

double SimulationEngine::voltageAt(const PinRef& pin) const {
    const auto net = rootForPin(pin);
    if (groundNets_.contains(net)) {
        return 0.0;
    }
    const auto it = netVoltages_.find(net);
    return it == netVoltages_.end()
        ? std::numeric_limits<double>::quiet_NaN()
        : it->second;
}

double SimulationEngine::branchCurrent(const std::string& branchKey) const {
    const auto it = branchCurrents_.find(branchKey);
    return it == branchCurrents_.end() ? 0.0 : it->second;
}

std::optional<std::string>
SimulationEngine::netIdAt(const PinRef& pin) const {
    const auto it = pinToNet_.find(pin);
    if (it == pinToNet_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<std::string> SimulationEngine::netIds() const {
    std::vector<std::string> result;
    result.reserve(netToPins_.size());
    for (const auto& [netId, pins] : netToPins_) {
        static_cast<void>(pins);
        result.push_back(netId);
    }
    return result;
}

double SimulationEngine::netVoltage(const std::string& netId) const {
    if (groundNets_.contains(netId)) {
        return 0.0;
    }
    const auto it = netVoltages_.find(netId);
    return it == netVoltages_.end()
        ? std::numeric_limits<double>::quiet_NaN()
        : it->second;
}

Signal SimulationEngine::netSignal(const std::string& netId) const {
    const auto it = netSignals_.find(netId);
    return it == netSignals_.end() ? Signal::undefined() : it->second;
}

void SimulationEngine::setScopeChannels(
    std::map<std::string, PinRef> channels) {
    scopeChannels_ = std::move(channels);
    clearScope();
}

const std::vector<ScopeSample>&
SimulationEngine::scopeSamples() const noexcept {
    return scopeSamples_;
}

void SimulationEngine::clearScope() {
    scopeSamples_.clear();
}

const std::vector<LogEntry>&
SimulationEngine::logEntries() const noexcept {
    return logEntries_;
}

void SimulationEngine::clearLog() {
    logEntries_.clear();
}

void SimulationEngine::addLog(IssueSeverity severity, std::string source,
                              std::string message) {
    if (!logEntries_.empty()) {
        const auto& previous = logEntries_.back();
        if (previous.severity == severity && previous.source == source
            && previous.message == message
            && std::abs(previous.timeSeconds - timeSeconds_) < 0.1) {
            return;
        }
    }
    logEntries_.push_back(
        {timeSeconds_, severity, std::move(source), std::move(message)});
    if (logEntries_.size() > 5000) {
        logEntries_.erase(logEntries_.begin(),
                          logEntries_.begin() + 1000);
    }
}

void SimulationEngine::rebuildGraphIfNeeded() {
    if (graphRevision_ != circuit_.revision()) {
        rebuildGraph();
    }
}

void SimulationEngine::rebuildGraph() {
    DisjointSet sets;
    std::map<std::string, PinRef> keyToPin;
    for (const auto* component : circuit_.components()) {
        for (const auto& definition : component->pins()) {
            const auto ref = component->pinRef(definition.id);
            const auto key = pinKey(ref);
            sets.add(key);
            keyToPin.emplace(key, ref);
        }
    }

    for (const auto& [wireId, wire] : circuit_.wires()) {
        static_cast<void>(wireId);
        sets.unite(pinKey(wire.start), pinKey(wire.end));
    }

    for (const auto& [junctionId, junction] : circuit_.junctions()) {
        static_cast<void>(junctionId);
        std::optional<PinRef> firstPin;
        for (const auto& wireId : junction.wireIds) {
            const auto* wire = circuit_.wire(wireId);
            if (!wire) {
                continue;
            }
            if (!firstPin) {
                firstPin = wire->start;
            } else {
                sets.unite(pinKey(*firstPin), pinKey(wire->start));
            }
        }
    }

    pinToNet_.clear();
    netToPins_.clear();
    groundNets_.clear();
    std::map<std::string, std::string> rootToNet;
    int netNumber = 1;
    for (const auto& [key, ref] : keyToPin) {
        const auto root = sets.find(key);
        auto [it, inserted] = rootToNet.emplace(
            root, "NET_" + std::to_string(netNumber));
        if (inserted) {
            ++netNumber;
        }
        pinToNet_[ref] = it->second;
        netToPins_[it->second].push_back(ref);
    }

    for (const auto* component : circuit_.components()) {
        if (component->typeId() == "ground") {
            const auto it = pinToNet_.find(component->pinRef("G"));
            if (it != pinToNet_.end()) {
                groundNets_.insert(it->second);
            }
        }
    }

    netToNode_.clear();
    int nodeIndex = 0;
    for (const auto& [netId, pins] : netToPins_) {
        static_cast<void>(pins);
        if (!groundNets_.contains(netId)) {
            netToNode_[netId] = nodeIndex++;
        }
    }
    graphRevision_ = circuit_.revision();
}

void SimulationEngine::performTick(double dt) {
    rebuildGraphIfNeeded();
    timeSeconds_ += dt;
    processDueEvents();

    for (int iteration = 0; iteration < 3; ++iteration) {
        evaluateDigitalComponents();
        processDueEvents();
        solveAnalog(dt);
        resolveNetSignals();
    }
    sampleScope();
}

void SimulationEngine::evaluateDigitalComponents() {
    for (auto* component : circuit_.components()) {
        const auto requests = component->evaluate(*this);
        for (const auto& request : requests) {
            scheduleDrive(component->pinRef(request.pinId), request.signal,
                          request.delaySeconds);
        }
    }
}

void SimulationEngine::scheduleDrive(const PinRef& pin,
                                     const Signal& signal, double delay) {
    const auto requested = requestedDrivers_.find(pin);
    if (requested != requestedDrivers_.end()
        && requested->second.approximatelyEquals(signal)) {
        return;
    }
    requestedDrivers_[pin] = signal;
    if (delay <= 0.0) {
        activeDrivers_[pin] = signal;
        return;
    }
    events_.push(
        {timeSeconds_ + delay, eventSequence_++, pin, signal});
}

void SimulationEngine::processDueEvents() {
    while (!events_.empty()
           && events_.top().dueTime <= timeSeconds_ + 1.0e-15) {
        const auto event = events_.top();
        events_.pop();
        activeDrivers_[event.pin] = event.signal;
    }
}

void SimulationEngine::solveAnalog(double dt) {
    conductanceStamps_.clear();
    currentStamps_.clear();
    voltageSourceStamps_.clear();
    branchCurrents_.clear();
    for (auto* component : circuit_.components()) {
        component->stampAnalog(*this, dt);
    }

    const auto nodeCount = static_cast<int>(netToNode_.size());
    const auto sourceCount = static_cast<int>(voltageSourceStamps_.size());
    const auto size = nodeCount + sourceCount;
    if (size == 0) {
        return;
    }

    std::vector<std::vector<double>> matrix(
        static_cast<std::size_t>(size),
        std::vector<double>(static_cast<std::size_t>(size), 0.0));
    std::vector<double> rhs(static_cast<std::size_t>(size), 0.0);
    for (int node = 0; node < nodeCount; ++node) {
        matrix[node][node] += 1.0e-12;
    }

    const auto addConductanceToMatrix =
        [this, &matrix](const PinRef& first, const PinRef& second,
                        double conductance) {
            const auto firstNode = nodeIndexForPin(first);
            const auto secondNode = nodeIndexForPin(second);
            if (firstNode >= 0) {
                matrix[firstNode][firstNode] += conductance;
            }
            if (secondNode >= 0) {
                matrix[secondNode][secondNode] += conductance;
            }
            if (firstNode >= 0 && secondNode >= 0) {
                matrix[firstNode][secondNode] -= conductance;
                matrix[secondNode][firstNode] -= conductance;
            }
        };

    for (const auto& stamp : conductanceStamps_) {
        addConductanceToMatrix(stamp.first, stamp.second, stamp.siemens);
    }
    for (const auto& stamp : currentStamps_) {
        const auto from = nodeIndexForPin(stamp.from);
        const auto to = nodeIndexForPin(stamp.to);
        if (from >= 0) {
            rhs[from] -= stamp.amperes;
        }
        if (to >= 0) {
            rhs[to] += stamp.amperes;
        }
    }

    int sourceIndex = 0;
    for (const auto& source : voltageSourceStamps_) {
        const auto row = nodeCount + sourceIndex++;
        const auto positive = nodeIndexForPin(source.positive);
        const auto negative = nodeIndexForPin(source.negative);
        if (positive >= 0) {
            matrix[positive][row] += 1.0;
            matrix[row][positive] += 1.0;
        }
        if (negative >= 0) {
            matrix[negative][row] -= 1.0;
            matrix[row][negative] -= 1.0;
        }
        rhs[row] = source.volts;
    }

    constexpr double driverConductance = 1.0e9;
    for (const auto& [pin, signal] : activeDrivers_) {
        if (!signal.hasVoltage()) {
            continue;
        }
        const auto node = nodeIndexForPin(pin);
        if (node >= 0) {
            matrix[node][node] += driverConductance;
            rhs[node] += driverConductance * signal.voltage;
        }
    }

    std::vector<double> solution;
    try {
        solution = solveLinearSystem(std::move(matrix), std::move(rhs));
    } catch (const std::exception& error) {
        addLog(IssueSeverity::Error, "Analog solver", error.what());
        return;
    }

    for (const auto& [netId, node] : netToNode_) {
        netVoltages_[netId] = solution[static_cast<std::size_t>(node)];
    }
    for (const auto& ground : groundNets_) {
        netVoltages_[ground] = 0.0;
    }
    for (int index = 0; index < sourceCount; ++index) {
        branchCurrents_[voltageSourceStamps_[index].branchKey] =
            solution[static_cast<std::size_t>(nodeCount + index)];
    }
    for (auto* component : circuit_.components()) {
        component->acceptAnalogResult(*this, dt);
    }
}

void SimulationEngine::resolveNetSignals() {
    for (const auto& [netId, pins] : netToPins_) {
        static_cast<void>(pins);
        const auto voltage = netVoltage(netId);
        auto signal = Signal::analog(
            voltage, circuit_.logicLowMaximum(),
            circuit_.logicHighMinimum());
        bool hasDriver = false;
        bool conflict = false;
        std::optional<Signal> first;
        for (const auto& [pin, driver] : activeDrivers_) {
            if (rootForPin(pin) != netId) {
                continue;
            }
            hasDriver = true;
            if (!first) {
                first = driver;
            } else if (!first->approximatelyEquals(driver, 0.25)) {
                conflict = true;
            }
        }
        if (conflict) {
            signal = Signal::undefined();
            addLog(IssueSeverity::Error, netId,
                   "Conflicting sources drive this net.");
        } else if (hasDriver && first && !first->hasVoltage()) {
            signal = Signal::undefined();
        }
        netSignals_[netId] = signal;
    }
}

void SimulationEngine::sampleScope() {
    if (scopeChannels_.empty()) {
        return;
    }
    ScopeSample sample;
    sample.timeSeconds = timeSeconds_;
    for (const auto& [name, pin] : scopeChannels_) {
        sample.channelVoltages[name] = voltageAt(pin);
    }
    scopeSamples_.push_back(std::move(sample));
    if (scopeSamples_.size() > 10000) {
        scopeSamples_.erase(scopeSamples_.begin(),
                            scopeSamples_.begin() + 2000);
    }
}

std::string SimulationEngine::rootForPin(const PinRef& pin) const {
    const auto it = pinToNet_.find(pin);
    return it == pinToNet_.end() ? std::string{} : it->second;
}

int SimulationEngine::nodeIndexForPin(const PinRef& pin) const {
    const auto net = rootForPin(pin);
    if (net.empty() || groundNets_.contains(net)) {
        return -1;
    }
    const auto it = netToNode_.find(net);
    return it == netToNode_.end() ? -1 : it->second;
}

bool SimulationEngine::hasGround() const {
    return !groundNets_.empty();
}

std::vector<PinRef>
SimulationEngine::pinsOnNet(const std::string& netId) const {
    const auto it = netToPins_.find(netId);
    return it == netToPins_.end() ? std::vector<PinRef>{} : it->second;
}

void SimulationEngine::addConductance(const ConductanceStamp& stamp) {
    if (std::isfinite(stamp.siemens) && stamp.siemens >= 0.0) {
        conductanceStamps_.push_back(stamp);
    }
}

void SimulationEngine::addCurrent(const CurrentStamp& stamp) {
    if (std::isfinite(stamp.amperes)) {
        currentStamps_.push_back(stamp);
    }
}

void SimulationEngine::addVoltageSource(const VoltageSourceStamp& stamp) {
    if (std::isfinite(stamp.volts)) {
        voltageSourceStamps_.push_back(stamp);
    }
}

void SimulationEngine::markGround(const PinRef& pin) {
    const auto net = rootForPin(pin);
    if (!net.empty()) {
        groundNets_.insert(net);
    }
}

void SimulationEngine::logWarning(const std::string& source,
                                  const std::string& message) const {
    const_cast<SimulationEngine*>(this)->addLog(
        IssueSeverity::Warning, source, message);
}

} // namespace proteus
