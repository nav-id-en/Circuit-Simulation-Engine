#include "proteus/ui/StudioApp.hpp"

#include "proteus/persistence/CircuitSerializer.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace proteus::ui {

StudioApp::StudioApp() = default;

StudioApp::~StudioApp() {
    shutdown();
}

int StudioApp::run(int argc, char** argv) {
    bool headless = false;
    std::string smokeOutput = "artifacts/sdl_smoke.png";
    std::string requestedProject;
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--headless-smoke") {
            headless = true;
            if (index + 1 < argc && argv[index + 1][0] != '-') {
                smokeOutput = argv[++index];
            }
        } else if (argument == "--project" && index + 1 < argc) {
            requestedProject = argv[++index];
        }
    }

    if (!initialize(headless)) {
        return 1;
    }

    if (!requestedProject.empty()) {
        openProject(requestedProject);
    }

    if (headless) {
        return runHeadlessSmoke(smokeOutput);
    }
    return runInteractive();
}

bool StudioApp::initialize(bool hidden) {
    if (initialized_) return true;
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';
        return false;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    const Uint32 flags =
        (hidden ? SDL_WINDOW_HIDDEN : SDL_WINDOW_SHOWN)
        | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
    window_ = SDL_CreateWindow(
        "ProteusLab SDL - Untitled",
        static_cast<int>(SDL_WINDOWPOS_CENTERED),
        static_cast<int>(SDL_WINDOWPOS_CENTERED), windowWidth_, windowHeight_,
        flags);
    if (!window_) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << '\n';
        SDL_Quit();
        return false;
    }
    SDL_SetWindowMinimumSize(window_, 1050, 680);
    renderer_ = SDL_CreateRenderer(
        window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) {
        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!renderer_) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << '\n';
        SDL_DestroyWindow(window_);
        window_ = nullptr;
        SDL_Quit();
        return false;
    }
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    loadCatalog();
    loadRecentProjects();
    rebuildEngine();
    updateLayout();
    resetHistory();
    initialized_ = true;
    lastFrameSeconds_ = static_cast<double>(SDL_GetTicks64()) / 1000.0;
    return true;
}

void StudioApp::shutdown() {
    if (!initialized_ && !window_ && !renderer_) return;
    saveRecentProjects();
    engine_.reset();
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    SDL_Quit();
    initialized_ = false;
}

int StudioApp::runInteractive() {
    running_ = true;
    while (running_) {
        SDL_Event event{};
        while (SDL_PollEvent(&event)) {
            handleEvent(event);
        }
        const auto now = static_cast<double>(SDL_GetTicks64()) / 1000.0;
        const auto delta = std::clamp(now - lastFrameSeconds_, 0.0, 0.1);
        lastFrameSeconds_ = now;
        update(delta);
        render();
    }
    return 0;
}

int StudioApp::runHeadlessSmoke(const std::string& outputPath) {
    if (!openProject("examples/mixed_signal_demo.oopproteus.json")) {
        newProject("SDL Smoke Test");
        addComponent("dc_source", {220.0, 260.0});
        addComponent("resistor", {500.0, 260.0});
        addComponent("led", {780.0, 260.0});
        addComponent("ground", {500.0, 520.0});
    }
    screen_ = Screen::Editor;
    showLog_ = true;
    runDrc();
    updateLayout();
    render();
    if (!saveRendererPng(renderer_, outputPath, windowWidth_, windowHeight_)) {
        std::cerr << "Could not write SDL smoke image: " << SDL_GetError()
                  << '\n';
        return 2;
    }
    std::cout << "SDL smoke image written to " << outputPath << '\n';
    return 0;
}

void StudioApp::rebuildEngine() {
    engine_ = std::make_unique<SimulationEngine>(circuit_);
    engine_->setScopeChannels(probeChannels_);
}

void StudioApp::loadCatalog() {
    catalog_.clear();
    for (const auto& type : ComponentFactory::supportedTypes()) {
        auto component = ComponentFactory::create(type, "preview", "PREVIEW");
        catalog_.push_back(
            {type, component->displayName(), component->category()});
    }
    activeTypes_ = {"resistor", "dc_source", "ground", "led",
                    "and_gate", "microcontroller"};
}

void StudioApp::updateLayout() {
    if (window_) SDL_GetWindowSize(window_, &windowWidth_, &windowHeight_);
    const auto leftWidth = windowWidth_ < 1200 ? 215 : 250;
    const auto rightWidth = windowWidth_ < 1200 ? 250 : 300;
    const auto menuHeight = 30;
    const auto toolbarHeight = 48;
    const auto statusHeight = 24;
    const auto logHeight = showLog_ ? std::min(176, windowHeight_ / 4) : 0;

    menuBarRect_ = {0, 0, windowWidth_, menuHeight};
    toolbarRect_ = {0, menuHeight, windowWidth_, toolbarHeight};
    statusRect_ = {0, windowHeight_ - statusHeight, windowWidth_,
                   statusHeight};
    libraryRect_ = {0, menuHeight + toolbarHeight, leftWidth,
                    windowHeight_ - menuHeight - toolbarHeight - statusHeight};
    propertiesRect_ = {windowWidth_ - rightWidth,
                       menuHeight + toolbarHeight, rightWidth,
                       windowHeight_ - menuHeight - toolbarHeight
                           - statusHeight};
    logRect_ = {leftWidth,
                windowHeight_ - statusHeight - logHeight,
                windowWidth_ - leftWidth - rightWidth, logHeight};
    canvasRect_ = {leftWidth, menuHeight + toolbarHeight,
                   windowWidth_ - leftWidth - rightWidth,
                   windowHeight_ - menuHeight - toolbarHeight - statusHeight
                       - logHeight};
    searchRect_ = {libraryRect_.x + 12, libraryRect_.y + 44,
                   libraryRect_.w - 24, 32};
    categoryRect_ = {libraryRect_.x + 12, libraryRect_.y + 84,
                     libraryRect_.w - 24, 30};
    propertyRowsRect_ = {propertiesRect_.x + 10,
                         propertiesRect_.y + 78,
                         propertiesRect_.w - 20,
                         propertiesRect_.h - 88};
}

void StudioApp::update(double deltaSeconds) {
    if (screen_ == Screen::Editor && engine_
        && engine_->state() == SimulationState::Running) {
        engine_->advance(deltaSeconds);
    }
    if (toast_.remainingSeconds > 0.0) {
        toast_.remainingSeconds =
            std::max(0.0, toast_.remainingSeconds - deltaSeconds);
    }
}

void StudioApp::render() {
    setColor(renderer_, colors::background);
    SDL_RenderClear(renderer_);
    if (screen_ == Screen::Welcome) {
        renderWelcome();
    } else {
        renderEditor();
    }
    renderToast();
    if (modal_) renderModal();
    SDL_RenderPresent(renderer_);
}

std::vector<StudioApp::Button> StudioApp::toolbarButtons() const {
    const auto y = toolbarRect_.y + 7;
    auto x = 10;
    const auto add =
        [&x, y, this](std::vector<Button>& result, std::string id,
                      std::string label, bool active = false,
                      bool enabled = true, int width = 54) {
            result.push_back(
                {std::move(id), std::move(label),
                 {x, y, width, toolbarRect_.h - 14}, active, enabled});
            x += width + 5;
        };
    std::vector<Button> result;
    add(result, "new", "NEW");
    add(result, "open", "OPEN", false, true, 60);
    add(result, "save", "SAVE");
    x += 8;
    add(result, "undo", "UNDO", false, history_.index > 0, 58);
    add(result, "redo", "REDO", false,
        history_.index + 1 < history_.snapshots.size(), 58);
    x += 8;
    add(result, "select", "SEL", tool_ == Tool::Select);
    add(result, "wire", "WIRE", tool_ == Tool::Wire, true, 60);
    add(result, "junction", "DOT", tool_ == Tool::Junction);
    add(result, "probe", "PROBE", tool_ == Tool::Probe, true, 64);
    add(result, "pan", "PAN", tool_ == Tool::Pan);
    x += 8;
    const auto stopped =
        !engine_ || engine_->state() == SimulationState::Stopped;
    const auto running =
        engine_ && engine_->state() == SimulationState::Running;
    add(result, "run", "RUN", running, !running);
    add(result, "pause", "PAUSE", false, running, 62);
    add(result, "stop", "STOP", false, !stopped);
    add(result, "step", "STEP", false, !running);
    add(result, "drc", "DRC");
    add(result, "scope", "SCOPE", showScope_, true, 64);
    add(result, "export", "PNG", false, true, 56);
    return result;
}

std::vector<StudioApp::MenuItem> StudioApp::menuItems(
    const std::string& menu) const {
    if (menu == "FILE") {
        return {{"New project     Ctrl+N", "new", true},
                {"Open project    Ctrl+O", "open", true},
                {"Save project    Ctrl+S", "save", true},
                {"Save project as", "save_as", true},
                {"Export PNG      Ctrl+E", "export", true},
                {"Welcome screen", "welcome", true},
                {"Exit", "exit", true}};
    }
    if (menu == "EDIT") {
        return {{"Undo            Ctrl+Z", "undo", history_.index > 0},
                {"Redo            Ctrl+Y", "redo",
                 history_.index + 1 < history_.snapshots.size()},
                {"Delete selection", "delete",
                 !selectedComponents_.empty() || !selectedWires_.empty()},
                {"Rotate          R", "rotate",
                 !selectedComponents_.empty()},
                {"Mirror H        H", "mirror_h",
                 !selectedComponents_.empty()},
                {"Mirror V        V", "mirror_v",
                 !selectedComponents_.empty()}};
    }
    if (menu == "SIM") {
        return {{"Run", "run", true},
                {"Pause", "pause", true},
                {"Stop", "stop", true},
                {"Single step", "step", true},
                {"Design rule check", "drc", true},
                {"Oscilloscope", "scope", true}};
    }
    if (menu == "VIEW") {
        return {{"Reset camera", "reset_camera", true},
                {"Toggle grid", "toggle_grid", true},
                {"Toggle log", "toggle_log", true},
                {"Fit canvas", "fit_canvas", true}};
    }
    return {};
}

void StudioApp::setToast(std::string message, Color color, double seconds) {
    toast_ = {std::move(message), color, seconds};
}

void StudioApp::setTool(Tool tool, std::string placeType) {
    tool_ = tool;
    placeType_ = std::move(placeType);
    wireStart_.reset();
    selectingBox_ = false;
    if (tool_ != Tool::Place) draggedLibraryType_.clear();
}

void StudioApp::resetCamera() {
    camera_ = {};
}

std::string StudioApp::simulationStateText() const {
    if (!engine_) return "NO ENGINE";
    switch (engine_->state()) {
    case SimulationState::Stopped: return "STOPPED";
    case SimulationState::Running: return "RUNNING";
    case SimulationState::Paused: return "PAUSED";
    }
    return "STOPPED";
}

std::string StudioApp::timeStampForFile() const {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &time);
#else
    localtime_r(&time, &local);
#endif
    std::ostringstream output;
    output << std::put_time(&local, "%Y%m%d_%H%M%S");
    return output.str();
}

} // namespace proteus::ui
