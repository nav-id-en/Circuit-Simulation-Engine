#include "proteus/core/Component.hpp"

#include <algorithm>
#include <utility>

namespace proteus {

Component::Component(std::string id, std::string label)
    : id_(std::move(id)), label_(std::move(label)) {
}

const std::string& Component::id() const noexcept {
    return id_;
}

const std::string& Component::label() const noexcept {
    return label_;
}

void Component::setLabel(std::string label) {
    label_ = std::move(label);
}

Point Component::position() const noexcept {
    return position_;
}

void Component::setPosition(Point position) noexcept {
    position_ = position;
}

int Component::rotationDegrees() const noexcept {
    return rotationDegrees_;
}

void Component::setRotationDegrees(int degrees) noexcept {
    degrees %= 360;
    if (degrees < 0) {
        degrees += 360;
    }
    rotationDegrees_ = (degrees / 90) * 90;
}

bool Component::mirroredHorizontally() const noexcept {
    return mirrorHorizontal_;
}

bool Component::mirroredVertically() const noexcept {
    return mirrorVertical_;
}

void Component::setMirroredHorizontally(bool mirrored) noexcept {
    mirrorHorizontal_ = mirrored;
}

void Component::setMirroredVertically(bool mirrored) noexcept {
    mirrorVertical_ = mirrored;
}

const std::vector<PinDefinition>& Component::pins() const noexcept {
    return pins_;
}

const PinDefinition* Component::findPin(const std::string& pinId) const {
    const auto it = std::find_if(
        pins_.begin(), pins_.end(),
        [&pinId](const PinDefinition& pin) { return pin.id == pinId; });
    return it == pins_.end() ? nullptr : &*it;
}

PinRef Component::pinRef(const std::string& pinId) const {
    return {id_, pinId};
}

const PropertyMap& Component::properties() const noexcept {
    return properties_;
}

const PropertyMap& Component::runtimeState() const noexcept {
    return runtimeState_;
}

std::vector<PropertyDefinition> Component::propertyDefinitions() const {
    return propertyDefinitions_;
}

bool Component::setProperty(const std::string& key, PropertyValue value) {
    const auto definition = std::find_if(
        propertyDefinitions_.begin(), propertyDefinitions_.end(),
        [&key](const PropertyDefinition& item) { return item.key == key; });
    if (definition == propertyDefinitions_.end()) {
        return false;
    }

    if (definition->kind == PropertyKind::Number) {
        if (auto* numeric = std::get_if<double>(&value)) {
            if (definition->maximum > definition->minimum) {
                *numeric = std::clamp(*numeric, definition->minimum,
                                      definition->maximum);
            }
        } else {
            return false;
        }
    } else if (definition->kind == PropertyKind::Integer) {
        if (auto* numeric = std::get_if<int>(&value)) {
            if (definition->maximum > definition->minimum) {
                *numeric = std::clamp(
                    *numeric, static_cast<int>(definition->minimum),
                    static_cast<int>(definition->maximum));
            }
        } else {
            return false;
        }
    } else if (definition->kind == PropertyKind::Boolean
               && !std::holds_alternative<bool>(value)) {
        return false;
    } else if ((definition->kind == PropertyKind::Text
                || definition->kind == PropertyKind::FilePath
                || definition->kind == PropertyKind::Color
                || definition->kind == PropertyKind::Choice)
               && !std::holds_alternative<std::string>(value)) {
        return false;
    }

    properties_[key] = std::move(value);
    return true;
}

void Component::setRuntimeValue(const std::string& key, PropertyValue value) {
    runtimeState_[key] = std::move(value);
}

void Component::replaceRuntimeState(PropertyMap state) {
    runtimeState_ = std::move(state);
}

void Component::resetRuntime() {
    runtimeState_.clear();
}

void Component::stampAnalog(AnalogStampCollector&, double) {
}

void Component::acceptAnalogResult(const AnalogResultView&, double) {
}

std::vector<DriveRequest> Component::evaluate(const SimulationView&) {
    return {};
}

std::optional<std::string> Component::validationError() const {
    return std::nullopt;
}

void Component::addPin(PinDefinition pin) {
    pins_.push_back(std::move(pin));
}

void Component::defineProperty(PropertyDefinition definition) {
    properties_.insert_or_assign(definition.key, definition.defaultValue);
    propertyDefinitions_.push_back(std::move(definition));
}

void Component::copyCommonStateTo(Component& target) const {
    target.setLabel(label_);
    target.setPosition(position_);
    target.setRotationDegrees(rotationDegrees_);
    target.setMirroredHorizontally(mirrorHorizontal_);
    target.setMirroredVertically(mirrorVertical_);
    for (const auto& [key, value] : properties_) {
        target.setProperty(key, value);
    }
    target.replaceRuntimeState(runtimeState_);
}

} // namespace proteus
