#pragma once

#include "proteus/core/Circuit.hpp"
#include "proteus/simulation/SimulationEngine.hpp"

#include <string>

namespace proteus {

struct LoadedSimulationState {
    SimulationState state = SimulationState::Stopped;
    double timeSeconds = 0.0;
    double timeStepSeconds = 0.001;
};

class CircuitSerializer {
public:
    [[nodiscard]] static std::string toString(
        const Circuit& circuit, const SimulationEngine* engine = nullptr,
        bool includeRuntimeState = true, bool pretty = true);

    [[nodiscard]] static LoadedSimulationState fromString(
        const std::string& text, Circuit& circuit);

    static void saveToFile(const std::string& filePath,
                           const Circuit& circuit,
                           const SimulationEngine* engine = nullptr,
                           bool includeRuntimeState = true);

    [[nodiscard]] static LoadedSimulationState loadFromFile(
        const std::string& filePath, Circuit& circuit);
};

} // namespace proteus
