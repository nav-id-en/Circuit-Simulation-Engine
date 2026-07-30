#include "proteus/core/Circuit.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace proteus {

Circuit::Circuit() = default;

const std::string& Circuit::projectName() const noexcept {
    return projectName_;
}

void Circuit::setProjectName(std::string name) {
    projectName_ = std::move(name);
    touch();
}

Point Circuit::canvasSize() const noexcept {
    return canvasSize_;
}

void Circuit::setCanvasSize(Point size) {
    canvasSize_.x = std::max(400.0, size.x);
    canvasSize_.y = std::max(300.0, size.y);
    touch();
}

double Circuit::gridSize() const noexcept {
    return gridSize_;
}

void Circuit::setGridSize(double size) {
    gridSize_ = std::clamp(size, 5.0, 100.0);
    touch();
}

double Circuit::logicLowMaximum() const noexcept {
    return logicLowMaximum_;
}

double Circuit::logicHighMinimum() const noexcept {
    return logicHighMinimum_;
}

void Circuit::setLogicThresholds(double lowMaximum,
                                 double highMinimum) {
    if (!std::isfinite(lowMaximum) || !std::isfinite(highMinimum)
        || lowMaximum >= highMinimum) {
        throw std::invalid_argument(
            "LOW threshold must be smaller than HIGH threshold.");
    }
    logicLowMaximum_ = lowMaximum;
    logicHighMinimum_ = highMinimum;
    touch();
}

Component& Circuit::addComponent(std::unique_ptr<Component> componentValue) {
    if (!componentValue) {
        throw std::invalid_argument("Cannot add a null component.");
    }
    if (components_.contains(componentValue->id())) {
        throw std::invalid_argument("Duplicate component id: "
                                    + componentValue->id());
    }
    const auto id = componentValue->id();
    auto [it, inserted] = components_.emplace(id, std::move(componentValue));
    if (!inserted) {
        throw std::runtime_error("Failed to add component.");
    }
    touch();
    return *it->second;
}

bool Circuit::removeComponent(const std::string& componentId) {
    if (!components_.contains(componentId)) {
        return false;
    }

    std::vector<std::string> wiresToRemove;
    for (const auto& [wireId, wireValue] : wires_) {
        if (wireValue.start.componentId == componentId
            || wireValue.end.componentId == componentId) {
            wiresToRemove.push_back(wireId);
        }
    }
    for (const auto& wireId : wiresToRemove) {
        removeWire(wireId);
    }

    components_.erase(componentId);
    touch();
    return true;
}

Component* Circuit::component(const std::string& componentId) {
    const auto it = components_.find(componentId);
    return it == components_.end() ? nullptr : it->second.get();
}

const Component* Circuit::component(const std::string& componentId) const {
    const auto it = components_.find(componentId);
    return it == components_.end() ? nullptr : it->second.get();
}

std::vector<Component*> Circuit::components() {
    std::vector<Component*> result;
    result.reserve(components_.size());
    for (auto& [id, value] : components_) {
        static_cast<void>(id);
        result.push_back(value.get());
    }
    return result;
}

std::vector<const Component*> Circuit::components() const {
    std::vector<const Component*> result;
    result.reserve(components_.size());
    for (const auto& [id, value] : components_) {
        static_cast<void>(id);
        result.push_back(value.get());
    }
    return result;
}

Wire& Circuit::addWire(Wire wireValue) {
    if (wireValue.id.empty()) {
        wireValue.id = createId("wire");
    }
    if (!containsPin(wireValue.start) || !containsPin(wireValue.end)) {
        throw std::invalid_argument("Wire endpoint does not exist.");
    }
    if (wireValue.start == wireValue.end) {
        throw std::invalid_argument("A wire cannot connect a pin to itself.");
    }
    const auto id = wireValue.id;
    auto [it, inserted] = wires_.emplace(id, std::move(wireValue));
    if (!inserted) {
        throw std::invalid_argument("Duplicate wire id: " + id);
    }
    touch();
    return it->second;
}

bool Circuit::removeWire(const std::string& wireId) {
    if (!wires_.erase(wireId)) {
        return false;
    }
    std::vector<std::string> emptyJunctions;
    for (auto& [junctionId, junction] : junctions_) {
        std::erase(junction.wireIds, wireId);
        if (junction.wireIds.size() < 2) {
            emptyJunctions.push_back(junctionId);
        }
    }
    for (const auto& junctionId : emptyJunctions) {
        junctions_.erase(junctionId);
    }
    touch();
    return true;
}

Wire* Circuit::wire(const std::string& wireId) {
    const auto it = wires_.find(wireId);
    return it == wires_.end() ? nullptr : &it->second;
}

const Wire* Circuit::wire(const std::string& wireId) const {
    const auto it = wires_.find(wireId);
    return it == wires_.end() ? nullptr : &it->second;
}

const std::map<std::string, Wire>& Circuit::wires() const noexcept {
    return wires_;
}

Junction& Circuit::addJunction(Junction junction) {
    if (junction.id.empty()) {
        junction.id = createId("junction");
    }
    std::erase_if(junction.wireIds, [this](const std::string& wireId) {
        return !wires_.contains(wireId);
    });
    std::sort(junction.wireIds.begin(), junction.wireIds.end());
    junction.wireIds.erase(
        std::unique(junction.wireIds.begin(), junction.wireIds.end()),
        junction.wireIds.end());
    if (junction.wireIds.size() < 2) {
        throw std::invalid_argument(
            "A junction must connect at least two wires.");
    }
    const auto id = junction.id;
    auto [it, inserted] = junctions_.emplace(id, std::move(junction));
    if (!inserted) {
        throw std::invalid_argument("Duplicate junction id: " + id);
    }
    touch();
    return it->second;
}

bool Circuit::removeJunction(const std::string& junctionId) {
    if (!junctions_.erase(junctionId)) {
        return false;
    }
    touch();
    return true;
}

const std::map<std::string, Junction>& Circuit::junctions() const noexcept {
    return junctions_;
}

bool Circuit::isPinConnected(const PinRef& pin) const {
    return std::any_of(
        wires_.begin(), wires_.end(),
        [&pin](const auto& entry) {
            return entry.second.start == pin || entry.second.end == pin;
        });
}

bool Circuit::containsPin(const PinRef& pin) const {
    const auto* owner = component(pin.componentId);
    return owner && owner->findPin(pin.pinId);
}

std::string Circuit::nextComponentLabel(const std::string& prefix) const {
    for (int index = 1; index < 100000; ++index) {
        const auto candidate = prefix + std::to_string(index);
        const auto found = std::any_of(
            components_.begin(), components_.end(),
            [&candidate](const auto& item) {
                return item.second->label() == candidate;
            });
        if (!found) {
            return candidate;
        }
    }
    return prefix + "_new";
}

std::string Circuit::createId(const std::string& prefix) {
    for (;;) {
        std::ostringstream stream;
        stream << prefix << '_' << std::hex << std::setw(8)
               << std::setfill('0') << idCounter_++;
        const auto candidate = stream.str();
        if (!components_.contains(candidate)
            && !wires_.contains(candidate)
            && !junctions_.contains(candidate)) {
            return candidate;
        }
    }
}

void Circuit::clear() {
    components_.clear();
    wires_.clear();
    junctions_.clear();
    projectName_ = "Untitled";
    canvasSize_ = {1600.0, 1000.0};
    gridSize_ = 20.0;
    logicLowMaximum_ = 1.5;
    logicHighMinimum_ = 3.5;
    touch();
}

void Circuit::touch() noexcept {
    ++revision_;
}

std::uint64_t Circuit::revision() const noexcept {
    return revision_;
}

} // namespace proteus
