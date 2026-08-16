#include "proteus/ui/StudioApp.hpp"

#include "proteus/persistence/CircuitSerializer.hpp"
#include "proteus/simulation/FirmwareLoader.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <type_traits>

namespace proteus::ui {

void StudioApp::newProject(const std::string& name) {
    circuit_.clear();
    circuit_.setProjectName(name.empty() ? "Untitled" : name);
    currentPath_.clear();
    probeChannels_.clear();
    clearSelection();
    wireStart_.reset();
    rebuildEngine();
    resetHistory();
    resetCamera();
    screen_ = Screen::Editor;
    documentModified_ = false;
    SDL_SetWindowTitle(
        window_, ("ProteusLab SDL - " + circuit_.projectName()).c_str());
    setToast("NEW SDL PROJECT CREATED");
}

bool StudioApp::openProject(const std::string& path) {
    try {
        const auto saved =
            CircuitSerializer::loadFromFile(path, circuit_);
        probeChannels_.clear();
        clearSelection();
        wireStart_.reset();
        rebuildEngine();
        engine_->restoreSavedState(saved.state, saved.timeSeconds,
                                   saved.timeStepSeconds);
        currentPath_ = path;
        screen_ = Screen::Editor;
        documentModified_ = false;
        resetCamera();
        const auto size = circuit_.canvasSize();
        camera_.zoom =
            std::clamp(std::min(canvasRect_.w / (size.x + 80.0),
                                canvasRect_.h / (size.y + 80.0)),
                       0.25, 1.4);
        camera_.x = -40.0;
        camera_.y = -40.0;
        resetHistory();
        addRecentProject(path);
        SDL_SetWindowTitle(
            window_, ("ProteusLab SDL - " + circuit_.projectName()).c_str());
        setToast("PROJECT OPENED: " + path);
        return true;
    } catch (const std::exception& error) {
        setToast(error.what(), colors::error, 5.0);
        if (window_) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                     "Cannot open project", error.what(),
                                     window_);
        }
        return false;
    }
}

bool StudioApp::saveProject(const std::string& path) {
    try {
        auto actualPath = path;
        if (actualPath.empty()) {
            throw std::runtime_error("Project path cannot be empty.");
        }
        const auto extension =
            std::filesystem::path(actualPath).extension().string();
        if (extension.empty()) actualPath += ".oopproteus.json";
        CircuitSerializer::saveToFile(actualPath, circuit_, engine_.get(),
                                      saveRuntimeState_);
        currentPath_ = actualPath;
        documentModified_ = false;
        addRecentProject(actualPath);
        SDL_SetWindowTitle(
            window_, ("ProteusLab SDL - " + circuit_.projectName()).c_str());
        setToast("PROJECT SAVED: " + actualPath);
        return true;
    } catch (const std::exception& error) {
        setToast(error.what(), colors::error, 5.0);
        return false;
    }
}

void StudioApp::addRecentProject(const std::string& path) {
    if (path.empty()) return;
    std::erase(recentProjects_, path);
    recentProjects_.insert(recentProjects_.begin(), path);
    if (recentProjects_.size() > 10) recentProjects_.resize(10);
    saveRecentProjects();
}

void StudioApp::loadRecentProjects() {
    recentProjects_.clear();
    std::ifstream input(".proteuslab_recent.txt");
    std::string lineValue;
    while (std::getline(input, lineValue)) {
        if (!lineValue.empty()) recentProjects_.push_back(lineValue);
        if (recentProjects_.size() >= 10) break;
    }
}

void StudioApp::saveRecentProjects() const {
    std::ofstream output(".proteuslab_recent.txt", std::ios::trunc);
    if (!output) return;
    for (const auto& path : recentProjects_) output << path << '\n';
}

bool StudioApp::exportImage(const std::string& path) {
    auto actualPath = path;
    if (actualPath.empty()) {
        actualPath = "exports/" + circuit_.projectName() + "_"
            + timeStampForFile() + ".png";
    }
    const auto extension =
        std::filesystem::path(actualPath).extension().string();
    if (extension != ".png" && extension != ".PNG") {
        actualPath += ".png";
    }
    render();
    if (!saveRendererPngRegion(renderer_, actualPath, canvasRect_)) {
        setToast("PNG EXPORT FAILED", colors::error);
        return false;
    }
    setToast("PNG EXPORTED: " + actualPath);
    return true;
}

void StudioApp::runDrc() {
    engine_->clearLog();
    const auto issues = engine_->validateDesign();
    int errors = 0;
    int warnings = 0;
    for (const auto& issue : issues) {
        engine_->addLog(issue.severity, issue.source,
                        issue.code + ": " + issue.message);
        errors += issue.severity == IssueSeverity::Error ? 1 : 0;
        warnings += issue.severity == IssueSeverity::Warning ? 1 : 0;
    }
    showLog_ = true;
    updateLayout();
    if (errors > 0) {
        setToast("DRC FAILED: " + std::to_string(errors) + " ERROR(S)",
                 colors::error, 4.0);
    } else if (warnings > 0) {
        setToast("DRC PASSED WITH WARNINGS", colors::warning);
    } else {
        setToast("DRC PASSED - NO ERRORS", colors::accent);
    }
}

void StudioApp::runSimulation() {
    if (engine_->run()) {
        setToast("SIMULATION RUNNING");
    } else {
        showLog_ = true;
        updateLayout();
        setToast("RUN BLOCKED BY DRC", colors::error, 4.0);
    }
}

void StudioApp::pauseSimulation() {
    engine_->pause();
    setToast("SIMULATION PAUSED", colors::warning);
}

void StudioApp::stopSimulation() {
    engine_->stop();
    showScope_ = false;
    runtimePressedComponent_.clear();
    setToast("SIMULATION STOPPED", colors::muted);
}

void StudioApp::stepSimulation() {
    if (engine_->step()) {
        setToast("ONE SIMULATION STEP EXECUTED");
    } else {
        showLog_ = true;
        updateLayout();
        setToast("STEP BLOCKED BY DRC", colors::error);
    }
}

void StudioApp::toggleScope() {
    showScope_ = !showScope_;
    if (showScope_ && probeChannels_.empty()) {
        setToast("SELECT PROBE TOOL, THEN CLICK A PIN", colors::warning, 4.0);
    }
}

void StudioApp::addComponent(const std::string& type, Point position) {
    if (engine_->state() != SimulationState::Stopped) {
        setToast("STOP SIMULATION BEFORE EDITING", colors::warning);
        return;
    }
    try {
        const auto id = circuit_.createId(type);
        const auto prefix = ComponentFactory::suggestedPrefix(type);
        const auto label = circuit_.nextComponentLabel(prefix);
        auto component = ComponentFactory::create(type, id, label);
        component->setPosition(snap(position));
        circuit_.addComponent(std::move(component));
        if (std::find(activeTypes_.begin(), activeTypes_.end(), type)
            == activeTypes_.end()) {
            activeTypes_.push_back(type);
        }
        selectOnly(id);
        markModified("Add " + type);
    } catch (const std::exception& error) {
        setToast(error.what(), colors::error);
    }
}

void StudioApp::beginWire(const PinRef& pin) {
    if (engine_->state() != SimulationState::Stopped) return;
    wireStart_ = pin;
    setToast("WIRE STARTED - CLICK A SECOND PIN", colors::warning, 1.5);
}

void StudioApp::finishWire(const PinRef& pin) {
    if (!wireStart_) {
        beginWire(pin);
        return;
    }
    if (*wireStart_ == pin) {
        wireStart_.reset();
        setToast("WIRE CANCELLED", colors::muted);
        return;
    }
    for (const auto& [id, wire] : circuit_.wires()) {
        static_cast<void>(id);
        if ((wire.start == *wireStart_ && wire.end == pin)
            || (wire.start == pin && wire.end == *wireStart_)) {
            wireStart_.reset();
            setToast("THESE PINS ARE ALREADY CONNECTED", colors::warning);
            return;
        }
    }
    const auto* startComponent =
        circuit_.component(wireStart_->componentId);
    const auto* endComponent = circuit_.component(pin.componentId);
    const auto* startDefinition =
        startComponent ? startComponent->findPin(wireStart_->pinId) : nullptr;
    const auto* endDefinition =
        endComponent ? endComponent->findPin(pin.pinId) : nullptr;
    if (!startComponent || !endComponent || !startDefinition
        || !endDefinition) {
        wireStart_.reset();
        setToast("INVALID WIRE ENDPOINT", colors::error);
        return;
    }

    Wire wire;
    wire.id = circuit_.createId("wire");
    wire.start = *wireStart_;
    wire.end = pin;
    const auto start = pinWorldPosition(*startComponent, *startDefinition);
    const auto end = pinWorldPosition(*endComponent, *endDefinition);
    if (std::abs(start.x - end.x) > 1.0e-8
        && std::abs(start.y - end.y) > 1.0e-8) {
        wire.waypoints.push_back(snap({start.x, end.y}));
    }
    try {
        circuit_.addWire(std::move(wire));
        wireStart_.reset();
        markModified("Add wire");
        setToast("90-DEGREE WIRE ADDED");
    } catch (const std::exception& error) {
        wireStart_.reset();
        setToast(error.what(), colors::error);
    }
}

void StudioApp::createJunction(Point point) {
    point = snap(point);
    const auto wires =
        wiresAtWorldPoint(point, 8.0 / std::max(0.25, camera_.zoom));
    if (wires.size() < 2) {
        setToast("JUNCTION NEEDS AT LEAST TWO WIRES", colors::warning);
        return;
    }
    try {
        circuit_.addJunction(
            {circuit_.createId("junction"), point, wires});
        markModified("Add junction");
        setToast("JUNCTION CREATED");
    } catch (const std::exception& error) {
        setToast(error.what(), colors::error);
    }
}

void StudioApp::addProbe(const PinRef& pin) {
    for (const auto& [channel, current] : probeChannels_) {
        if (current == pin) {
            setToast(channel + " ALREADY USES THIS PIN", colors::warning);
            showScope_ = true;
            return;
        }
    }
    const auto name = "CH" + std::to_string(probeChannels_.size() + 1);
    probeChannels_[name] = pin;
    engine_->setScopeChannels(probeChannels_);
    showScope_ = true;
    setToast(name + " ADDED TO OSCILLOSCOPE");
}

void StudioApp::deleteSelection() {
    if (engine_->state() != SimulationState::Stopped) {
        setToast("STOP SIMULATION BEFORE EDITING", colors::warning);
        return;
    }
    if (selectedComponents_.empty() && selectedWires_.empty()) return;
    for (const auto& id : selectedWires_) circuit_.removeWire(id);
    for (const auto& id : selectedComponents_) circuit_.removeComponent(id);
    clearSelection();
    markModified("Delete selection");
}

void StudioApp::rotateSelection() {
    if (engine_->state() != SimulationState::Stopped
        || selectedComponents_.empty()) {
        return;
    }
    for (const auto& id : selectedComponents_) {
        if (auto* component = circuit_.component(id)) {
            component->setRotationDegrees(component->rotationDegrees() + 90);
        }
    }
    circuit_.touch();
    markModified("Rotate selection");
}

void StudioApp::mirrorSelection(bool horizontal) {
    if (engine_->state() != SimulationState::Stopped
        || selectedComponents_.empty()) {
        return;
    }
    for (const auto& id : selectedComponents_) {
        if (auto* component = circuit_.component(id)) {
            if (horizontal) {
                component->setMirroredHorizontally(
                    !component->mirroredHorizontally());
            } else {
                component->setMirroredVertically(
                    !component->mirroredVertically());
            }
        }
    }
    circuit_.touch();
    markModified(horizontal ? "Mirror horizontal" : "Mirror vertical");
}

void StudioApp::selectOnly(const std::string& componentId) {
    selectedComponents_.clear();
    selectedWires_.clear();
    if (!componentId.empty()) selectedComponents_.insert(componentId);
}

void StudioApp::clearSelection() {
    selectedComponents_.clear();
    selectedWires_.clear();
}

std::string StudioApp::selectedComponentId() const {
    return selectedComponents_.size() == 1 ? *selectedComponents_.begin()
                                           : std::string{};
}

Component* StudioApp::selectedComponent() {
    return circuit_.component(selectedComponentId());
}

const Component* StudioApp::selectedComponent() const {
    return circuit_.component(selectedComponentId());
}

std::string StudioApp::propertyValueText(const PropertyValue& value) const {
    return std::visit(
        [](const auto& concrete) {
            using ValueType = std::decay_t<decltype(concrete)>;
            if constexpr (std::is_same_v<ValueType, double>) {
                std::ostringstream output;
                output << std::setprecision(8) << concrete;
                return output.str();
            } else if constexpr (std::is_same_v<ValueType, int>) {
                return std::to_string(concrete);
            } else if constexpr (std::is_same_v<ValueType, bool>) {
                return std::string(concrete ? "TRUE" : "FALSE");
            } else {
                return concrete;
            }
        },
        value);
}

void StudioApp::applyProperty(const std::string& key, PropertyKind kind,
                              const std::string& textValue) {
    auto* component = selectedComponent();
    if (!component) return;
    try {
        if (key == "__label__") {
            if (textValue.empty()) {
                throw std::runtime_error("Label cannot be empty.");
            }
            component->setLabel(textValue);
        } else {
            PropertyValue value;
            if (kind == PropertyKind::Number) {
                std::size_t used = 0;
                const auto parsed = std::stod(textValue, &used);
                if (used != textValue.size()) {
                    throw std::runtime_error("Invalid numeric value.");
                }
                value = parsed;
            } else if (kind == PropertyKind::Integer) {
                std::size_t used = 0;
                const auto parsed = std::stoi(textValue, &used);
                if (used != textValue.size()) {
                    throw std::runtime_error("Invalid integer value.");
                }
                value = parsed;
            } else if (kind == PropertyKind::Boolean) {
                const auto enabled =
                    textValue == "1" || textValue == "true"
                    || textValue == "TRUE" || textValue == "on";
                value = enabled;
            } else {
                value = textValue;
            }
            if (!component->setProperty(key, std::move(value))) {
                throw std::runtime_error("Property value has the wrong type.");
            }
            loadSelectedFirmwareIfNeeded(key);
        }
        circuit_.touch();
        markModified("Edit property");
    } catch (const std::exception& error) {
        setToast(error.what(), colors::error, 4.0);
    }
}

void StudioApp::loadSelectedFirmwareIfNeeded(const std::string& key) {
    if (key != "firmwarePath") return;
    auto* microcontroller =
        dynamic_cast<MicrocontrollerComponent*>(selectedComponent());
    if (!microcontroller) return;
    auto path =
        microcontroller->property("firmwarePath", std::string{});
    if (path.empty()) return;
    auto resolved = std::filesystem::path(path);
    if (resolved.is_relative() && !currentPath_.empty()) {
        const auto nextToProject =
            std::filesystem::path(currentPath_).parent_path() / resolved;
        if (std::filesystem::exists(nextToProject)) resolved = nextToProject;
    }
    const auto firmware =
        FirmwareLoader::loadIntelHexFile(resolved.string());
    microcontroller->loadFlash(firmware.bytes);
    setToast("FIRMWARE LOADED: "
             + std::to_string(firmware.bytes.size()) + " BYTES");
}

void StudioApp::handleRuntimePress(const std::string& componentId,
                                   bool pressed, int mouseX, int mouseY) {
    auto* component = circuit_.component(componentId);
    if (!component) return;
    const auto type = component->typeId();
    if (type == "switch" && pressed) {
        component->setProperty(
            "closed", !component->property("closed", false));
        circuit_.touch();
    } else if (type == "push_button") {
        component->setProperty("closed", pressed);
        circuit_.touch();
        runtimePressedComponent_ = pressed ? componentId : std::string{};
    } else if (type == "keypad_4x4" && pressed) {
        const auto rect = componentScreenRect(*component);
        const auto column =
            std::clamp((mouseX - rect.x) * 4 / std::max(1, rect.w), 0, 3);
        const auto row =
            std::clamp((mouseY - rect.y) * 4 / std::max(1, rect.h), 0, 3);
        component->setProperty("pressedKey", row * 4 + column);
        circuit_.touch();
        runtimePressedComponent_ = componentId;
    } else if (type == "keypad_4x4" && !pressed) {
        component->setProperty("pressedKey", -1);
        circuit_.touch();
        runtimePressedComponent_.clear();
    } else if (type == "potentiometer" && pressed) {
        auto wiper = component->property("wiper", 50.0);
        wiper += 5.0;
        if (wiper > 99.9) wiper = 0.1;
        component->setProperty("wiper", wiper);
        circuit_.touch();
    }
}

void StudioApp::resetHistory() {
    history_.snapshots = {
        CircuitSerializer::toString(circuit_, engine_.get(),
                                    saveRuntimeState_, false)};
    history_.descriptions = {"Initial state"};
    history_.index = 0;
}

void StudioApp::commitHistory(const std::string& description) {
    const auto snapshot =
        CircuitSerializer::toString(circuit_, engine_.get(),
                                    saveRuntimeState_, false);
    if (!history_.snapshots.empty()
        && history_.snapshots[history_.index] == snapshot) {
        return;
    }
    if (history_.index + 1 < history_.snapshots.size()) {
        history_.snapshots.erase(
            history_.snapshots.begin()
                + static_cast<std::ptrdiff_t>(history_.index + 1),
            history_.snapshots.end());
        history_.descriptions.erase(
            history_.descriptions.begin()
                + static_cast<std::ptrdiff_t>(history_.index + 1),
            history_.descriptions.end());
    }
    history_.snapshots.push_back(snapshot);
    history_.descriptions.push_back(description);
    history_.index = history_.snapshots.size() - 1;
    if (history_.snapshots.size() > 80) {
        history_.snapshots.erase(history_.snapshots.begin());
        history_.descriptions.erase(history_.descriptions.begin());
        --history_.index;
    }
}

void StudioApp::markModified(const std::string& description) {
    documentModified_ = true;
    commitHistory(description);
    SDL_SetWindowTitle(
        window_, ("ProteusLab SDL - " + circuit_.projectName() + " *").c_str());
}

void StudioApp::restoreHistorySnapshot(std::size_t index) {
    if (index >= history_.snapshots.size()) return;
    try {
        const auto saved = CircuitSerializer::fromString(
            history_.snapshots[index], circuit_);
        rebuildEngine();
        engine_->restoreSavedState(saved.state, saved.timeSeconds,
                                   saved.timeStepSeconds);
        clearSelection();
        wireStart_.reset();
        history_.index = index;
        documentModified_ = true;
    } catch (const std::exception& error) {
        setToast("UNDO/REDO FAILED: " + std::string(error.what()),
                 colors::error);
    }
}

void StudioApp::undo() {
    if (history_.index == 0) return;
    restoreHistorySnapshot(history_.index - 1);
    setToast("UNDO: " + history_.descriptions[history_.index + 1],
             colors::accentBlue);
}

void StudioApp::redo() {
    if (history_.index + 1 >= history_.snapshots.size()) return;
    restoreHistorySnapshot(history_.index + 1);
    setToast("REDO: " + history_.descriptions[history_.index],
             colors::accentBlue);
}

} // namespace proteus::ui
