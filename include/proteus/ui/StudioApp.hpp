#pragma once

#include "proteus/components/Components.hpp"
#include "proteus/core/Circuit.hpp"
#include "proteus/simulation/SimulationEngine.hpp"
#include "proteus/ui/Draw.hpp"

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace proteus::ui {

class StudioApp {
public:
    StudioApp();
    ~StudioApp();

    StudioApp(const StudioApp&) = delete;
    StudioApp& operator=(const StudioApp&) = delete;

    [[nodiscard]] int run(int argc, char** argv);

private:
    enum class Screen {
        Welcome,
        Editor
    };

    enum class Tool {
        Select,
        Wire,
        Junction,
        Probe,
        Pan,
        Place
    };

    enum class ModalAction {
        None,
        NewProject,
        OpenProject,
        SaveProjectAs,
        ConfirmOverwrite,
        ExportImage,
        EditLabel,
        EditProperty
    };

    struct CatalogEntry {
        std::string type;
        std::string name;
        std::string category;
    };

    struct Button {
        std::string id;
        std::string label;
        SDL_Rect rect{};
        bool active = false;
        bool enabled = true;
    };

    struct MenuItem {
        std::string label;
        std::string action;
        bool enabled = true;
    };

    struct Modal {
        ModalAction action = ModalAction::None;
        std::string title;
        std::string value;
        std::string propertyKey;
        PropertyKind propertyKind = PropertyKind::Text;
        SDL_Rect box{};
    };

    struct Camera {
        double x = 0.0;
        double y = 0.0;
        double zoom = 1.0;
    };

    struct History {
        std::vector<std::string> snapshots;
        std::vector<std::string> descriptions;
        std::size_t index = 0;
    };

    struct Toast {
        std::string message;
        Color color = colors::accent;
        double remainingSeconds = 0.0;
    };

    bool initialize(bool hidden);
    void shutdown();
    int runInteractive();
    int runHeadlessSmoke(const std::string& outputPath);

    void rebuildEngine();
    void loadCatalog();
    void updateLayout();
    void update(double deltaSeconds);
    void render();
    void renderWelcome();
    void renderEditor();
    void renderTopMenu();
    void renderToolbar();
    void renderLibrary();
    void renderCanvas();
    void renderGrid();
    void renderWires();
    void renderComponents();
    void renderComponent(Component& component);
    void renderProperties();
    void renderLog();
    void renderStatusBar();
    void renderDropdownMenu();
    void renderModal();
    void renderScope();
    void renderToast();

    void handleEvent(const SDL_Event& event);
    void handleKeyDown(const SDL_KeyboardEvent& event);
    void handleTextInput(const SDL_TextInputEvent& event);
    void handleMouseDown(const SDL_MouseButtonEvent& event);
    void handleMouseUp(const SDL_MouseButtonEvent& event);
    void handleMouseMotion(const SDL_MouseMotionEvent& event);
    void handleMouseWheel(const SDL_MouseWheelEvent& event);
    void handleWelcomeClick(int x, int y);
    void handleEditorClick(int x, int y, Uint8 button, Uint8 clicks,
                           Uint16 modifiers);
    void handleAction(const std::string& action);
    void acceptModal();
    void cancelModal();

    [[nodiscard]] std::vector<Button> toolbarButtons() const;
    [[nodiscard]] std::vector<MenuItem> menuItems(
        const std::string& menu) const;
    [[nodiscard]] std::optional<std::string> toolbarAt(int x, int y) const;
    [[nodiscard]] std::optional<std::string> menuAt(int x, int y) const;
    [[nodiscard]] std::optional<std::string> dropdownActionAt(int x,
                                                              int y) const;
    [[nodiscard]] std::optional<std::size_t> libraryRowAt(int x,
                                                          int y) const;
    [[nodiscard]] std::vector<CatalogEntry> filteredCatalog() const;
    [[nodiscard]] std::optional<std::pair<std::string, std::string>>
    propertyAt(int x, int y) const;

    [[nodiscard]] Point screenToWorld(int x, int y) const;
    [[nodiscard]] SDL_Point worldToScreen(Point point) const;
    [[nodiscard]] Point snap(Point point) const;
    [[nodiscard]] SDL_Rect componentScreenRect(
        const Component& component) const;
    [[nodiscard]] Point componentSize(const Component& component) const;
    [[nodiscard]] Point pinWorldPosition(const Component& component,
                                         const PinDefinition& pin) const;
    [[nodiscard]] std::optional<PinRef> pinAt(int x, int y,
                                             int radius = 10) const;
    [[nodiscard]] std::optional<std::string> componentAt(int x, int y) const;
    [[nodiscard]] std::optional<std::string> wireAt(int x, int y) const;
    [[nodiscard]] std::vector<Point> wirePath(const Wire& wire) const;
    [[nodiscard]] std::vector<std::string> wiresAtWorldPoint(
        Point point, double tolerance) const;

    void newProject(const std::string& name);
    bool openProject(const std::string& path);
    bool saveProject(const std::string& path);
    void addRecentProject(const std::string& path);
    void loadRecentProjects();
    void saveRecentProjects() const;
    bool exportImage(const std::string& path);
    void runDrc();
    void runSimulation();
    void pauseSimulation();
    void stopSimulation();
    void stepSimulation();
    void toggleScope();

    void addComponent(const std::string& type, Point position);
    void beginWire(const PinRef& pin);
    void finishWire(const PinRef& pin);
    void createJunction(Point point);
    void addProbe(const PinRef& pin);
    void deleteSelection();
    void rotateSelection();
    void mirrorSelection(bool horizontal);
    void selectOnly(const std::string& componentId);
    void clearSelection();
    void commitHistory(const std::string& description);
    void resetHistory();
    void undo();
    void redo();
    void restoreHistorySnapshot(std::size_t index);
    void markModified(const std::string& description);
    void setToast(std::string message, Color color = colors::accent,
                  double seconds = 2.5);
    void setTool(Tool tool, std::string placeType = {});
    void resetCamera();
    void applyProperty(const std::string& key, PropertyKind kind,
                       const std::string& textValue);
    void loadSelectedFirmwareIfNeeded(const std::string& key);
    void handleRuntimePress(const std::string& componentId, bool pressed,
                            int mouseX, int mouseY);

    [[nodiscard]] std::string selectedComponentId() const;
    [[nodiscard]] Component* selectedComponent();
    [[nodiscard]] const Component* selectedComponent() const;
    [[nodiscard]] std::string propertyValueText(
        const PropertyValue& value) const;
    [[nodiscard]] Color wireColor(const Wire& wire) const;
    [[nodiscard]] std::string simulationStateText() const;
    [[nodiscard]] std::string timeStampForFile() const;

    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    int windowWidth_ = 1440;
    int windowHeight_ = 900;
    bool running_ = true;
    bool initialized_ = false;

    Screen screen_ = Screen::Welcome;
    Tool tool_ = Tool::Select;
    Circuit circuit_;
    std::unique_ptr<SimulationEngine> engine_;
    Camera camera_;
    History history_;
    Toast toast_;

    std::vector<CatalogEntry> catalog_;
    std::vector<std::string> recentProjects_;
    std::vector<std::string> activeTypes_;
    std::set<std::string> selectedComponents_;
    std::set<std::string> selectedWires_;
    std::map<std::string, PinRef> probeChannels_;

    std::string currentPath_;
    std::string searchText_;
    std::string categoryFilter_ = "All";
    std::string placeType_;
    std::string draggedLibraryType_;
    std::string openMenu_;
    std::string runtimePressedComponent_;
    std::optional<PinRef> wireStart_;
    std::optional<PinRef> hoveredPin_;
    std::optional<Modal> modal_;

    SDL_Rect menuBarRect_{};
    SDL_Rect toolbarRect_{};
    SDL_Rect libraryRect_{};
    SDL_Rect propertiesRect_{};
    SDL_Rect canvasRect_{};
    SDL_Rect logRect_{};
    SDL_Rect statusRect_{};
    SDL_Rect searchRect_{};
    SDL_Rect categoryRect_{};
    SDL_Rect propertyRowsRect_{};

    bool searchFocused_ = false;
    bool showGrid_ = true;
    bool showLog_ = true;
    bool showScope_ = false;
    bool saveRuntimeState_ = true;
    bool documentModified_ = false;
    bool panning_ = false;
    bool selectingBox_ = false;
    bool movingComponents_ = false;
    bool componentMoveChanged_ = false;
    bool libraryDragging_ = false;
    int mouseX_ = 0;
    int mouseY_ = 0;
    int previousMouseX_ = 0;
    int previousMouseY_ = 0;
    int libraryScroll_ = 0;
    int propertyScroll_ = 0;
    int logScroll_ = 0;
    double scopeTimePerDivision_ = 0.1;
    double scopeVoltsPerDivision_ = 1.0;
    SDL_Rect selectionBox_{};
    Point dragStartWorld_{};
    std::map<std::string, Point> dragOriginalPositions_;
    double lastFrameSeconds_ = 0.0;
};

} // namespace proteus::ui
