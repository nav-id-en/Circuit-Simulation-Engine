#include "proteus/ui/StudioApp.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <iterator>

namespace proteus::ui {
namespace {

std::string lowerCopy(std::string value) {
    std::transform(
        value.begin(), value.end(), value.begin(),
        [](unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
    return value;
}

SDL_Rect normalizedRect(int x1, int y1, int x2, int y2) {
    return {std::min(x1, x2), std::min(y1, y2), std::abs(x2 - x1),
            std::abs(y2 - y1)};
}

} // namespace

void StudioApp::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_QUIT) {
        running_ = false;
        return;
    }
    if (event.type == SDL_WINDOWEVENT
        && (event.window.event == SDL_WINDOWEVENT_RESIZED
            || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)) {
        windowWidth_ = event.window.data1;
        windowHeight_ = event.window.data2;
        updateLayout();
        return;
    }
    if (event.type == SDL_KEYDOWN) handleKeyDown(event.key);
    if (event.type == SDL_TEXTINPUT) handleTextInput(event.text);
    if (event.type == SDL_MOUSEBUTTONDOWN) handleMouseDown(event.button);
    if (event.type == SDL_MOUSEBUTTONUP) handleMouseUp(event.button);
    if (event.type == SDL_MOUSEMOTION) handleMouseMotion(event.motion);
    if (event.type == SDL_MOUSEWHEEL) handleMouseWheel(event.wheel);
}

void StudioApp::handleKeyDown(const SDL_KeyboardEvent& event) {
    const auto key = event.keysym.sym;
    const auto control = (event.keysym.mod & KMOD_CTRL) != 0;
    const auto shift = (event.keysym.mod & KMOD_SHIFT) != 0;

    if (modal_) {
        if (key == SDLK_ESCAPE) {
            cancelModal();
        } else if (key == SDLK_RETURN) {
            acceptModal();
        } else if (key == SDLK_BACKSPACE && !modal_->value.empty()) {
            modal_->value.pop_back();
        } else if (control && key == SDLK_v && SDL_HasClipboardText()) {
            auto* clipboard = SDL_GetClipboardText();
            if (clipboard) {
                modal_->value += clipboard;
                SDL_free(clipboard);
            }
        }
        return;
    }

    if (searchFocused_) {
        if (key == SDLK_ESCAPE || key == SDLK_RETURN) {
            searchFocused_ = false;
            SDL_StopTextInput();
        } else if (key == SDLK_BACKSPACE && !searchText_.empty()) {
            searchText_.pop_back();
            libraryScroll_ = 0;
        } else if (control && key == SDLK_v && SDL_HasClipboardText()) {
            auto* clipboard = SDL_GetClipboardText();
            if (clipboard) {
                searchText_ += clipboard;
                SDL_free(clipboard);
                libraryScroll_ = 0;
            }
        }
        return;
    }

    if (control) {
        if (key == SDLK_n) handleAction("new");
        else if (key == SDLK_o) handleAction("open");
        else if (key == SDLK_s && shift) handleAction("save_as");
        else if (key == SDLK_s) handleAction("save");
        else if (key == SDLK_e) handleAction("export");
        else if (key == SDLK_z) undo();
        else if (key == SDLK_y) redo();
        else if (key == SDLK_a && screen_ == Screen::Editor) {
            selectedComponents_.clear();
            selectedWires_.clear();
            for (const auto* component : circuit_.components()) {
                selectedComponents_.insert(component->id());
            }
            for (const auto& [id, wire] : circuit_.wires()) {
                static_cast<void>(wire);
                selectedWires_.insert(id);
            }
        }
        return;
    }

    if (key == SDLK_ESCAPE) {
        if (showScope_) {
            showScope_ = false;
        } else if (!openMenu_.empty()) {
            openMenu_.clear();
        } else if (wireStart_) {
            wireStart_.reset();
            setToast("WIRE CANCELLED", colors::muted);
        } else {
            setTool(Tool::Select);
        }
        return;
    }
    if (screen_ != Screen::Editor) return;
    if (key == SDLK_DELETE || key == SDLK_BACKSPACE) deleteSelection();
    else if (key == SDLK_r) rotateSelection();
    else if (key == SDLK_h) mirrorSelection(true);
    else if (key == SDLK_v) mirrorSelection(false);
    else if (key == SDLK_w) setTool(Tool::Wire);
    else if (key == SDLK_j) setTool(Tool::Junction);
    else if (key == SDLK_p) setTool(Tool::Probe);
    else if (key == SDLK_PLUS || key == SDLK_EQUALS
             || key == SDLK_KP_PLUS) {
        camera_.zoom = std::min(3.5, camera_.zoom * 1.15);
    } else if (key == SDLK_MINUS || key == SDLK_KP_MINUS) {
        camera_.zoom = std::max(0.25, camera_.zoom / 1.15);
    } else if ((key == SDLK_LEFT || key == SDLK_RIGHT || key == SDLK_UP
                || key == SDLK_DOWN)
               && engine_->state() == SimulationState::Stopped
               && !selectedComponents_.empty()) {
        const auto distance = shift ? 1.0 : circuit_.gridSize();
        for (const auto& id : selectedComponents_) {
            auto* component = circuit_.component(id);
            if (!component) continue;
            auto position = component->position();
            if (key == SDLK_LEFT) position.x -= distance;
            if (key == SDLK_RIGHT) position.x += distance;
            if (key == SDLK_UP) position.y -= distance;
            if (key == SDLK_DOWN) position.y += distance;
            component->setPosition(shift ? position : snap(position));
        }
        circuit_.touch();
        markModified("Nudge selection");
    } else if (key == SDLK_SPACE) {
        setTool(Tool::Pan);
    }
}

void StudioApp::handleTextInput(const SDL_TextInputEvent& event) {
    if (modal_) {
        modal_->value += event.text;
    } else if (searchFocused_) {
        searchText_ += event.text;
        libraryScroll_ = 0;
    }
}

void StudioApp::handleMouseDown(const SDL_MouseButtonEvent& event) {
    mouseX_ = previousMouseX_ = event.x;
    mouseY_ = previousMouseY_ = event.y;
    if (modal_) {
        const SDL_Rect ok{modal_->box.x + modal_->box.w - 220,
                          modal_->box.y + modal_->box.h - 58, 90, 34};
        const SDL_Rect cancel{modal_->box.x + modal_->box.w - 118,
                              modal_->box.y + modal_->box.h - 58, 90, 34};
        if (contains(ok, event.x, event.y)) acceptModal();
        else if (contains(cancel, event.x, event.y)) cancelModal();
        return;
    }
    if (screen_ == Screen::Welcome) {
        if (event.button == SDL_BUTTON_LEFT) {
            handleWelcomeClick(event.x, event.y);
        }
        return;
    }
    handleEditorClick(
        event.x, event.y, event.button, event.clicks,
        static_cast<Uint16>(SDL_GetModState()));
}

void StudioApp::handleMouseUp(const SDL_MouseButtonEvent& event) {
    mouseX_ = event.x;
    mouseY_ = event.y;
    if (modal_ || screen_ == Screen::Welcome) return;

    if (!runtimePressedComponent_.empty()) {
        handleRuntimePress(runtimePressedComponent_, false, event.x, event.y);
    }
    if (libraryDragging_) {
        if (contains(canvasRect_, event.x, event.y)
            && !draggedLibraryType_.empty()) {
            addComponent(draggedLibraryType_,
                         screenToWorld(event.x, event.y));
            setTool(Tool::Select);
        }
        libraryDragging_ = false;
        draggedLibraryType_.clear();
    }
    if (movingComponents_) {
        movingComponents_ = false;
        if (componentMoveChanged_) {
            markModified("Move component");
            componentMoveChanged_ = false;
        }
        dragOriginalPositions_.clear();
    }
    if (selectingBox_) {
        selectingBox_ = false;
        for (const auto* component : circuit_.components()) {
            if (intersects(selectionBox_, componentScreenRect(*component))) {
                selectedComponents_.insert(component->id());
            }
        }
        selectionBox_ = {};
    }
    panning_ = false;
}

void StudioApp::handleMouseMotion(const SDL_MouseMotionEvent& event) {
    previousMouseX_ = mouseX_;
    previousMouseY_ = mouseY_;
    mouseX_ = event.x;
    mouseY_ = event.y;
    if (modal_ || screen_ != Screen::Editor) return;
    hoveredPin_ = contains(canvasRect_, event.x, event.y)
        ? pinAt(event.x, event.y)
        : std::nullopt;

    if (panning_) {
        camera_.x -= static_cast<double>(event.xrel) / camera_.zoom;
        camera_.y -= static_cast<double>(event.yrel) / camera_.zoom;
    }
    if (movingComponents_) {
        const auto current = screenToWorld(event.x, event.y);
        const Point delta{current.x - dragStartWorld_.x,
                          current.y - dragStartWorld_.y};
        for (const auto& [id, original] : dragOriginalPositions_) {
            if (auto* component = circuit_.component(id)) {
                const auto next = snap(
                    {original.x + delta.x, original.y + delta.y});
                if (std::abs(next.x - component->position().x) > 1.0e-9
                    || std::abs(next.y - component->position().y) > 1.0e-9) {
                    component->setPosition(next);
                    componentMoveChanged_ = true;
                }
            }
        }
        if (componentMoveChanged_) circuit_.touch();
    }
    if (selectingBox_) {
        const auto start = worldToScreen(dragStartWorld_);
        selectionBox_ =
            normalizedRect(start.x, start.y, event.x, event.y);
    }
}

void StudioApp::handleMouseWheel(const SDL_MouseWheelEvent& event) {
    if (modal_ || screen_ != Screen::Editor) return;
    if (contains(libraryRect_, mouseX_, mouseY_)) {
        const auto maxScroll =
            std::max(0, static_cast<int>(filteredCatalog().size()) * 42
                            - (libraryRect_.h - 230));
        libraryScroll_ =
            std::clamp(libraryScroll_ - event.y * 42, 0, maxScroll);
        return;
    }
    if (contains(propertiesRect_, mouseX_, mouseY_)) {
        propertyScroll_ =
            std::max(0, propertyScroll_ - event.y * 35);
        return;
    }
    if (showLog_ && contains(logRect_, mouseX_, mouseY_)) {
        logScroll_ = std::clamp(
            logScroll_ - event.y * 2, 0,
            static_cast<int>(engine_->logEntries().size()));
        return;
    }
    if (!contains(canvasRect_, mouseX_, mouseY_)) return;

    const auto before = screenToWorld(mouseX_, mouseY_);
    const auto factor = event.y > 0 ? 1.12 : 1.0 / 1.12;
    camera_.zoom = std::clamp(camera_.zoom * factor, 0.25, 3.5);
    const auto after = screenToWorld(mouseX_, mouseY_);
    camera_.x += before.x - after.x;
    camera_.y += before.y - after.y;
}

void StudioApp::handleWelcomeClick(int x, int y) {
    const SDL_Rect card{windowWidth_ / 2 - 380, windowHeight_ / 2 - 290,
                        760, 580};
    const SDL_Rect newButton{card.x + 48, card.y + 172, 205, 54};
    const SDL_Rect openButton{card.x + 270, card.y + 172, 205, 54};
    const SDL_Rect demoButton{card.x + 492, card.y + 172, 205, 54};
    if (contains(newButton, x, y)) {
        handleAction("new");
        return;
    }
    if (contains(openButton, x, y)) {
        handleAction("open");
        return;
    }
    if (contains(demoButton, x, y)) {
        openProject("examples/mixed_signal_demo.oopproteus.json");
        return;
    }
    const auto count = std::min<std::size_t>(recentProjects_.size(), 6);
    for (std::size_t index = 0; index < count; ++index) {
        SDL_Rect row{card.x + 48,
                     card.y + 300 + static_cast<int>(index) * 38,
                     card.w - 96, 32};
        if (contains(row, x, y)) {
            openProject(recentProjects_[index]);
            return;
        }
    }
}

void StudioApp::handleEditorClick(int x, int y, Uint8 button,
                                  Uint8 clicks, Uint16 modifiers) {
    static_cast<void>(clicks);
    static_cast<void>(modifiers);
    if (showScope_ && button == SDL_BUTTON_LEFT) {
        const SDL_Rect panel{canvasRect_.x + 40, canvasRect_.y + 40,
                             std::max(360, canvasRect_.w - 80),
                             std::max(250, canvasRect_.h - 80)};
        const SDL_Rect timeMinus{panel.x + 14, panel.y + 42, 28, 28};
        const SDL_Rect timePlus{panel.x + 218, panel.y + 42, 28, 28};
        const SDL_Rect voltsMinus{panel.x + 270, panel.y + 42, 28, 28};
        const SDL_Rect voltsPlus{panel.x + 474, panel.y + 42, 28, 28};
        const SDL_Rect close{panel.x + panel.w - 95, panel.y + 8, 82, 28};
        if (contains(timeMinus, x, y)) {
            scopeTimePerDivision_ =
                std::max(0.001, scopeTimePerDivision_ / 2.0);
        } else if (contains(timePlus, x, y)) {
            scopeTimePerDivision_ =
                std::min(100.0, scopeTimePerDivision_ * 2.0);
        } else if (contains(voltsMinus, x, y)) {
            scopeVoltsPerDivision_ =
                std::max(0.01, scopeVoltsPerDivision_ / 2.0);
        } else if (contains(voltsPlus, x, y)) {
            scopeVoltsPerDivision_ =
                std::min(1000.0, scopeVoltsPerDivision_ * 2.0);
        } else if (contains(close, x, y)) {
            showScope_ = false;
        }
        return;
    }
    if (auto action = dropdownActionAt(x, y)) {
        handleAction(*action);
        openMenu_.clear();
        return;
    }
    if (auto menu = menuAt(x, y)) {
        openMenu_ = openMenu_ == *menu ? std::string{} : *menu;
        return;
    }
    if (!openMenu_.empty()) {
        openMenu_.clear();
        return;
    }
    if (auto action = toolbarAt(x, y)) {
        handleAction(*action);
        return;
    }

    if (contains(searchRect_, x, y)) {
        searchFocused_ = true;
        SDL_StartTextInput();
        return;
    }
    if (contains(categoryRect_, x, y)) {
        std::vector<std::string> categories = {"All"};
        for (const auto& entry : catalog_) {
            if (std::find(categories.begin(), categories.end(),
                          entry.category)
                == categories.end()) {
                categories.push_back(entry.category);
            }
        }
        const auto current =
            std::find(categories.begin(), categories.end(), categoryFilter_);
        const auto next =
            current == categories.end() || std::next(current) == categories.end()
            ? categories.begin()
            : std::next(current);
        categoryFilter_ = *next;
        libraryScroll_ = 0;
        return;
    }
    if (auto row = libraryRowAt(x, y)) {
        const auto entries = filteredCatalog();
        if (*row < entries.size()) {
            setTool(Tool::Place, entries[*row].type);
            draggedLibraryType_ = entries[*row].type;
            libraryDragging_ = true;
            return;
        }
    }

    const SDL_Rect runtimeToggle{propertiesRect_.x + 12,
                                 propertiesRect_.y + 43,
                                 propertiesRect_.w - 24, 27};
    if (contains(runtimeToggle, x, y)) {
        saveRuntimeState_ = !saveRuntimeState_;
        setToast(saveRuntimeState_ ? "RUNTIME STATE WILL BE SAVED"
                                   : "RUNTIME STATE SAVE DISABLED");
        return;
    }
    if (auto property = propertyAt(x, y)) {
        auto* component = circuit_.component(property->first);
        if (!component) return;
        if (property->second == "__label__") {
            modal_ = Modal{ModalAction::EditLabel, "EDIT COMPONENT LABEL",
                           component->label(), "__label__",
                           PropertyKind::Text, {}};
            SDL_StartTextInput();
            return;
        }
        const auto definitions = component->propertyDefinitions();
        const auto found = std::find_if(
            definitions.begin(), definitions.end(),
            [&property](const PropertyDefinition& value) {
                return value.key == property->second;
            });
        if (found != definitions.end()) {
            const auto current = component->properties().find(found->key);
            if (found->kind == PropertyKind::Boolean
                && current != component->properties().end()) {
                const auto old = std::get_if<bool>(&current->second);
                if (old) {
                    component->setProperty(found->key, !*old);
                    circuit_.touch();
                    markModified("Toggle property");
                }
            } else {
                modal_ = Modal{
                    ModalAction::EditProperty,
                    "EDIT " + found->title,
                    current == component->properties().end()
                        ? std::string{}
                        : propertyValueText(current->second),
                    found->key, found->kind, {}};
                SDL_StartTextInput();
            }
        }
        return;
    }

    if (showLog_ && contains(logRect_, x, y)
        && y < logRect_.y + 26 && x > logRect_.x + logRect_.w - 150) {
        showLog_ = false;
        updateLayout();
        return;
    }
    if (!contains(canvasRect_, x, y)) return;

    if (button == SDL_BUTTON_MIDDLE || button == SDL_BUTTON_RIGHT
        || (tool_ == Tool::Pan && button == SDL_BUTTON_LEFT)) {
        panning_ = true;
        return;
    }
    if (button != SDL_BUTTON_LEFT) return;
    const auto world = screenToWorld(x, y);
    if (tool_ == Tool::Place && !placeType_.empty()) {
        addComponent(placeType_, world);
        return;
    }
    if (tool_ == Tool::Wire || wireStart_) {
        if (const auto pin = pinAt(x, y)) {
            if (wireStart_) finishWire(*pin);
            else beginWire(*pin);
        } else {
            setToast("CLICK DIRECTLY ON A PIN", colors::warning);
        }
        return;
    }
    if (tool_ == Tool::Junction) {
        createJunction(world);
        return;
    }
    if (tool_ == Tool::Probe) {
        if (const auto pin = pinAt(x, y)) {
            addProbe(*pin);
        } else {
            setToast("PROBE MUST BE PLACED ON A PIN", colors::warning);
        }
        return;
    }

    if (const auto componentId = componentAt(x, y)) {
        if (engine_->state() != SimulationState::Stopped) {
            handleRuntimePress(*componentId, true, x, y);
            return;
        }
        const auto shiftHeld = (modifiers & KMOD_SHIFT) != 0;
        if (shiftHeld) {
            if (selectedComponents_.contains(*componentId)) {
                selectedComponents_.erase(*componentId);
            } else {
                selectedComponents_.insert(*componentId);
            }
        } else if (!selectedComponents_.contains(*componentId)) {
            selectOnly(*componentId);
        }
        movingComponents_ = true;
        componentMoveChanged_ = false;
        dragStartWorld_ = world;
        dragOriginalPositions_.clear();
        for (const auto& id : selectedComponents_) {
            if (const auto* component = circuit_.component(id)) {
                dragOriginalPositions_[id] = component->position();
            }
        }
        return;
    }
    if (const auto wireId = wireAt(x, y)) {
        selectedComponents_.clear();
        selectedWires_.clear();
        selectedWires_.insert(*wireId);
        return;
    }
    clearSelection();
    selectingBox_ = true;
    dragStartWorld_ = world;
    selectionBox_ = {x, y, 0, 0};
}

void StudioApp::handleAction(const std::string& action) {
    if (action == "new") {
        modal_ = Modal{ModalAction::NewProject, "NEW PROJECT NAME",
                       "Untitled", {}, PropertyKind::Text, {}};
        SDL_StartTextInput();
    } else if (action == "open") {
        modal_ = Modal{ModalAction::OpenProject, "OPEN PROJECT PATH",
                       "examples/mixed_signal_demo.oopproteus.json", {},
                       PropertyKind::Text, {}};
        SDL_StartTextInput();
    } else if (action == "save") {
        if (currentPath_.empty()) {
            handleAction("save_as");
        } else {
            saveProject(currentPath_);
        }
    } else if (action == "save_as") {
        modal_ = Modal{
            ModalAction::SaveProjectAs, "SAVE PROJECT AS",
            currentPath_.empty()
                ? "projects/" + circuit_.projectName()
                    + ".oopproteus.json"
                : currentPath_,
            {}, PropertyKind::Text, {}};
        SDL_StartTextInput();
    } else if (action == "export") {
        modal_ = Modal{ModalAction::ExportImage, "EXPORT PNG PATH",
                       "exports/" + circuit_.projectName() + "_"
                           + timeStampForFile() + ".png",
                       {}, PropertyKind::Text, {}};
        SDL_StartTextInput();
    } else if (action == "undo") {
        undo();
    } else if (action == "redo") {
        redo();
    } else if (action == "select") {
        setTool(Tool::Select);
    } else if (action == "wire") {
        setTool(Tool::Wire);
    } else if (action == "junction") {
        setTool(Tool::Junction);
    } else if (action == "probe") {
        setTool(Tool::Probe);
    } else if (action == "pan") {
        setTool(Tool::Pan);
    } else if (action == "run") {
        runSimulation();
    } else if (action == "pause") {
        pauseSimulation();
    } else if (action == "stop") {
        stopSimulation();
    } else if (action == "step") {
        stepSimulation();
    } else if (action == "drc") {
        runDrc();
    } else if (action == "scope") {
        toggleScope();
    } else if (action == "delete") {
        deleteSelection();
    } else if (action == "rotate") {
        rotateSelection();
    } else if (action == "mirror_h") {
        mirrorSelection(true);
    } else if (action == "mirror_v") {
        mirrorSelection(false);
    } else if (action == "toggle_grid") {
        showGrid_ = !showGrid_;
    } else if (action == "toggle_log") {
        showLog_ = !showLog_;
        updateLayout();
    } else if (action == "reset_camera") {
        resetCamera();
    } else if (action == "fit_canvas") {
        const auto size = circuit_.canvasSize();
        camera_.zoom =
            std::clamp(std::min(canvasRect_.w / (size.x + 80.0),
                                canvasRect_.h / (size.y + 80.0)),
                       0.25, 3.5);
        camera_.x = -40.0;
        camera_.y = -40.0;
    } else if (action == "welcome") {
        screen_ = Screen::Welcome;
    } else if (action == "exit") {
        running_ = false;
    }
}

void StudioApp::acceptModal() {
    if (!modal_) return;
    const auto modal = *modal_;
    modal_.reset();
    SDL_StopTextInput();
    switch (modal.action) {
    case ModalAction::NewProject: newProject(modal.value); break;
    case ModalAction::OpenProject: openProject(modal.value); break;
    case ModalAction::SaveProjectAs:
        if (std::filesystem::exists(modal.value)
            && modal.value != currentPath_) {
            modal_ = Modal{
                ModalAction::ConfirmOverwrite,
                "FILE EXISTS - TYPE OVERWRITE TO REPLACE IT",
                {}, modal.value, PropertyKind::Text, {}};
            SDL_StartTextInput();
        } else {
            saveProject(modal.value);
        }
        break;
    case ModalAction::ConfirmOverwrite:
        if (lowerCopy(modal.value) == "overwrite") {
            saveProject(modal.propertyKey);
        } else {
            setToast("OVERWRITE CANCELLED", colors::warning);
        }
        break;
    case ModalAction::ExportImage: exportImage(modal.value); break;
    case ModalAction::EditLabel:
        applyProperty("__label__", PropertyKind::Text, modal.value);
        break;
    case ModalAction::EditProperty:
        applyProperty(modal.propertyKey, modal.propertyKind, modal.value);
        break;
    case ModalAction::None: break;
    }
}

void StudioApp::cancelModal() {
    modal_.reset();
    SDL_StopTextInput();
}

std::optional<std::string> StudioApp::toolbarAt(int x, int y) const {
    for (const auto& button : toolbarButtons()) {
        if (button.enabled && contains(button.rect, x, y)) return button.id;
    }
    return std::nullopt;
}

std::optional<std::string> StudioApp::menuAt(int x, int y) const {
    if (!contains(menuBarRect_, x, y)) return std::nullopt;
    const std::array<std::string, 4> menus = {"FILE", "EDIT", "SIM", "VIEW"};
    int menuX = 12;
    for (const auto& menu : menus) {
        if (contains({menuX, 3, 70, 24}, x, y)) return menu;
        menuX += 72;
    }
    return std::nullopt;
}

std::optional<std::string> StudioApp::dropdownActionAt(int x, int y) const {
    if (openMenu_.empty()) return std::nullopt;
    auto panelX = 12;
    if (openMenu_ == "EDIT") panelX += 72;
    if (openMenu_ == "SIM") panelX += 144;
    if (openMenu_ == "VIEW") panelX += 216;
    const auto items = menuItems(openMenu_);
    auto rowY = menuBarRect_.h + 4;
    for (const auto& item : items) {
        if (item.enabled && contains({panelX + 4, rowY, 227, 28}, x, y)) {
            return item.action;
        }
        rowY += 30;
    }
    return std::nullopt;
}

std::vector<StudioApp::CatalogEntry> StudioApp::filteredCatalog() const {
    std::vector<CatalogEntry> result;
    const auto query = lowerCopy(searchText_);
    for (const auto& entry : catalog_) {
        if (categoryFilter_ != "All"
            && entry.category != categoryFilter_) {
            continue;
        }
        if (!query.empty()) {
            const auto haystack =
                lowerCopy(entry.name + " " + entry.type + " "
                          + entry.category);
            if (haystack.find(query) == std::string::npos) continue;
        }
        result.push_back(entry);
    }
    return result;
}

std::optional<std::size_t> StudioApp::libraryRowAt(int x, int y) const {
    const auto listTop = categoryRect_.y + categoryRect_.h + 12;
    const auto listBottom = libraryRect_.y + libraryRect_.h - 100;
    if (x < libraryRect_.x + 9 || x >= libraryRect_.x + libraryRect_.w - 9
        || y < listTop || y >= listBottom) {
        return std::nullopt;
    }
    const auto row = (y - listTop + libraryScroll_) / 42;
    if (row < 0) return std::nullopt;
    return static_cast<std::size_t>(row);
}

std::optional<std::pair<std::string, std::string>>
StudioApp::propertyAt(int x, int y) const {
    const auto* component = selectedComponent();
    if (!component || !contains(propertyRowsRect_, x, y)) {
        return std::nullopt;
    }
    auto rowY = propertyRowsRect_.y - propertyScroll_ + 25;
    if (contains({propertyRowsRect_.x, rowY, propertyRowsRect_.w, 42},
                 x, y)) {
        return std::pair{component->id(), std::string("__label__")};
    }
    rowY += 46;
    for (const auto& definition : component->propertyDefinitions()) {
        if (contains({propertyRowsRect_.x, rowY,
                      propertyRowsRect_.w, 48},
                     x, y)) {
            return std::pair{component->id(), definition.key};
        }
        rowY += 52;
    }
    return std::nullopt;
}

} // namespace proteus::ui
