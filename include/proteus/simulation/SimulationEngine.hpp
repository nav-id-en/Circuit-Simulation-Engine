#pragma once

#include "proteus/core/Circuit.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace proteus {

enum class SimulationState {
    Stopped,
    Running,
    Paused
};

enum class IssueSeverity {
    Information,
    Warning,
    Error
};

struct DrcIssue {
    IssueSeverity severity = IssueSeverity::Information;
    std::string code;
    std::string source;
    std::string message;
};

struct LogEntry {
    double timeSeconds = 0.0;
    IssueSeverity severity = IssueSeverity::Information;
    std::string source;
    std::string message;
};

struct ScopeSample {
    double timeSeconds = 0.0;
    std::map<std::string, double> channelVoltages;
};

class SimulationEngine final : private SimulationView,
                               private AnalogStampCollector,
                               private AnalogResultView {
public:
    explicit SimulationEngine(Circuit& circuit);

    [[nodiscard]] SimulationState state() const noexcept;
    [[nodiscard]] double timeSeconds() const override;
    [[nodiscard]] double timeStepSeconds() const override;
    void setTimeStepSeconds(double seconds);

    [[nodiscard]] std::vector<DrcIssue> validateDesign();
    [[nodiscard]] bool run();
    void pause();
    void stop();
    [[nodiscard]] bool step();
    void advance(double realSeconds);
    void reset();
    void restoreSavedState(SimulationState state, double timeSeconds,
                           double timeStepSeconds);

    [[nodiscard]] Signal signalAt(const PinRef& pin) const override;
    [[nodiscard]] double voltageAt(const PinRef& pin) const override;
    [[nodiscard]] double branchCurrent(const std::string& branchKey) const override;
    [[nodiscard]] std::optional<std::string> netIdAt(const PinRef& pin) const;
    [[nodiscard]] std::vector<std::string> netIds() const;
    [[nodiscard]] double netVoltage(const std::string& netId) const;
    [[nodiscard]] Signal netSignal(const std::string& netId) const;

    void setScopeChannels(std::map<std::string, PinRef> channels);
    [[nodiscard]] const std::vector<ScopeSample>& scopeSamples() const noexcept;
    void clearScope();

    [[nodiscard]] const std::vector<LogEntry>& logEntries() const noexcept;
    void clearLog();
    void addLog(IssueSeverity severity, std::string source,
                std::string message);

private:
    struct Event {
        double dueTime = 0.0;
        std::uint64_t sequence = 0;
        PinRef pin;
        Signal signal;
    };

    struct EventCompare {
        bool operator()(const Event& left, const Event& right) const {
            if (left.dueTime != right.dueTime) {
                return left.dueTime > right.dueTime;
            }
            return left.sequence > right.sequence;
        }
    };

    class DisjointSet;

    void rebuildGraphIfNeeded();
    void rebuildGraph();
    void performTick(double dt);
    void evaluateDigitalComponents();
    void scheduleDrive(const PinRef& pin, const Signal& signal, double delay);
    void processDueEvents();
    void solveAnalog(double dt);
    void resolveNetSignals();
    void sampleScope();
    [[nodiscard]] std::string rootForPin(const PinRef& pin) const;
    [[nodiscard]] int nodeIndexForPin(const PinRef& pin) const;
    [[nodiscard]] bool hasGround() const;
    [[nodiscard]] std::vector<PinRef> pinsOnNet(const std::string& netId) const;

    void addConductance(const ConductanceStamp& stamp) override;
    void addCurrent(const CurrentStamp& stamp) override;
    void addVoltageSource(const VoltageSourceStamp& stamp) override;
    void markGround(const PinRef& pin) override;
    void logWarning(const std::string& source,
                    const std::string& message) const override;

    Circuit& circuit_;
    SimulationState state_ = SimulationState::Stopped;
    double timeSeconds_ = 0.0;
    double timeStepSeconds_ = 0.001;
    double accumulatedRealSeconds_ = 0.0;
    std::uint64_t graphRevision_ = 0;
    std::uint64_t eventSequence_ = 0;

    std::map<PinRef, std::string> pinToNet_;
    std::map<std::string, std::vector<PinRef>> netToPins_;
    std::map<std::string, int> netToNode_;
    std::set<std::string> groundNets_;
    std::map<std::string, double> netVoltages_;
    std::map<std::string, Signal> netSignals_;
    std::map<PinRef, Signal> activeDrivers_;
    std::map<PinRef, Signal> requestedDrivers_;
    std::priority_queue<Event, std::vector<Event>, EventCompare> events_;

    std::vector<ConductanceStamp> conductanceStamps_;
    std::vector<CurrentStamp> currentStamps_;
    std::vector<VoltageSourceStamp> voltageSourceStamps_;
    std::map<std::string, double> branchCurrents_;

    std::map<std::string, PinRef> scopeChannels_;
    std::vector<ScopeSample> scopeSamples_;
    mutable std::vector<LogEntry> logEntries_;
};

} // namespace proteus
