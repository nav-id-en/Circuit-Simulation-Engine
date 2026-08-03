#include "proteus/persistence/CircuitSerializer.hpp"

#include "proteus/components/Components.hpp"
#include "proteus/persistence/SimpleJson.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace proteus {
namespace {

Json propertyValueToJson(const PropertyValue& value) {
    Json result(Json::Object{});
    std::visit(
        [&result](const auto& concrete) {
            using ValueType = std::decay_t<decltype(concrete)>;
            if constexpr (std::is_same_v<ValueType, double>) {
                result["type"] = "number";
            } else if constexpr (std::is_same_v<ValueType, int>) {
                result["type"] = "integer";
            } else if constexpr (std::is_same_v<ValueType, bool>) {
                result["type"] = "boolean";
            } else {
                result["type"] = "string";
            }
            result["value"] = Json(concrete);
        },
        value);
    return result;
}

PropertyValue propertyValueFromJson(const Json& object) {
    const auto type = object["type"].asString();
    const auto& value = object["value"];
    if (type == "number") return value.asNumber();
    if (type == "integer") return value.asInt();
    if (type == "boolean") return value.asBool();
    if (type == "string") return value.asString();
    throw std::runtime_error("Unsupported serialized property type.");
}

Json propertyMapToJson(const PropertyMap& properties) {
    Json object(Json::Object{});
    for (const auto& [key, value] : properties) {
        object[key] = propertyValueToJson(value);
    }
    return object;
}

PropertyMap propertyMapFromJson(const Json& object) {
    PropertyMap result;
    if (!object.isObject()) return result;
    for (const auto& [key, value] : object.asObject()) {
        if (value.isObject()) {
            result[key] = propertyValueFromJson(value);
        }
    }
    return result;
}

Json pinRefToJson(const PinRef& pin) {
    Json object(Json::Object{});
    object["componentId"] = pin.componentId;
    object["pinId"] = pin.pinId;
    return object;
}

PinRef pinRefFromJson(const Json& object) {
    return {object["componentId"].asString(), object["pinId"].asString()};
}

std::string simulationStateToString(SimulationState state) {
    switch (state) {
    case SimulationState::Stopped: return "stopped";
    case SimulationState::Running: return "running";
    case SimulationState::Paused: return "paused";
    }
    return "stopped";
}

SimulationState simulationStateFromString(const std::string& value) {
    if (value == "running") return SimulationState::Running;
    if (value == "paused") return SimulationState::Paused;
    return SimulationState::Stopped;
}

std::string encodeBase64(const std::vector<std::uint8_t>& bytes) {
    static constexpr char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    result.reserve((bytes.size() + 2U) / 3U * 4U);
    for (std::size_t index = 0; index < bytes.size(); index += 3U) {
        const auto remaining = bytes.size() - index;
        const auto first = static_cast<unsigned>(bytes[index]);
        const auto second =
            remaining > 1U ? static_cast<unsigned>(bytes[index + 1U]) : 0U;
        const auto third =
            remaining > 2U ? static_cast<unsigned>(bytes[index + 2U]) : 0U;
        const auto packed = (first << 16U) | (second << 8U) | third;
        result.push_back(alphabet[(packed >> 18U) & 0x3FU]);
        result.push_back(alphabet[(packed >> 12U) & 0x3FU]);
        result.push_back(
            remaining > 1U ? alphabet[(packed >> 6U) & 0x3FU] : '=');
        result.push_back(remaining > 2U ? alphabet[packed & 0x3FU] : '=');
    }
    return result;
}

std::vector<std::uint8_t> decodeBase64(const std::string& text) {
    const auto valueOf = [](char value) -> int {
        if (value >= 'A' && value <= 'Z') return value - 'A';
        if (value >= 'a' && value <= 'z') return value - 'a' + 26;
        if (value >= '0' && value <= '9') return value - '0' + 52;
        if (value == '+') return 62;
        if (value == '/') return 63;
        return -1;
    };
    std::vector<std::uint8_t> result;
    unsigned accumulator = 0;
    int bits = 0;
    for (const auto value : text) {
        if (value == '=') break;
        const auto decoded = valueOf(value);
        if (decoded < 0) continue;
        accumulator = (accumulator << 6U) | static_cast<unsigned>(decoded);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            result.push_back(static_cast<std::uint8_t>(
                (accumulator >> static_cast<unsigned>(bits)) & 0xFFU));
        }
    }
    return result;
}

Json toJson(const Circuit& circuit, const SimulationEngine* engine,
            bool includeRuntimeState) {
    Json root(Json::Object{});
    root["format"] = "OOPProteusProject";
    root["schemaVersion"] = 3;
    root["frontend"] = "SDL2";
    root["projectName"] = circuit.projectName();

    Json canvas(Json::Object{});
    canvas["width"] = circuit.canvasSize().x;
    canvas["height"] = circuit.canvasSize().y;
    canvas["gridSize"] = circuit.gridSize();
    root["canvas"] = std::move(canvas);

    Json thresholds(Json::Object{});
    thresholds["lowMaximum"] = circuit.logicLowMaximum();
    thresholds["highMinimum"] = circuit.logicHighMinimum();
    root["logicThresholds"] = std::move(thresholds);

    Json components(Json::Array{});
    for (const auto* component : circuit.components()) {
        Json object(Json::Object{});
        object["id"] = component->id();
        object["type"] = component->typeId();
        object["label"] = component->label();
        object["x"] = component->position().x;
        object["y"] = component->position().y;
        object["rotation"] = component->rotationDegrees();
        object["mirrorHorizontal"] = component->mirroredHorizontally();
        object["mirrorVertical"] = component->mirroredVertically();
        object["properties"] = propertyMapToJson(component->properties());
        if (includeRuntimeState) {
            object["runtime"] =
                propertyMapToJson(component->runtimeState());
        }
        if (const auto* microcontroller =
                dynamic_cast<const MicrocontrollerComponent*>(component)) {
            object["flashBase64"] =
                encodeBase64(microcontroller->flash());
        }
        components.pushBack(std::move(object));
    }
    root["components"] = std::move(components);

    Json wires(Json::Array{});
    for (const auto& [wireId, wire] : circuit.wires()) {
        Json object(Json::Object{});
        object["id"] = wireId;
        object["start"] = pinRefToJson(wire.start);
        object["end"] = pinRefToJson(wire.end);
        Json waypoints(Json::Array{});
        for (const auto& point : wire.waypoints) {
            Json waypoint(Json::Object{});
            waypoint["x"] = point.x;
            waypoint["y"] = point.y;
            waypoints.pushBack(std::move(waypoint));
        }
        object["waypoints"] = std::move(waypoints);
        wires.pushBack(std::move(object));
    }
    root["wires"] = std::move(wires);

    Json junctions(Json::Array{});
    for (const auto& [junctionId, junction] : circuit.junctions()) {
        Json object(Json::Object{});
        object["id"] = junctionId;
        object["x"] = junction.position.x;
        object["y"] = junction.position.y;
        Json wireIds(Json::Array{});
        for (const auto& wireId : junction.wireIds) {
            wireIds.pushBack(wireId);
        }
        object["wireIds"] = std::move(wireIds);
        junctions.pushBack(std::move(object));
    }
    root["junctions"] = std::move(junctions);

    if (engine) {
        Json simulation(Json::Object{});
        simulation["state"] = simulationStateToString(engine->state());
        simulation["timeSeconds"] = engine->timeSeconds();
        simulation["timeStepSeconds"] = engine->timeStepSeconds();
        root["simulation"] = std::move(simulation);
    }
    return root;
}

LoadedSimulationState fromJson(const Json& root, Circuit& circuit) {
    if (!root.isObject()
        || root["format"].asString() != "OOPProteusProject") {
        throw std::runtime_error("This is not an OOP Proteus project file.");
    }
    const auto schemaVersion = root["schemaVersion"].asInt();
    if (schemaVersion < 1 || schemaVersion > 3) {
        throw std::runtime_error("Unsupported project schema version.");
    }
    if (!root["components"].isArray() || !root["wires"].isArray()) {
        throw std::runtime_error("Project has no component or wire list.");
    }

    circuit.clear();
    circuit.setProjectName(root["projectName"].asString("Untitled"));
    const auto& canvas = root["canvas"];
    circuit.setCanvasSize(
        {canvas["width"].asNumber(1600.0),
         canvas["height"].asNumber(1000.0)});
    circuit.setGridSize(canvas["gridSize"].asNumber(20.0));
    const auto& thresholds = root["logicThresholds"];
    circuit.setLogicThresholds(
        thresholds["lowMaximum"].asNumber(1.5),
        thresholds["highMinimum"].asNumber(3.5));

    for (const auto& object : root["components"].asArray()) {
        const auto id = object["id"].asString();
        const auto type = object["type"].asString();
        const auto label = object["label"].asString();
        if (id.empty() || type.empty()) {
            throw std::runtime_error(
                "Serialized component is missing its id or type.");
        }
        auto component = ComponentFactory::create(type, id, label);
        component->setPosition(
            {object["x"].asNumber(), object["y"].asNumber()});
        component->setRotationDegrees(object["rotation"].asInt());
        component->setMirroredHorizontally(
            object["mirrorHorizontal"].asBool());
        component->setMirroredVertically(
            object["mirrorVertical"].asBool());
        const auto properties =
            propertyMapFromJson(object["properties"]);
        for (const auto& [key, value] : properties) {
            component->setProperty(key, value);
        }
        if (object.contains("runtime")) {
            component->replaceRuntimeState(
                propertyMapFromJson(object["runtime"]));
        }
        if (auto* microcontroller =
                dynamic_cast<MicrocontrollerComponent*>(component.get())) {
            const auto bytes =
                decodeBase64(object["flashBase64"].asString());
            if (!bytes.empty()) {
                const auto runtime = component->runtimeState();
                microcontroller->loadFlash(bytes);
                component->replaceRuntimeState(runtime);
                microcontroller->restoreExecutionStateFromRuntime();
            }
        }
        circuit.addComponent(std::move(component));
    }

    for (const auto& object : root["wires"].asArray()) {
        Wire wire;
        wire.id = object["id"].asString();
        wire.start = pinRefFromJson(object["start"]);
        wire.end = pinRefFromJson(object["end"]);
        if (object["waypoints"].isArray()) {
            for (const auto& waypoint : object["waypoints"].asArray()) {
                wire.waypoints.push_back(
                    {waypoint["x"].asNumber(),
                     waypoint["y"].asNumber()});
            }
        }
        circuit.addWire(std::move(wire));
    }

    if (root["junctions"].isArray()) {
        for (const auto& object : root["junctions"].asArray()) {
            Junction junction;
            junction.id = object["id"].asString();
            junction.position =
                {object["x"].asNumber(), object["y"].asNumber()};
            if (object["wireIds"].isArray()) {
                for (const auto& wireId : object["wireIds"].asArray()) {
                    junction.wireIds.push_back(wireId.asString());
                }
            }
            circuit.addJunction(std::move(junction));
        }
    }

    const auto& simulation = root["simulation"];
    return {
        simulationStateFromString(simulation["state"].asString()),
        simulation["timeSeconds"].asNumber(0.0),
        simulation["timeStepSeconds"].asNumber(0.001)};
}

} // namespace

std::string CircuitSerializer::toString(
    const Circuit& circuit, const SimulationEngine* engine,
    bool includeRuntimeState, bool pretty) {
    return toJson(circuit, engine, includeRuntimeState).dump(pretty ? 2 : 0);
}

LoadedSimulationState CircuitSerializer::fromString(
    const std::string& text, Circuit& circuit) {
    return fromJson(Json::parse(text), circuit);
}

void CircuitSerializer::saveToFile(
    const std::string& filePath, const Circuit& circuit,
    const SimulationEngine* engine, bool includeRuntimeState) {
    const std::filesystem::path path(filePath);
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
    }
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        throw std::runtime_error("Cannot save project: " + filePath);
    }
    output << toString(circuit, engine, includeRuntimeState, true);
    if (!output) {
        throw std::runtime_error("Cannot write project: " + filePath);
    }
}

LoadedSimulationState CircuitSerializer::loadFromFile(
    const std::string& filePath, Circuit& circuit) {
    std::ifstream input(filePath, std::ios::binary);
    if (!input) {
        throw std::runtime_error("Cannot open project: " + filePath);
    }
    std::ostringstream content;
    content << input.rdbuf();
    return fromString(content.str(), circuit);
}

} // namespace proteus
