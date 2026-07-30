#pragma once

#include "proteus/core/Component.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace proteus {

struct Wire {
    std::string id;
    PinRef start;
    PinRef end;
    std::vector<Point> waypoints;
};

struct Junction {
    std::string id;
    Point position;
    std::vector<std::string> wireIds;
};

class Circuit {
public:
    Circuit();

    [[nodiscard]] const std::string& projectName() const noexcept;
    void setProjectName(std::string name);
    [[nodiscard]] Point canvasSize() const noexcept;
    void setCanvasSize(Point size);
    [[nodiscard]] double gridSize() const noexcept;
    void setGridSize(double size);
    [[nodiscard]] double logicLowMaximum() const noexcept;
    [[nodiscard]] double logicHighMinimum() const noexcept;
    void setLogicThresholds(double lowMaximum, double highMinimum);

    Component& addComponent(std::unique_ptr<Component> component);
    bool removeComponent(const std::string& componentId);
    [[nodiscard]] Component* component(const std::string& componentId);
    [[nodiscard]] const Component* component(const std::string& componentId) const;
    [[nodiscard]] std::vector<Component*> components();
    [[nodiscard]] std::vector<const Component*> components() const;

    Wire& addWire(Wire wire);
    bool removeWire(const std::string& wireId);
    [[nodiscard]] Wire* wire(const std::string& wireId);
    [[nodiscard]] const Wire* wire(const std::string& wireId) const;
    [[nodiscard]] const std::map<std::string, Wire>& wires() const noexcept;

    Junction& addJunction(Junction junction);
    bool removeJunction(const std::string& junctionId);
    [[nodiscard]] const std::map<std::string, Junction>& junctions() const noexcept;

    [[nodiscard]] bool isPinConnected(const PinRef& pin) const;
    [[nodiscard]] bool containsPin(const PinRef& pin) const;
    [[nodiscard]] std::string nextComponentLabel(const std::string& prefix) const;
    [[nodiscard]] std::string createId(const std::string& prefix);

    void clear();
    void touch() noexcept;
    [[nodiscard]] std::uint64_t revision() const noexcept;

private:
    std::string projectName_ = "Untitled";
    Point canvasSize_{1600.0, 1000.0};
    double gridSize_ = 20.0;
    double logicLowMaximum_ = 1.5;
    double logicHighMinimum_ = 3.5;
    std::map<std::string, std::unique_ptr<Component>> components_;
    std::map<std::string, Wire> wires_;
    std::map<std::string, Junction> junctions_;
    std::uint64_t revision_ = 1;
    std::uint64_t idCounter_ = 1;
};

} // namespace proteus
