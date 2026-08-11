#include "proteus/ui/StudioApp.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>

namespace proteus::ui {
namespace {

void catalogIcon(SDL_Renderer* renderer, const std::string& type,
                 const SDL_Rect& rect, Color color) {
    const auto cx = rect.x + rect.w / 2;
    const auto cy = rect.y + rect.h / 2;
    if (type == "ground") {
        line(renderer, cx, rect.y + 4, cx, cy, color, 2);
        line(renderer, cx - 10, cy, cx + 10, cy, color, 2);
        line(renderer, cx - 7, cy + 4, cx + 7, cy + 4, color);
        line(renderer, cx - 4, cy + 8, cx + 4, cy + 8, color);
    } else if (type == "resistor") {
        line(renderer, rect.x + 2, cy, rect.x + 7, cy, color);
        SDL_Point points[] = {
            {rect.x + 7, cy},      {rect.x + 10, cy - 6},
            {rect.x + 14, cy + 6}, {rect.x + 18, cy - 6},
            {rect.x + 22, cy + 6}, {rect.x + 25, cy},
            {rect.x + 30, cy}};
        setColor(renderer, color);
        SDL_RenderDrawLines(renderer, points, 7);
    } else if (type == "capacitor") {
        line(renderer, rect.x + 2, cy, cx - 3, cy, color);
        line(renderer, cx - 3, cy - 9, cx - 3, cy + 9, color, 2);
        line(renderer, cx + 3, cy - 9, cx + 3, cy + 9, color, 2);
        line(renderer, cx + 3, cy, rect.x + rect.w - 2, cy, color);
    } else if (type == "inductor") {
        line(renderer, rect.x + 2, cy, rect.x + 7, cy, color);
        for (int index = 0; index < 4; ++index) {
            circle(renderer, rect.x + 10 + index * 5, cy, 4, color);
        }
        line(renderer, rect.x + 26, cy, rect.x + rect.w - 2, cy, color);
    } else if (type == "dc_source" || type == "battery"
               || type == "clock") {
        circle(renderer, cx, cy, 11, color);
        if (type == "clock") {
            line(renderer, cx - 7, cy + 4, cx - 7, cy - 4, color);
            line(renderer, cx - 7, cy - 4, cx, cy - 4, color);
            line(renderer, cx, cy - 4, cx, cy + 4, color);
            line(renderer, cx, cy + 4, cx + 7, cy + 4, color);
        } else {
            text(renderer, cx - 3, cy - 10, "+", color, 1);
            text(renderer, cx - 3, cy + 2, "-", color, 1);
        }
    } else if (type == "switch" || type == "push_button") {
        circle(renderer, rect.x + 7, cy + 5, 2, color, true);
        circle(renderer, rect.x + rect.w - 7, cy + 5, 2, color, true);
        line(renderer, rect.x + 8, cy + 4, rect.x + rect.w - 9, cy - 6,
             color, 2);
    } else if (type == "led") {
        line(renderer, cx - 8, cy - 8, cx - 8, cy + 8, color, 2);
        line(renderer, cx + 7, cy - 8, cx + 7, cy + 8, color, 2);
        line(renderer, cx - 8, cy - 8, cx + 7, cy, color);
        line(renderer, cx - 8, cy + 8, cx + 7, cy, color);
    } else {
        fillRect(renderer, {rect.x + 4, rect.y + 4, rect.w - 8, rect.h - 8},
                 withAlpha(color, 32));
        strokeRect(renderer,
                   {rect.x + 4, rect.y + 4, rect.w - 8, rect.h - 8},
                   color);
        const auto abbreviation =
            type == "microcontroller" ? "MCU"
            : type == "external_memory" ? "MEM"
            : type == "seven_segment"   ? "7S"
            : type == "d_flip_flop"     ? "DFF"
            : type == "lcd_16x2"        ? "LCD"
            : type == "keypad_4x4"      ? "KEY"
            : type == "voltmeter"       ? "V"
            : type == "ammeter"         ? "A"
            : type == "and_gate"        ? "AND"
            : type == "or_gate"         ? "OR"
            : type == "not_gate"        ? "NOT"
            : type == "xor_gate"        ? "XOR"
            : type == "nand_gate"       ? "NAND"
            : type == "adc"             ? "ADC"
            : type == "dac"             ? "DAC"
                                        : "DEV";
        textCentered(renderer, rect, abbreviation, color, 1);
    }
}

Color severityColor(IssueSeverity severity) {
    switch (severity) {
    case IssueSeverity::Information: return colors::accentBlue;
    case IssueSeverity::Warning: return colors::warning;
    case IssueSeverity::Error: return colors::error;
    }
    return colors::text;
}

} // namespace

void StudioApp::renderWelcome() {
    for (int x = -20; x < windowWidth_; x += 34) {
        line(renderer_, x, 0, x, windowHeight_, {30, 52, 75, 70});
    }
    for (int y = 0; y < windowHeight_; y += 34) {
        line(renderer_, 0, y, windowWidth_, y, {30, 52, 75, 70});
    }
    const SDL_Rect card{windowWidth_ / 2 - 380, windowHeight_ / 2 - 290,
                        760, 580};
    roundedPanel(renderer_, card, {17, 28, 43, 248}, colors::border, 12);
    fillRect(renderer_, {card.x, card.y, 8, card.h}, colors::accent);

    text(renderer_, card.x + 46, card.y + 46, "PROTEUSLAB SDL",
         colors::accent, 4);
    text(renderer_, card.x + 48, card.y + 86,
         "C++20 CIRCUIT DESIGN AND SIMULATION STUDIO", colors::muted, 2);
    text(renderer_, card.x + 48, card.y + 126,
         "FRONTEND: SDL2   BACKEND: INDEPENDENT OOP CORE",
         colors::accentBlue, 1);

    const SDL_Rect newButton{card.x + 48, card.y + 172, 205, 54};
    const SDL_Rect openButton{card.x + 270, card.y + 172, 205, 54};
    const SDL_Rect demoButton{card.x + 492, card.y + 172, 205, 54};
    roundedPanel(renderer_, newButton, colors::accent,
                 mix(colors::accent, colors::text, 0.3F));
    textCentered(renderer_, newButton, "NEW PROJECT",
                 {6, 28, 24, 255}, 2);
    roundedPanel(renderer_, openButton, colors::panelLight,
                 colors::accentBlue);
    textCentered(renderer_, openButton, "OPEN PROJECT", colors::text, 2);
    roundedPanel(renderer_, demoButton, colors::panelLight,
                 colors::warning);
    textCentered(renderer_, demoButton, "OPEN DEMO", colors::text, 2);

    text(renderer_, card.x + 48, card.y + 258, "RECENT PROJECTS",
         colors::text, 2);
    line(renderer_, card.x + 48, card.y + 282, card.x + card.w - 48,
         card.y + 282, colors::border);
    if (recentProjects_.empty()) {
        text(renderer_, card.x + 48, card.y + 310,
             "NO RECENT PROJECTS YET", colors::muted, 2);
    } else {
        const auto count = std::min<std::size_t>(recentProjects_.size(), 6);
        for (std::size_t index = 0; index < count; ++index) {
            const SDL_Rect row{
                card.x + 48, card.y + 300 + static_cast<int>(index) * 38,
                card.w - 96, 32};
            fillRect(renderer_, row,
                     index % 2 == 0 ? Color{22, 36, 53, 255}
                                    : Color{18, 31, 47, 255});
            text(renderer_, row.x + 10, row.y + 9,
                 clippedText(recentProjects_[index], row.w - 20, 1),
                 colors::muted, 1);
        }
    }
    text(renderer_, card.x + 48, card.y + card.h - 36,
         "TIP: CTRL+O OPEN   CTRL+S SAVE   W WIRE   R ROTATE",
         colors::muted, 1);
}

void StudioApp::renderEditor() {
    renderTopMenu();
    renderToolbar();
    renderLibrary();
    renderCanvas();
    renderProperties();
    if (showLog_) renderLog();
    renderStatusBar();
    if (!openMenu_.empty()) renderDropdownMenu();
    if (showScope_) renderScope();
}

void StudioApp::renderTopMenu() {
    fillRect(renderer_, menuBarRect_, {12, 21, 33, 255});
    line(renderer_, 0, menuBarRect_.h - 1, windowWidth_,
         menuBarRect_.h - 1, colors::border);
    const std::array<std::string, 4> menus = {"FILE", "EDIT", "SIM", "VIEW"};
    int x = 12;
    for (const auto& menu : menus) {
        SDL_Rect rect{x, 3, 70, 24};
        if (openMenu_ == menu) fillRect(renderer_, rect, colors::panelLight);
        textCentered(renderer_, rect, menu,
                     openMenu_ == menu ? colors::accent : colors::text, 1);
        x += 72;
    }
    const auto title =
        clippedText(circuit_.projectName()
                        + (documentModified_ ? " *" : "")
                        + " - SDL2",
                    windowWidth_ / 2, 1);
    text(renderer_, windowWidth_ - textWidth(title, 1) - 15, 10, title,
         colors::muted, 1);
}

void StudioApp::renderToolbar() {
    fillRect(renderer_, toolbarRect_, colors::panel);
    line(renderer_, 0, toolbarRect_.y + toolbarRect_.h - 1, windowWidth_,
         toolbarRect_.y + toolbarRect_.h - 1, colors::border);
    for (const auto& button : toolbarButtons()) {
        auto fill = button.active ? Color{34, 91, 79, 255}
                                  : colors::panelLight;
        auto border = button.active ? colors::accent : colors::border;
        auto foreground = button.enabled ? colors::text : Color{80, 98, 116,
                                                                 255};
        roundedPanel(renderer_, button.rect, fill, border, 5);
        textCentered(renderer_, button.rect, button.label, foreground, 1);
    }
}

void StudioApp::renderLibrary() {
    fillRect(renderer_, libraryRect_, colors::panel);
    line(renderer_, libraryRect_.x + libraryRect_.w - 1, libraryRect_.y,
         libraryRect_.x + libraryRect_.w - 1,
         libraryRect_.y + libraryRect_.h, colors::border);
    text(renderer_, libraryRect_.x + 14, libraryRect_.y + 15,
         "COMPONENT LIBRARY", colors::text, 2);

    fillRect(renderer_, searchRect_,
             searchFocused_ ? Color{28, 48, 66, 255}
                            : Color{14, 25, 39, 255});
    strokeRect(renderer_, searchRect_,
               searchFocused_ ? colors::accentBlue : colors::border);
    const auto searchDisplay =
        searchText_.empty() ? std::string("SEARCH...")
                            : clippedText(searchText_, searchRect_.w - 20, 1);
    text(renderer_, searchRect_.x + 9, searchRect_.y + 11, searchDisplay,
         searchText_.empty() ? colors::muted : colors::text, 1);
    if (searchFocused_
        && (SDL_GetTicks64() / 450U) % 2U == 0U) {
        const auto cursorX =
            searchRect_.x + 9
            + textWidth(clippedText(searchText_, searchRect_.w - 30, 1), 1);
        line(renderer_, cursorX, searchRect_.y + 7, cursorX,
             searchRect_.y + searchRect_.h - 7, colors::accentBlue);
    }

    fillRect(renderer_, categoryRect_, {14, 25, 39, 255});
    strokeRect(renderer_, categoryRect_, colors::border);
    text(renderer_, categoryRect_.x + 9, categoryRect_.y + 10,
         clippedText("CATEGORY: " + categoryFilter_, categoryRect_.w - 25, 1),
         colors::muted, 1);
    text(renderer_, categoryRect_.x + categoryRect_.w - 16,
         categoryRect_.y + 10, "V", colors::accent, 1);

    const auto entries = filteredCatalog();
    const auto rowHeight = 42;
    const auto listTop = categoryRect_.y + categoryRect_.h + 12;
    const auto listBottom = libraryRect_.y + libraryRect_.h - 100;
    SDL_Rect clip{libraryRect_.x + 1, listTop, libraryRect_.w - 2,
                  std::max(0, listBottom - listTop)};
    SDL_RenderSetClipRect(renderer_, &clip);
    for (std::size_t index = 0; index < entries.size(); ++index) {
        const auto y = listTop + static_cast<int>(index) * rowHeight
            - libraryScroll_;
        if (y + rowHeight < listTop || y > listBottom) continue;
        SDL_Rect row{libraryRect_.x + 9, y, libraryRect_.w - 18,
                     rowHeight - 4};
        const auto active =
            placeType_ == entries[index].type
            || draggedLibraryType_ == entries[index].type;
        fillRect(renderer_, row,
                 active ? Color{30, 73, 65, 255}
                        : (index % 2 == 0 ? Color{21, 34, 51, 255}
                                         : Color{17, 29, 44, 255}));
        strokeRect(renderer_, row,
                   active ? colors::accent : Color{35, 51, 69, 255});
        SDL_Rect icon{row.x + 5, row.y + 5, 34, row.h - 10};
        catalogIcon(renderer_, entries[index].type, icon,
                    active ? colors::accent : colors::accentBlue);
        text(renderer_, row.x + 47, row.y + 7,
             clippedText(entries[index].name, row.w - 55, 1), colors::text,
             1);
        text(renderer_, row.x + 47, row.y + 22,
             clippedText(entries[index].category, row.w - 55, 1),
             colors::muted, 1);
    }
    SDL_RenderSetClipRect(renderer_, nullptr);

    SDL_Rect preview{libraryRect_.x + 10,
                     libraryRect_.y + libraryRect_.h - 90,
                     libraryRect_.w - 20, 78};
    fillRect(renderer_, preview, {12, 23, 35, 255});
    strokeRect(renderer_, preview, colors::border);
    text(renderer_, preview.x + 8, preview.y + 8, "PREVIEW",
         colors::muted, 1);
    if (!placeType_.empty()) {
        const auto found = std::find_if(
            catalog_.begin(), catalog_.end(), [this](const auto& entry) {
                return entry.type == placeType_;
            });
        if (found != catalog_.end()) {
            SDL_Rect icon{preview.x + 8, preview.y + 27, 42, 38};
            catalogIcon(renderer_, found->type, icon, colors::accent);
            text(renderer_, preview.x + 58, preview.y + 29,
                 clippedText(found->name, preview.w - 65, 1), colors::text,
                 1);
            text(renderer_, preview.x + 58, preview.y + 47,
                 "CLICK OR DRAG TO CANVAS", colors::muted, 1);
        }
    } else {
        text(renderer_, preview.x + 8, preview.y + 40,
             "SELECT A COMPONENT", colors::muted, 1);
    }
}

void StudioApp::renderCanvas() {
    SDL_RenderSetClipRect(renderer_, &canvasRect_);
    fillRect(renderer_, canvasRect_, {8, 14, 24, 255});
    if (showGrid_) renderGrid();
    renderWires();
    renderComponents();

    if (wireStart_) {
        const auto* component = circuit_.component(wireStart_->componentId);
        const auto* pin =
            component ? component->findPin(wireStart_->pinId) : nullptr;
        if (component && pin) {
            const auto start =
                worldToScreen(pinWorldPosition(*component, *pin));
            const auto end = worldToScreen(snap(screenToWorld(mouseX_, mouseY_)));
            line(renderer_, start.x, start.y, start.x, end.y, colors::warning,
                 2);
            line(renderer_, start.x, end.y, end.x, end.y, colors::warning, 2);
            circle(renderer_, end.x, end.y, 4, colors::warning);
        }
    }
    if (selectingBox_) {
        fillRect(renderer_, selectionBox_,
                 withAlpha(colors::accentBlue, 30));
        strokeRect(renderer_, selectionBox_, colors::accentBlue);
    }
    if (libraryDragging_ && !draggedLibraryType_.empty()
        && contains(canvasRect_, mouseX_, mouseY_)) {
        SDL_Rect ghost{mouseX_ - 45, mouseY_ - 30, 90, 60};
        fillRect(renderer_, ghost, withAlpha(colors::accent, 35));
        strokeRect(renderer_, ghost, colors::accent, 2);
        textCentered(renderer_, ghost,
                     clippedText(draggedLibraryType_, ghost.w - 5, 1),
                     colors::accent, 1);
    }
    SDL_RenderSetClipRect(renderer_, nullptr);
    strokeRect(renderer_, canvasRect_, colors::border);
}

void StudioApp::renderGrid() {
    const auto grid =
        std::max(4.0, circuit_.gridSize() * camera_.zoom);
    const auto worldTopLeft = screenToWorld(canvasRect_.x, canvasRect_.y);
    const auto firstWorldX =
        std::floor(worldTopLeft.x / circuit_.gridSize())
        * circuit_.gridSize();
    const auto firstWorldY =
        std::floor(worldTopLeft.y / circuit_.gridSize())
        * circuit_.gridSize();
    int lineNumber = static_cast<int>(
        std::round(firstWorldX / circuit_.gridSize()));
    for (double worldX = firstWorldX;; worldX += circuit_.gridSize()) {
        const auto x = worldToScreen({worldX, 0.0}).x;
        if (x > canvasRect_.x + canvasRect_.w) break;
        if (x >= canvasRect_.x) {
            const auto major = lineNumber % 5 == 0;
            line(renderer_, x, canvasRect_.y, x,
                 canvasRect_.y + canvasRect_.h,
                 major ? Color{55, 79, 103, 64}
                       : Color{46, 67, 89, 32});
        }
        ++lineNumber;
        if (grid < 6.0) break;
    }
    lineNumber = static_cast<int>(
        std::round(firstWorldY / circuit_.gridSize()));
    for (double worldY = firstWorldY;; worldY += circuit_.gridSize()) {
        const auto y = worldToScreen({0.0, worldY}).y;
        if (y > canvasRect_.y + canvasRect_.h) break;
        if (y >= canvasRect_.y) {
            const auto major = lineNumber % 5 == 0;
            line(renderer_, canvasRect_.x, y,
                 canvasRect_.x + canvasRect_.w, y,
                 major ? Color{55, 79, 103, 64}
                       : Color{46, 67, 89, 32});
        }
        ++lineNumber;
        if (grid < 6.0) break;
    }
}

void StudioApp::renderWires() {
    for (const auto& [id, wire] : circuit_.wires()) {
        const auto path = wirePath(wire);
        const auto color =
            selectedWires_.contains(id) ? colors::accent : wireColor(wire);
        for (std::size_t index = 1; index < path.size(); ++index) {
            const auto first = worldToScreen(path[index - 1]);
            const auto second = worldToScreen(path[index]);
            line(renderer_, first.x, first.y, second.x, second.y, color,
                 selectedWires_.contains(id) ? 4 : 3);
        }
    }
    for (const auto& [id, junction] : circuit_.junctions()) {
        static_cast<void>(id);
        const auto point = worldToScreen(junction.position);
        circle(renderer_, point.x, point.y,
               std::max(4, static_cast<int>(5 * camera_.zoom)),
               colors::accent, true);
    }
}

void StudioApp::renderComponents() {
    for (auto* component : circuit_.components()) {
        renderComponent(*component);
    }
}

void StudioApp::renderComponent(Component& component) {
    const auto rect = componentScreenRect(component);
    if (!intersects(rect, canvasRect_)) return;
    const auto selected = selectedComponents_.contains(component.id());
    const auto type = component.typeId();
    auto bodyFill = Color{18, 34, 49, 255};
    if (type == "lcd_16x2") bodyFill = {29, 55, 46, 255};
    if (type == "microcontroller") bodyFill = {31, 38, 65, 255};
    if (type == "external_memory") bodyFill = {51, 38, 58, 255};
    if (type == "led" && component.runtimeValue("lit", false)) {
        bodyFill = {82, 31, 38, 255};
    }
    fillRect(renderer_, rect, bodyFill);
    strokeRect(renderer_, rect,
               selected ? colors::accent : colors::border,
               selected ? 3 : 1);

    const auto cx = rect.x + rect.w / 2;
    const auto cy = rect.y + rect.h / 2;
    const auto symbolColor =
        selected ? colors::accent : Color{194, 218, 233, 255};
    if (type == "ground") {
        fillRect(renderer_, rect, {8, 14, 24, 255});
        line(renderer_, cx, rect.y + 3, cx, cy - 2, symbolColor, 2);
        line(renderer_, cx - 15, cy, cx + 15, cy, symbolColor, 2);
        line(renderer_, cx - 10, cy + 7, cx + 10, cy + 7, symbolColor, 2);
        line(renderer_, cx - 5, cy + 14, cx + 5, cy + 14, symbolColor, 2);
    } else if (type == "dc_source" || type == "battery"
               || type == "clock") {
        circle(renderer_, cx, cy, std::max(13, rect.h / 3), symbolColor);
        if (type == "clock") {
            line(renderer_, cx - 12, cy + 6, cx - 12, cy - 7,
                 symbolColor, 2);
            line(renderer_, cx - 12, cy - 7, cx, cy - 7, symbolColor, 2);
            line(renderer_, cx, cy - 7, cx, cy + 6, symbolColor, 2);
            line(renderer_, cx, cy + 6, cx + 12, cy + 6, symbolColor, 2);
        } else {
            text(renderer_, cx - 4, cy - 18, "+", symbolColor, 1);
            text(renderer_, cx - 4, cy + 9, "-", symbolColor, 1);
        }
    } else if (type == "resistor" || type == "potentiometer") {
        SDL_Point points[] = {
            {rect.x + 12, cy},     {rect.x + 20, cy - 11},
            {rect.x + 30, cy + 11}, {rect.x + 40, cy - 11},
            {rect.x + 50, cy + 11}, {rect.x + 60, cy - 11},
            {rect.x + rect.w - 12, cy}};
        setColor(renderer_, symbolColor);
        SDL_RenderDrawLines(renderer_, points, 7);
        if (type == "potentiometer") {
            line(renderer_, cx, rect.y + 8, cx, cy - 2, colors::warning, 2);
            line(renderer_, cx, cy - 2, cx - 4, cy - 8, colors::warning);
        }
    } else if (type == "capacitor") {
        line(renderer_, cx - 7, cy - 20, cx - 7, cy + 20, symbolColor, 3);
        line(renderer_, cx + 7, cy - 20, cx + 7, cy + 20, symbolColor, 3);
    } else if (type == "inductor") {
        for (int index = 0; index < 4; ++index) {
            circle(renderer_, cx - 18 + index * 12, cy, 8, symbolColor);
        }
    } else if (type == "switch" || type == "push_button") {
        circle(renderer_, rect.x + 19, cy + 10, 4, symbolColor, true);
        circle(renderer_, rect.x + rect.w - 19, cy + 10, 4, symbolColor,
               true);
        const auto closed = component.property("closed", false);
        line(renderer_, rect.x + 22, cy + 8, rect.x + rect.w - 22,
             closed ? cy + 8 : cy - 15, symbolColor, 3);
        if (type == "push_button") {
            line(renderer_, cx, rect.y + 7, cx, cy - 10, colors::warning, 2);
        }
    } else if (type == "led") {
        const auto lit = component.runtimeValue("lit", false);
        circle(renderer_, cx, cy, std::max(12, rect.h / 4),
               lit ? colors::highSignal : symbolColor, lit);
        line(renderer_, cx + 10, cy - 12, cx + 21, cy - 22,
             lit ? colors::warning : colors::muted, 2);
        line(renderer_, cx + 14, cy - 4, cx + 25, cy - 14,
             lit ? colors::warning : colors::muted, 2);
    } else if (type == "seven_segment") {
        const auto mask = component.runtimeValue("segments", 0);
        const std::array<SDL_Rect, 7> segments = {
            SDL_Rect{cx - 13, cy - 35, 26, 4},
            SDL_Rect{cx + 12, cy - 31, 4, 27},
            SDL_Rect{cx + 12, cy + 3, 4, 27},
            SDL_Rect{cx - 13, cy + 30, 26, 4},
            SDL_Rect{cx - 16, cy + 3, 4, 27},
            SDL_Rect{cx - 16, cy - 31, 4, 27},
            SDL_Rect{cx - 13, cy - 2, 26, 4}};
        for (std::size_t index = 0; index < segments.size(); ++index) {
            fillRect(renderer_, segments[index],
                     (mask & (1 << index)) ? colors::highSignal
                                           : Color{63, 37, 44, 255});
        }
    } else if (type == "lcd_16x2") {
        SDL_Rect display{rect.x + 10, rect.y + 16, rect.w - 20,
                         rect.h - 28};
        fillRect(renderer_, display, {74, 112, 55, 255});
        strokeRect(renderer_, display, {122, 159, 91, 255});
        text(renderer_, display.x + 7, display.y + 10,
             clippedText(component.runtimeValue(
                             "line1", std::string("                ")),
                         display.w - 14, 1),
             {16, 37, 20, 255}, 1);
        text(renderer_, display.x + 7, display.y + 29,
             clippedText(component.runtimeValue(
                             "line2", std::string("                ")),
                         display.w - 14, 1),
             {16, 37, 20, 255}, 1);
    } else if (type == "keypad_4x4") {
        const auto pressed = component.property("pressedKey", -1);
        const auto size = std::min(rect.w - 22, rect.h - 22) / 4;
        for (int row = 0; row < 4; ++row) {
            for (int column = 0; column < 4; ++column) {
                SDL_Rect key{rect.x + 11 + column * size,
                             rect.y + 11 + row * size, size - 3, size - 3};
                fillRect(renderer_, key,
                         pressed == row * 4 + column
                             ? Color{45, 116, 94, 255}
                             : Color{31, 48, 66, 255});
                strokeRect(renderer_, key,
                           pressed == row * 4 + column ? colors::accent
                                                       : colors::border);
                textCentered(renderer_, key,
                             std::to_string(row * 4 + column),
                             colors::text, 1);
            }
        }
    } else {
        auto title = component.displayName();
        if (type == "and_gate") title = "AND";
        if (type == "or_gate") title = "OR";
        if (type == "not_gate") title = "NOT";
        if (type == "xor_gate") title = "XOR";
        if (type == "nand_gate") title = "NAND";
        if (type == "d_flip_flop") title = "D FLIP-FLOP";
        if (type == "microcontroller") title = "MCU";
        if (type == "external_memory") title = "MEMORY";
        if (type == "voltmeter") title = "V";
        if (type == "ammeter") title = "A";
        textCentered(renderer_,
                     {rect.x + 5, rect.y + 5, rect.w - 10, rect.h - 10},
                     clippedText(title, rect.w - 16, type == "microcontroller"
                                                        ? 2
                                                        : 1),
                     symbolColor,
                     type == "microcontroller" ? 2 : 1);
        if (type == "microcontroller") {
            const auto pc = component.runtimeValue("pc", 0);
            text(renderer_, rect.x + 10, rect.y + rect.h - 22,
                 "PC=" + std::to_string(pc), colors::muted, 1);
        }
        if (type == "voltmeter" || type == "ammeter") {
            std::ostringstream reading;
            reading << std::fixed << std::setprecision(3)
                    << component.runtimeValue("reading", 0.0);
            text(renderer_, rect.x + 8, rect.y + rect.h - 19,
                 reading.str(), colors::warning, 1);
        }
    }

    for (const auto& pin : component.pins()) {
        const auto pinPoint = worldToScreen(pinWorldPosition(component, pin));
        const auto highlighted =
            hoveredPin_ && hoveredPin_->componentId == component.id()
            && hoveredPin_->pinId == pin.id;
        const auto connected = circuit_.isPinConnected(component.pinRef(pin.id));
        const auto pinColor =
            highlighted ? colors::warning
            : connected  ? colors::accent
                         : colors::muted;
        circle(renderer_, pinPoint.x, pinPoint.y,
               highlighted ? 6 : 4, pinColor,
               highlighted || connected);
        if (camera_.zoom >= 0.8) {
            const auto labelX =
                pinPoint.x < rect.x + rect.w / 2
                ? pinPoint.x + 7
                : pinPoint.x - textWidth(pin.label, 1) - 7;
            text(renderer_, labelX, pinPoint.y - 4, pin.label, pinColor, 1);
        }
    }

    const auto label = clippedText(component.label(), rect.w + 50, 1);
    text(renderer_, rect.x + (rect.w - textWidth(label, 1)) / 2,
         rect.y - 15, label, selected ? colors::accent : colors::muted, 1);
}

void StudioApp::renderProperties() {
    fillRect(renderer_, propertiesRect_, colors::panel);
    line(renderer_, propertiesRect_.x, propertiesRect_.y,
         propertiesRect_.x, propertiesRect_.y + propertiesRect_.h,
         colors::border);
    text(renderer_, propertiesRect_.x + 14, propertiesRect_.y + 15,
         "PROPERTIES", colors::text, 2);

    SDL_Rect runtimeToggle{propertiesRect_.x + 12,
                           propertiesRect_.y + 43,
                           propertiesRect_.w - 24, 27};
    fillRect(renderer_, runtimeToggle,
             saveRuntimeState_ ? Color{29, 75, 65, 255}
                               : Color{28, 39, 52, 255});
    strokeRect(renderer_, runtimeToggle,
               saveRuntimeState_ ? colors::accent : colors::border);
    textCentered(renderer_, runtimeToggle,
                 saveRuntimeState_ ? "SAVE RUNTIME: ON"
                                   : "SAVE RUNTIME: OFF",
                 saveRuntimeState_ ? colors::accent : colors::muted, 1);

    SDL_RenderSetClipRect(renderer_, &propertyRowsRect_);
    auto y = propertyRowsRect_.y - propertyScroll_;
    if (selectedComponents_.empty()) {
        text(renderer_, propertyRowsRect_.x + 4, y + 12,
             "SELECT ONE COMPONENT", colors::muted, 1);
        text(renderer_, propertyRowsRect_.x + 4, y + 32,
             "TO EDIT ITS VALUES", colors::muted, 1);
    } else if (selectedComponents_.size() > 1) {
        text(renderer_, propertyRowsRect_.x + 4, y + 12,
             std::to_string(selectedComponents_.size())
                 + " COMPONENTS SELECTED",
             colors::accent, 1);
        text(renderer_, propertyRowsRect_.x + 4, y + 34,
             "R: ROTATE   H/V: MIRROR", colors::muted, 1);
    } else if (auto* component = selectedComponent()) {
        text(renderer_, propertyRowsRect_.x + 4, y + 4,
             clippedText(component->displayName(),
                         propertyRowsRect_.w - 8, 1),
             colors::accentBlue, 1);
        y += 25;

        SDL_Rect labelRow{propertyRowsRect_.x, y, propertyRowsRect_.w, 42};
        fillRect(renderer_, labelRow, {16, 28, 43, 255});
        text(renderer_, labelRow.x + 7, labelRow.y + 6, "LABEL",
             colors::muted, 1);
        text(renderer_, labelRow.x + 7, labelRow.y + 23,
             clippedText(component->label(), labelRow.w - 14, 1),
             colors::text, 1);
        y += 46;

        for (const auto& definition : component->propertyDefinitions()) {
            SDL_Rect row{propertyRowsRect_.x, y, propertyRowsRect_.w, 48};
            fillRect(renderer_, row,
                     (y / 48) % 2 == 0 ? Color{17, 29, 44, 255}
                                       : Color{20, 33, 49, 255});
            text(renderer_, row.x + 7, row.y + 6,
                 clippedText(definition.title, row.w - 14, 1),
                 colors::muted, 1);
            const auto found = component->properties().find(definition.key);
            auto value = found == component->properties().end()
                ? std::string("-")
                : propertyValueText(found->second);
            if (!definition.unit.empty()) value += " " + definition.unit;
            text(renderer_, row.x + 7, row.y + 25,
                 clippedText(value, row.w - 14, 1),
                 definition.runtimeEditable ? colors::accent
                                            : colors::text,
                 1);
            strokeRect(renderer_, row, {38, 54, 71, 255});
            y += 52;
        }

        if (!component->runtimeState().empty()) {
            y += 5;
            text(renderer_, propertyRowsRect_.x + 4, y,
                 "RUNTIME VALUES", colors::warning, 1);
            y += 20;
            for (const auto& [key, value] : component->runtimeState()) {
                SDL_Rect row{propertyRowsRect_.x, y, propertyRowsRect_.w, 32};
                fillRect(renderer_, row, {24, 30, 42, 255});
                text(renderer_, row.x + 6, row.y + 10,
                     clippedText(key + ": " + propertyValueText(value),
                                 row.w - 12, 1),
                     colors::muted, 1);
                y += 34;
            }
        }
    }
    SDL_RenderSetClipRect(renderer_, nullptr);
}

void StudioApp::renderLog() {
    fillRect(renderer_, logRect_, {12, 22, 34, 255});
    line(renderer_, logRect_.x, logRect_.y, logRect_.x + logRect_.w,
         logRect_.y, colors::border);
    text(renderer_, logRect_.x + 10, logRect_.y + 9,
         "SIMULATION LOG", colors::text, 1);
    text(renderer_, logRect_.x + logRect_.w - 122, logRect_.y + 9,
         "CLICK X TO HIDE", colors::muted, 1);
    const auto& entries = engine_->logEntries();
    const auto visible = std::max(1, (logRect_.h - 32) / 20);
    const auto total = static_cast<int>(entries.size());
    const auto end = std::max(0, total - logScroll_);
    const auto start = std::max(0, end - visible);
    auto y = logRect_.y + 30;
    if (entries.empty()) {
        text(renderer_, logRect_.x + 10, y,
             "NO MESSAGES. RUN DRC OR START SIMULATION.", colors::muted, 1);
    }
    for (int index = start; index < end; ++index) {
        const auto& entry = entries[static_cast<std::size_t>(index)];
        std::ostringstream timestamp;
        timestamp << std::fixed << std::setprecision(4)
                  << entry.timeSeconds << "S";
        const auto color = severityColor(entry.severity);
        text(renderer_, logRect_.x + 10, y, timestamp.str(), colors::muted, 1);
        text(renderer_, logRect_.x + 95, y,
             clippedText(entry.source, 110, 1), color, 1);
        text(renderer_, logRect_.x + 215, y,
             clippedText(entry.message, logRect_.w - 225, 1),
             colors::text, 1);
        y += 20;
    }
}

void StudioApp::renderStatusBar() {
    fillRect(renderer_, statusRect_, {10, 19, 29, 255});
    line(renderer_, 0, statusRect_.y, windowWidth_, statusRect_.y,
         colors::border);
    const auto world = screenToWorld(mouseX_, mouseY_);
    std::ostringstream position;
    position << "X:" << static_cast<int>(std::round(world.x))
             << "  Y:" << static_cast<int>(std::round(world.y))
             << "  ZOOM:" << static_cast<int>(camera_.zoom * 100.0) << "%";
    text(renderer_, 10, statusRect_.y + 8, position.str(), colors::muted, 1);
    const auto toolText =
        tool_ == Tool::Select     ? std::string("SELECT")
        : tool_ == Tool::Wire     ? std::string("WIRE")
        : tool_ == Tool::Junction ? std::string("JUNCTION")
        : tool_ == Tool::Probe    ? std::string("PROBE")
        : tool_ == Tool::Pan      ? std::string("PAN")
                                  : std::string("PLACE");
    text(renderer_, 260, statusRect_.y + 8,
         "TOOL:" + toolText
             + (tool_ == Tool::Place ? "(" + placeType_ + ")" : ""),
         colors::muted, 1);
    text(renderer_, 520, statusRect_.y + 8,
         "SIM:" + simulationStateText(), engine_->state()
                     == SimulationState::Running
                 ? colors::accent
                 : colors::muted,
         1);
    std::ostringstream time;
    time << "T=" << std::fixed << std::setprecision(4)
         << engine_->timeSeconds() << "S";
    text(renderer_, 660, statusRect_.y + 8, time.str(), colors::muted, 1);
    const auto help = "LMB SELECT  MMB PAN  W WIRE  DEL DELETE  +/- ZOOM";
    text(renderer_, windowWidth_ - textWidth(help, 1) - 12,
         statusRect_.y + 8, help, colors::muted, 1);
}

void StudioApp::renderDropdownMenu() {
    const auto items = menuItems(openMenu_);
    if (items.empty()) return;
    auto menuX = 12;
    if (openMenu_ == "EDIT") menuX += 72;
    if (openMenu_ == "SIM") menuX += 144;
    if (openMenu_ == "VIEW") menuX += 216;
    SDL_Rect panel{menuX, menuBarRect_.h, 235,
                   static_cast<int>(items.size()) * 30 + 8};
    fillRect(renderer_, panel, {20, 32, 48, 255});
    strokeRect(renderer_, panel, colors::border, 2);
    auto y = panel.y + 4;
    for (const auto& item : items) {
        SDL_Rect row{panel.x + 4, y, panel.w - 8, 28};
        if (contains(row, mouseX_, mouseY_) && item.enabled) {
            fillRect(renderer_, row, {36, 58, 74, 255});
        }
        text(renderer_, row.x + 8, row.y + 10,
             clippedText(item.label, row.w - 16, 1),
             item.enabled ? colors::text : Color{81, 95, 109, 255}, 1);
        y += 30;
    }
}

void StudioApp::renderModal() {
    fillRect(renderer_, {0, 0, windowWidth_, windowHeight_},
             {3, 8, 14, 185});
    auto& modal = *modal_;
    modal.box = {windowWidth_ / 2 - 300, windowHeight_ / 2 - 105, 600, 210};
    roundedPanel(renderer_, modal.box, colors::panel, colors::accentBlue, 9);
    text(renderer_, modal.box.x + 24, modal.box.y + 24, modal.title,
         colors::text, 2);
    SDL_Rect input{modal.box.x + 24, modal.box.y + 70,
                   modal.box.w - 48, 42};
    fillRect(renderer_, input, {8, 18, 29, 255});
    strokeRect(renderer_, input, colors::accentBlue, 2);
    const auto shown = clippedText(modal.value, input.w - 20, 1);
    text(renderer_, input.x + 10, input.y + 15, shown, colors::text, 1);
    if ((SDL_GetTicks64() / 450U) % 2U == 0U) {
        const auto cursorX =
            input.x + 10 + textWidth(shown, 1);
        line(renderer_, cursorX, input.y + 9, cursorX,
             input.y + input.h - 9, colors::accentBlue);
    }
    SDL_Rect ok{modal.box.x + modal.box.w - 220,
                modal.box.y + modal.box.h - 58, 90, 34};
    SDL_Rect cancel{modal.box.x + modal.box.w - 118,
                    modal.box.y + modal.box.h - 58, 90, 34};
    fillRect(renderer_, ok, colors::accent);
    strokeRect(renderer_, ok, colors::accent);
    textCentered(renderer_, ok, "OK", {6, 28, 24, 255}, 1);
    fillRect(renderer_, cancel, colors::panelLight);
    strokeRect(renderer_, cancel, colors::border);
    textCentered(renderer_, cancel, "CANCEL", colors::text, 1);
    text(renderer_, modal.box.x + 24, modal.box.y + modal.box.h - 42,
         "ENTER: ACCEPT   ESC: CANCEL", colors::muted, 1);
}

void StudioApp::renderScope() {
    const SDL_Rect panel{canvasRect_.x + 40, canvasRect_.y + 40,
                         std::max(360, canvasRect_.w - 80),
                         std::max(250, canvasRect_.h - 80)};
    fillRect(renderer_, panel, {5, 12, 19, 245});
    strokeRect(renderer_, panel, colors::accentBlue, 2);
    text(renderer_, panel.x + 14, panel.y + 12, "OSCILLOSCOPE",
         colors::accentBlue, 2);
    SDL_Rect timeMinus{panel.x + 14, panel.y + 42, 28, 28};
    SDL_Rect timePlus{panel.x + 218, panel.y + 42, 28, 28};
    SDL_Rect voltsMinus{panel.x + 270, panel.y + 42, 28, 28};
    SDL_Rect voltsPlus{panel.x + 474, panel.y + 42, 28, 28};
    SDL_Rect close{panel.x + panel.w - 95, panel.y + 8, 82, 28};
    fillRect(renderer_, timeMinus, colors::panelLight);
    fillRect(renderer_, timePlus, colors::panelLight);
    fillRect(renderer_, voltsMinus, colors::panelLight);
    fillRect(renderer_, voltsPlus, colors::panelLight);
    fillRect(renderer_, close, colors::panelLight);
    strokeRect(renderer_, timeMinus, colors::border);
    strokeRect(renderer_, timePlus, colors::border);
    strokeRect(renderer_, voltsMinus, colors::border);
    strokeRect(renderer_, voltsPlus, colors::border);
    strokeRect(renderer_, close, colors::border);
    textCentered(renderer_, timeMinus, "-", colors::text, 1);
    textCentered(renderer_, timePlus, "+", colors::text, 1);
    textCentered(renderer_, voltsMinus, "-", colors::text, 1);
    textCentered(renderer_, voltsPlus, "+", colors::text, 1);
    textCentered(renderer_, close, "CLOSE", colors::text, 1);
    std::ostringstream timeDiv;
    timeDiv << "TIME/DIV " << scopeTimePerDivision_ << "S";
    std::ostringstream voltsDiv;
    voltsDiv << "VOLTS/DIV " << scopeVoltsPerDivision_ << "V";
    text(renderer_, timeMinus.x + timeMinus.w + 6, panel.y + 52,
         timeDiv.str(), colors::muted, 1);
    text(renderer_, voltsMinus.x + voltsMinus.w + 6, panel.y + 52,
         voltsDiv.str(), colors::muted, 1);
    const SDL_Rect plot{panel.x + 52, panel.y + 82, panel.w - 75,
                        panel.h - 118};
    fillRect(renderer_, plot, {3, 10, 14, 255});
    strokeRect(renderer_, plot, colors::border);
    for (int lineIndex = 1; lineIndex < 10; ++lineIndex) {
        const auto x = plot.x + plot.w * lineIndex / 10;
        line(renderer_, x, plot.y, x, plot.y + plot.h,
             {34, 66, 65, 90});
    }
    for (int lineIndex = 1; lineIndex < 8; ++lineIndex) {
        const auto y = plot.y + plot.h * lineIndex / 8;
        line(renderer_, plot.x, y, plot.x + plot.w, y,
             {34, 66, 65, 90});
    }

    const auto& samples = engine_->scopeSamples();
    if (probeChannels_.empty()) {
        textCentered(renderer_, plot,
                     "NO CHANNELS. SELECT PROBE TOOL AND CLICK A PIN.",
                     colors::muted, 1);
        return;
    }
    if (samples.size() < 2) {
        textCentered(renderer_, plot,
                     "RUN OR STEP SIMULATION TO COLLECT SAMPLES.",
                     colors::muted, 1);
        return;
    }
    auto firstIndex = samples.size() > 500 ? samples.size() - 500 : 0;
    const auto minimumTime =
        samples.back().timeSeconds - scopeTimePerDivision_ * 10.0;
    while (firstIndex + 1 < samples.size()
           && samples[firstIndex].timeSeconds < minimumTime) {
        ++firstIndex;
    }
    const auto minimum = -scopeVoltsPerDivision_;
    const auto maximum = scopeVoltsPerDivision_ * 7.0;
    const std::array<Color, 6> channelColors = {
        colors::accent, colors::warning, colors::accentBlue,
        colors::error, Color{190, 110, 255, 255},
        Color{90, 220, 225, 255}};
    std::size_t channelIndex = 0;
    for (const auto& [channel, pin] : probeChannels_) {
        static_cast<void>(pin);
        const auto color =
            channelColors[channelIndex++ % channelColors.size()];
        bool havePrevious = false;
        SDL_Point previous{};
        for (std::size_t index = firstIndex; index < samples.size(); ++index) {
            const auto found =
                samples[index].channelVoltages.find(channel);
            if (found == samples[index].channelVoltages.end()
                || !std::isfinite(found->second)) {
                havePrevious = false;
                continue;
            }
            const auto denominator =
                std::max<std::size_t>(1, samples.size() - firstIndex - 1);
            SDL_Point point{
                plot.x
                    + static_cast<int>(
                        (index - firstIndex) * static_cast<std::size_t>(plot.w)
                        / denominator),
                plot.y + plot.h
                    - static_cast<int>(
                        (found->second - minimum) / (maximum - minimum)
                        * plot.h)};
            if (havePrevious) {
                line(renderer_, previous.x, previous.y, point.x, point.y,
                     color, 2);
            }
            previous = point;
            havePrevious = true;
        }
        text(renderer_, panel.x + 14,
             panel.y + 50 + static_cast<int>(channelIndex) * 18,
             channel, color, 1);
    }
    std::ostringstream maxLabel;
    maxLabel << std::fixed << std::setprecision(2) << maximum << "V";
    std::ostringstream minLabel;
    minLabel << std::fixed << std::setprecision(2) << minimum << "V";
    text(renderer_, plot.x - textWidth(maxLabel.str(), 1) - 5, plot.y,
         maxLabel.str(), colors::muted, 1);
    text(renderer_, plot.x - textWidth(minLabel.str(), 1) - 5,
         plot.y + plot.h - 7, minLabel.str(), colors::muted, 1);
}

void StudioApp::renderToast() {
    if (toast_.remainingSeconds <= 0.0 || toast_.message.empty()) return;
    const auto width =
        std::min(windowWidth_ - 40, textWidth(toast_.message, 1) + 40);
    SDL_Rect rect{windowWidth_ / 2 - width / 2,
                  screen_ == Screen::Editor ? toolbarRect_.y
                                                   + toolbarRect_.h + 12
                                             : 25,
                  width, 38};
    fillRect(renderer_, rect, {12, 22, 34, 240});
    strokeRect(renderer_, rect, toast_.color, 2);
    textCentered(renderer_, rect,
                 clippedText(toast_.message, rect.w - 20, 1),
                 toast_.color, 1);
}

} // namespace proteus::ui
