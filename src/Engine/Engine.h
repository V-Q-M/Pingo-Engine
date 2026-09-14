#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "Assets.h"
#include "Config.h"
#include "CursorState.h"
#include "DevCamera.h"
#include "FontRenderer.h"
#include "InputMap.h"
#include "Menu.h"
#include "Renderer.h"
#include "Scene.h"
#include "SceneBar.h"
#include "SceneCatalog.h"
#include "Settings.h"
#include "Theme.h"
#include "Window.h"

struct EngineOptions {
    std::string title = "PingoLegends";

    int windowWidth = 1280;
    int windowHeight = 720;

    // Resolution the game is drawn in. The window shows it scaled up by a
    // whole number.
    int virtualWidth = 320;
    int virtualHeight = 180;
};

// Holds everything every scene needs and runs the main loop.
//
// Scenes get the engine in their constructor and access assets, renderer,
// input, font and settings via the Get methods. The pause and settings
// menus belong to the engine, not the scene: a scene only turns them on or
// off via its SceneOptions. That way the settings exist exactly once and
// all scenes share them.
class Engine {
public:
    Engine(Config config, EngineOptions options);

    ~Engine();

    Engine(const Engine &) = delete;

    Engine &operator=(const Engine &) = delete;

    // Schedules a scene change, e.g. ChangeScene<MenuScene>(). The change
    // happens at the start of the next frame: first the old scene is
    // completely torn down, then the new one is created. That way a scene
    // may even replace itself in the middle of its own Update.
    template <typename T, typename... Args>
    void ChangeScene(Args... args) {
        pendingScene = [this, args...]() {
            return std::make_unique<T>(*this, args...);
        };
    }

    // Runs until the window is closed or Quit is called
    void Run();

    // A single frame: scene change, update, drawing. Run calls this in
    // a loop.
    void RunFrame();

    void Quit();

    // Runs every frame before the scene as long as no menu is open, also in the
    // Development mode. Meant for game-wide keys like scene changes.
    void SetGlobalUpdate(std::function<void(Engine &)> update);

    // The game's scenes. The game defines their types and loads the list.
    // In the Development mode they are shown as tabs at the top, see SceneBar.
    SceneCatalog &GetScenes();

    const SceneCatalog &GetScenes() const;

    // Schedules the change to the scene with this id, e.g. "hub". false
    // if it does not exist.
    bool EnterScene(const std::string &id);

    // Schedules the change to the entry point, e.g. the main menu
    bool EnterEntryPoint();

    // Id of the open scene, empty without a scene
    std::string CurrentSceneId() const;

    bool ShowsSceneTabs() const;

    // Tabs and windows of the scene bar. In the Development mode the engine
    // updates them every frame before the scene.
    SceneBar &GetSceneBar();

    const SceneBar &GetSceneBar() const;

    // The free camera of the Development mode. Scenes that turn on
    // SceneOptions::developmentZoom can zoom: mouse wheel in and out,
    // space to reset.
    DevCamera &GetDevCamera();

    // The background image of the open scene, see SceneEntry::background.
    // nullptr without an image: the background then stays black.
    Texture2D *GetBackground();

    // Development build only: plays the open scene like in the Debugging build,
    // so the world runs. Shift + Enter starts the simulation, Shift + ESC ends it.
    // Scene changes stay in the simulation. When switching, the open scene is
    // rebuilt in the new mode, at the next frame.
    void SetSimulating(bool simulate);

    bool IsSimulating() const;

    // The mouse cursor is always the hand from the font. Scenes report in Update
    // or UpdateDevelopment whether it is over something clickable or dragging.
    // Only applies to the current frame, the strongest report wins.
    void RequestCursor(CursorState state);

    CursorState GetCursor() const;

    // Topmost free height of the screen layer: below the scene bar, if it is
    // shown. E.g. for displays at the top right.
    float UiTop() const;

    // Opens the pause menu, if the scene has one. ESC calls this too.
    void OpenPauseMenu();

    // Opens the settings menu above the current scene, e.g. from a main menu.
    // Back then leads back into the scene.
    void OpenSettings();

    // Is a menu of the engine open right now? The scene pauses then.
    bool IsMenuOpen() const;

    // Key help of the developer tools. In the Development mode H toggles it,
    // it starts hidden.
    bool ShowsEditorHelp() const;

    void SetEditorHelpVisible(bool visible);

    // The mode the scenes are currently running in. In the simulation this is
    // Debugging, even if the build is Development.
    const Config &GetConfig() const;

    const Settings &GetSettings() const;

    Assets &GetAssets();

    Renderer &GetRenderer();

    InputMap &GetInput();

    const FontRenderer &GetFont() const;

    const Theme &GetTheme() const;

private:
    // Same order as in the pause menu
    enum class PauseItem {
        Continue,
        Settings,

        // The scene defines label and target, e.g. Quit
        Leave
    };

    // The items in the settings menu. Which of them are shown depends on the
    // build, see settingsItems.
    enum class SettingsItem {
        MasterVolume,
        MusicVolume,
        Fullscreen,
        ShowFps,
        Hitboxes,
        Back
    };

    // Lives in the working directory, not with the assets: the build copies the
    // assets again on every run and would overwrite the file
    static constexpr const char *SETTINGS_FILE = "settings.json";

    void SwitchScene();

    void Update(float dt);

    void Draw();

    void HandleEscape();

    void UpdatePauseMenu();

    void UpdateSettingsMenu();

    void CloseSettings();

    void ApplyTheme(const Theme &next);

    // Stops the music of the previous scene and starts the new one's
    void ApplyMusic(const std::string &music);

    // Applies the settings to audio, window and renderer
    void ApplySettings();

    // Rebuilds the items with the current values
    void RefreshSettingsMenu();

    void DrawFps() const;

    // Bottom left, so the Development mode is not mistaken for a freeze
    void DrawModeLabel() const;

    // Arrow keys and WASD move the free camera
    void UpdateDevelopmentCamera(float dt);

    Rectangle CurrentWorldArea() const;

    // The hand at the mouse, yellow over something clickable
    void DrawCursor() const;

    // Is the scene or the scene bar currently typing into a form?
    bool IsTyping() const;

    // The order of the members is intentional: they are constructed in this
    // order and destroyed in reverse. The window comes first and goes last,
    // the scene comes last and goes first.
    Window window;

    // The mode of the build and the one the scenes are currently running in
    Config buildConfig;
    Config config;

    Settings settings;

    Assets assets;
    Renderer renderer;
    InputMap input;

    // Only used in the Development mode
    DevCamera devCamera;

    FontRenderer font;
    Theme theme;

    Menu pauseMenu;
    Menu settingsMenu;

    SceneCatalog scenes;
    SceneBar sceneBar;

    CursorState cursor = CursorState::Idle;

    // For every item in the settings menu the setting behind it
    std::vector<SettingsItem> settingsItems;

    // Were the settings opened from the pause menu?
    bool settingsReturnToPause = false;

    bool quitRequested = false;

    bool editorHelpVisible = false;

    std::function<void(Engine &)> globalUpdate;

    // A change into the simulation or back applies from the next scene change on
    bool simulating = false;
    bool simulationPending = false;
    bool simulationRequested = false;

    std::function<std::unique_ptr<Scene>()> pendingScene;

    // How the open scene was created, so it can be rebuilt
    std::function<std::unique_ptr<Scene>()> sceneFactory;

    std::unique_ptr<Scene> scene;
};
