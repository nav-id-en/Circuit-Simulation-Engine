#include "proteus/ui/StudioApp.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace proteus::ui {
namespace {

enum class Side {
    Left,
    Right,
    Top,
    Bottom
};

bool containsToken(const std::string& text, const std::string& token) {
    return text.find(token) != std::string::npos;
}

Side pinSide(const Component& component, const PinDefinition& pin,
             std::size_t passiveIndex) {
    const auto type = component.typeId();
    if (type == "ground") return Side::Top;
    if (type == "dc_source" || type == "battery") {
        return pin.id == "P" ? Side::Right : Side::Left;
    }
    if (type == "clock") {
        return pin.id == "OUT" ? Side::Right : Side::Bottom;
    }
    if (type == "resistor" || type == "capacitor" || type == "inductor"
        || type == "switch" || type == "push_button"
        || type == "led" || type == "voltmeter"
        || type == "ammeter") {
        return passiveIndex == 0 ? Side::Left : Side::Right;
    }
    if (type == "potentiometer") {
        if (pin.id == "W") return Side::Top;
        return pin.id == "A" ? Side::Left : Side::Right;
    }
    if (type == "seven_segment") {
        return pin.id == "COM" ? Side::Bottom : Side::Left;
    }
    if (type == "microcontroller") {
        if (pin.id == "VCC") return Side::Top;
        if (pin.id == "GND") return Side::Bottom;
        return containsToken(pin.id, "PA") ? Side::Left : Side::Right;
    }
    if (type == "external_memory") {
        if (pin.id == "RD" || pin.id == "WR") return Side::Bottom;
        return !pin.id.empty() && pin.id.front() == 'A' ? Side::Left
                                                        : Side::Right;
    }
    if (type == "lcd_16x2") {
        return pin.id == "RS" || pin.id == "RW" || pin.id == "E"
            ? Side::Bottom
            : Side::Left;
    }
    if (type == "keypad_4x4") {
        return !pin.id.empty() && pin.id.front() == 'R' ? Side::Left
                                                        : Side::Right;
    }
    if (type == "adc") {
        return !pin.id.empty() && pin.id.front() == 'D' ? Side::Right
                                                        : Side::Left;
    }
    if (type == "dac") {
        if (pin.id == "VREFP" || pin.id == "VREFN") return Side::Bottom;
        return pin.id == "OUT" ? Side::Right : Side::Left;
    }
    if (pin.direction == PinDirection::Output) return Side::Right;
    if (pin.direction == PinDirection::Input) return Side::Left;
    if (pin.direction == PinDirection::Power) {
        if (containsToken(pin.id, "GND") || pin.id == "N") {
            return Side::Bottom;
        }
        return Side::Top;
    }
    return passiveIndex % 2U == 0U ? Side::Left : Side::Right;
}

double distanceToSegment(Point point, Point first, Point second) {
    const auto dx = second.x - first.x;
    const auto dy = second.y - first.y;
    const auto denominator = dx * dx + dy * dy;
    if (denominator < 1.0e-12) {
        return std::hypot(point.x - first.x, point.y - first.y);
    }
    const auto amount = std::clamp(
        ((point.x - first.x) * dx + (point.y - first.y) * dy)
            / denominator,
        0.0, 1.0);
    const Point nearest{first.x + amount * dx, first.y + amount * dy};
    return std::hypot(point.x - nearest.x, point.y - nearest.y);
}

} // namespace

Point StudioApp::screenToWorld(int x, int y) const {
    return {camera_.x + (static_cast<double>(x - canvasRect_.x)
                         / camera_.zoom),
            camera_.y + (static_cast<double>(y - canvasRect_.y)
                         / camera_.zoom)};
}

SDL_Point StudioApp::worldToScreen(Point point) const {
    return {canvasRect_.x
                + static_cast<int>(
                    std::round((point.x - camera_.x) * camera_.zoom)),
            canvasRect_.y
                + static_cast<int>(
                    std::round((point.y - camera_.y) * camera_.zoom))};
}

Point StudioApp::snap(Point point) const {
    const auto grid = circuit_.gridSize();
    return {std::round(point.x / grid) * grid,
            std::round(point.y / grid) * grid};
}

Point StudioApp::componentSize(const Component& component) const {
    const auto type = component.typeId();
    if (type == "ground") return {64.0, 52.0};
    if (type == "seven_segment") return {105.0, 145.0};
    if (type == "adc" || type == "dac") return {135.0, 145.0};
    if (type == "microcontroller") return {190.0, 190.0};
    if (type == "external_memory") return {185.0, 190.0};
    if (type == "lcd_16x2") return {210.0, 105.0};
    if (type == "keypad_4x4") return {155.0, 155.0};
    if (type == "d_flip_flop") return {115.0, 95.0};
    if (type == "voltmeter" || type == "ammeter") return {105.0, 90.0};
    if (containsToken(type, "_gate")) return {105.0, 80.0};
    if (type == "clock" || type == "dc_source" || type == "battery") {
        return {90.0, 90.0};
    }
    return {100.0, 70.0};
}

SDL_Rect StudioApp::componentScreenRect(const Component& component) const {
    auto size = componentSize(component);
    if (component.rotationDegrees() == 90
        || component.rotationDegrees() == 270) {
        std::swap(size.x, size.y);
    }
    const auto center = worldToScreen(component.position());
    const auto width =
        std::max(10, static_cast<int>(std::round(size.x * camera_.zoom)));
    const auto height =
        std::max(10, static_cast<int>(std::round(size.y * camera_.zoom)));
    return {center.x - width / 2, center.y - height / 2, width, height};
}

Point StudioApp::pinWorldPosition(const Component& component,
                                  const PinDefinition& pin) const {
    const auto size = componentSize(component);
    std::size_t passiveIndex = 0;
    for (std::size_t index = 0; index < component.pins().size(); ++index) {
        if (component.pins()[index].id == pin.id) {
            passiveIndex = index;
            break;
        }
    }
    const auto side = pinSide(component, pin, passiveIndex);
    std::vector<const PinDefinition*> sameSide;
    for (std::size_t index = 0; index < component.pins().size(); ++index) {
        const auto& candidate = component.pins()[index];
        if (pinSide(component, candidate, index) == side) {
            sameSide.push_back(&candidate);
        }
    }
    const auto found = std::find_if(
        sameSide.begin(), sameSide.end(), [&pin](const auto* candidate) {
            return candidate->id == pin.id;
        });
    const auto sideIndex =
        found == sameSide.end()
        ? 0
        : static_cast<int>(std::distance(sameSide.begin(), found));
    const auto count = std::max(1, static_cast<int>(sameSide.size()));

    Point local;
    if (side == Side::Left || side == Side::Right) {
        local.x = side == Side::Left ? -size.x / 2.0 : size.x / 2.0;
        local.y = -size.y / 2.0
            + size.y * static_cast<double>(sideIndex + 1)
                / static_cast<double>(count + 1);
    } else {
        local.y = side == Side::Top ? -size.y / 2.0 : size.y / 2.0;
        local.x = -size.x / 2.0
            + size.x * static_cast<double>(sideIndex + 1)
                / static_cast<double>(count + 1);
    }
    if (component.mirroredHorizontally()) local.x = -local.x;
    if (component.mirroredVertically()) local.y = -local.y;
    switch (component.rotationDegrees()) {
    case 90: local = {-local.y, local.x}; break;
    case 180: local = {-local.x, -local.y}; break;
    case 270: local = {local.y, -local.x}; break;
    default: break;
    }
    return {component.position().x + local.x,
            component.position().y + local.y};
}

std::optional<PinRef> StudioApp::pinAt(int x, int y, int radius) const {
    std::optional<PinRef> best;
    auto bestDistance = static_cast<double>(radius);
    for (const auto* component : circuit_.components()) {
        for (const auto& pin : component->pins()) {
            const auto point = worldToScreen(pinWorldPosition(*component, pin));
            const auto distance =
                std::hypot(static_cast<double>(point.x - x),
                           static_cast<double>(point.y - y));
            if (distance <= bestDistance) {
                bestDistance = distance;
                best = component->pinRef(pin.id);
            }
        }
    }
    return best;
}

std::optional<std::string> StudioApp::componentAt(int x, int y) const {
    const auto components = circuit_.components();
    for (auto iterator = components.rbegin(); iterator != components.rend();
         ++iterator) {
        if (contains(componentScreenRect(**iterator), x, y)) {
            return (*iterator)->id();
        }
    }
    return std::nullopt;
}

std::vector<Point> StudioApp::wirePath(const Wire& wire) const {
    const auto* startComponent = circuit_.component(wire.start.componentId);
    const auto* endComponent = circuit_.component(wire.end.componentId);
    const auto* startPin =
        startComponent ? startComponent->findPin(wire.start.pinId) : nullptr;
    const auto* endPin =
        endComponent ? endComponent->findPin(wire.end.pinId) : nullptr;
    if (!startComponent || !endComponent || !startPin || !endPin) return {};

    std::vector<Point> requested;
    requested.push_back(pinWorldPosition(*startComponent, *startPin));
    requested.insert(requested.end(), wire.waypoints.begin(),
                     wire.waypoints.end());
    requested.push_back(pinWorldPosition(*endComponent, *endPin));

    std::vector<Point> result;
    result.push_back(requested.front());
    for (std::size_t index = 1; index < requested.size(); ++index) {
        const auto previous = result.back();
        const auto target = requested[index];
        if (std::abs(previous.x - target.x) > 1.0e-8
            && std::abs(previous.y - target.y) > 1.0e-8) {
            result.push_back({previous.x, target.y});
        }
        if (std::abs(result.back().x - target.x) > 1.0e-8
            || std::abs(result.back().y - target.y) > 1.0e-8) {
            result.push_back(target);
        }
    }
    return result;
}

std::optional<std::string> StudioApp::wireAt(int x, int y) const {
    const auto point = screenToWorld(x, y);
    const auto tolerance = 7.0 / camera_.zoom;
    for (const auto& [id, wire] : circuit_.wires()) {
        const auto path = wirePath(wire);
        for (std::size_t index = 1; index < path.size(); ++index) {
            if (distanceToSegment(point, path[index - 1], path[index])
                <= tolerance) {
                return id;
            }
        }
    }
    return std::nullopt;
}

std::vector<std::string> StudioApp::wiresAtWorldPoint(
    Point point, double tolerance) const {
    std::vector<std::string> result;
    for (const auto& [id, wire] : circuit_.wires()) {
        const auto path = wirePath(wire);
        bool containsPoint = false;
        for (std::size_t index = 1; index < path.size(); ++index) {
            if (distanceToSegment(point, path[index - 1], path[index])
                <= tolerance) {
                containsPoint = true;
                break;
            }
        }
        if (containsPoint) {
            result.push_back(id);
        }
    }
    return result;
}

Color StudioApp::wireColor(const Wire& wire) const {
    if (!engine_ || engine_->state() == SimulationState::Stopped) {
        return Color{111, 136, 157, 255};
    }
    const auto signal = engine_->signalAt(wire.start);
    if (!signal.hasVoltage()) return colors::undefinedSignal;
    if (signal.logic == LogicLevel::High) return colors::highSignal;
    if (signal.logic == LogicLevel::Low) return colors::lowSignal;
    return colors::analogSignal;
}

} // namespace proteus::ui
