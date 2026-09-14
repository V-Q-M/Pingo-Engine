#include "Engine.h"

#include <algorithm>
#include <utility>

// Step by which - and + change a volume
constexpr int VOLUME_STEP = 10;

// Free camera in the Development mode, in pixels per second. Shift speeds it up.
constexpr float DEV_CAMERA_SPEED = 120.0f;
constexpr float DEV_CAMERA_FAST_FACTOR = 3.0f;

// Which pixel of a cursor icon sits on the mouse: the fingertip of the
// pointing hand, the center of the grabbing one
constexpr Vector2 CURSOR_POINTER_HOTSPOT{2.0f, 1.0f};
constexpr Vector2 CURSOR_GRAB_HOTSPOT{4.0f, 4.0f};

// Icons bring their own spacing to the text, see FontRenderer
static std::string IconLabel(char icon, const std::string &label) {
    return std::string(1, icon) + label;
}

static bool IsShiftDown() {
    return IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
}

static Menu::Entry ToggleEntry(const std::string &label, bool on) {
    return {label, on ? "ON" : "OFF", false};
}

static int StepVolume(int volume, MenuAction action) {
    if (action == MenuAction::Decrease) {
        volume -= VOLUME_STEP;
    } else if (action == MenuAction::Increase) {
        volume += VOLUME_STEP;
    }

    return std::clamp(volume, 0, 100);
}

Engine::Engine(Config config, EngineOptions options)
    : window(options.windowWidth, options.windowHeight, options.title),
      buildConfig(config),
      config(config),
      settings(Settings::Load(SETTINGS_FILE)),
      renderer(options.virtualWidth, options.virtualHeight),
      pauseMenu(font, 2, 12),
      settingsMenu(font, 1, 6) {
    settingsMenu.SetTitle("Settings", 2);

    settingsItems = {
        SettingsItem::MasterVolume,
        SettingsItem::MusicVolume,
        SettingsItem::Fullscreen
    };

    // FPS and hitboxes are debug tools and do not show up in Release
    if (config.HasDebugTools()) {
        settingsItems.push_back(SettingsItem::ShowFps);
        settingsItems.push_back(SettingsItem::Hitboxes);
    }

    settingsItems.push_back(SettingsItem::Back);

    RefreshSettingsMenu();
    ApplySettings();

    // Instead of the system cursor the engine draws the hand from the font
    HideCursor();
}

Engine::~Engine() {
    // The scene first: it may still hold references to textures
    if (scene) {
        scene->Exit();
        scene.reset();
    }
}

const Config &Engine::GetConfig() const {
    return config;
}

const Settings &Engine::GetSettings() const {
    return settings;
}

Assets &Engine::GetAssets() {
    return assets;
}

Renderer &Engine::GetRenderer() {
    return renderer;
}

InputMap &Engine::GetInput() {
    return input;
}

const FontRenderer &Engine::GetFont() const {
    return font;
}

const Theme &Engine::GetTheme() const {
    return theme;
}

void Engine::Run() {
    while (!WindowShouldClose() && !quitRequested) {
        RunFrame();
    }
}

void Engine::RunFrame() {
    SwitchScene();

    Update(GetFrameTime());

    Draw();
}

void Engine::Quit() {
    quitRequested = true;
}

bool Engine::ShowsEditorHelp() const {
    return editorHelpVisible;
}

void Engine::SetEditorHelpVisible(bool visible) {
    editorHelpVisible = visible;
}

SceneCatalog &Engine::GetScenes() {
    return scenes;
}

const SceneCatalog &Engine::GetScenes() const {
    return scenes;
}

bool Engine::EnterScene(const std::string &id) {
    int index = scenes.IndexOf(id);

    if (index < 0) {
        TraceLog(LOG_WARNING, "SCENE: Szene \"%s\" gibt es nicht", id.c_str());
        return false;
    }

    return scenes.Enter(*this, static_cast<std::size_t>(index));
}

bool Engine::EnterEntryPoint() {
    int index = scenes.EntryPoint();

    if (index < 0) {
        TraceLog(LOG_WARNING, "SCENE: Es gibt keinen Einstiegspunkt");
        return false;
    }

    return scenes.Enter(*this, static_cast<std::size_t>(index));
}

std::string Engine::CurrentSceneId() const {
    return scene ? scene->Options().id : "";
}

bool Engine::ShowsSceneTabs() const {
    return config.IsDevelopment() && scenes.Count() > 0;
}

DevCamera &Engine::GetDevCamera() {
    return devCamera;
}

SceneBar &Engine::GetSceneBar() {
    return sceneBar;
}

const SceneBar &Engine::GetSceneBar() const {
    return sceneBar;
}

float Engine::UiTop() const {
    return ShowsSceneTabs() ? sceneBar.Height(font) + 8.0f : 8.0f;
}

void Engine::RequestCursor(CursorState state) {
    // The states are sorted by strength
    if (static_cast<int>(state) > static_cast<int>(cursor)) {
        cursor = state;
    }
}

CursorState Engine::GetCursor() const {
    return cursor;
}

Texture2D *Engine::GetBackground() {
    const SceneEntry *entry = scene ? scenes.Find(scene->Options().id) : nullptr;

    if (entry == nullptr || entry->background.empty()) {
        return nullptr;
    }

    return &assets.Texture().Get(std::string(SceneCatalog::BACKGROUND_FOLDER) + "/" + entry->background);
}

void Engine::SetSimulating(bool simulate) {
    if (!buildConfig.IsDevelopment() || simulate == simulating || !sceneFactory) {
        return;
    }

    simulationRequested = simulate;
    simulationPending = true;

    // The open scene is rebuilt in the new mode
    pendingScene = sceneFactory;
}

bool Engine::IsSimulating() const {
    return simulating;
}

bool Engine::IsTyping() const {
    return (scene && scene->CapturesKeyboard()) || sceneBar.CapturesKeyboard();
}

void Engine::SetGlobalUpdate(std::function<void(Engine &)> update) {
    globalUpdate = std::move(update);
}

bool Engine::IsMenuOpen() const {
    return pauseMenu.IsOpen() || settingsMenu.IsOpen();
}

void Engine::OpenPauseMenu() {
    if (!scene || !scene->Options().pauseMenu) {
        return;
    }

    settingsMenu.Close();
    pauseMenu.Open();
}

void Engine::OpenSettings() {
    settingsReturnToPause = false;

    pauseMenu.Close();
    settingsMenu.Open();
}

void Engine::SwitchScene() {
    if (!pendingScene) {
        return;
    }

    std::function<std::unique_ptr<Scene>()> create = std::move(pendingScene);
    pendingScene = nullptr;

    // First tear down the old scene completely, then create the new one. That
    // way two scenes never exist at the same time, and nothing from the old
    // scene can reach into the new one.
    if (scene) {
        scene->Exit();
        scene.reset();
    }

    // A deleted scene only disappears once it has been torn down
    sceneBar.FinishPendingRemoval(scenes);

    // The new mode only applies from here on: the old scene was torn down in the old one
    if (simulationPending) {
        simulating = simulationRequested;
        simulationPending = false;

        config = simulating ? Config(BuildMode::Debugging) : buildConfig;

        TraceLog(LOG_INFO, simulating ? "ENGINE: Simulation gestartet" : "ENGINE: Zurueck im Development-Modus");
    }

    // Menus and camera belong to no scene and start fresh every time
    pauseMenu.Close();
    settingsMenu.Close();
    settingsReturnToPause = false;

    sceneBar.CloseWindows();

    renderer.ClearCamera();

    scene = create();
    sceneFactory = create;

    sceneBar.Refresh(scenes, CurrentSceneId(), renderer.GetWidth());

    // The free camera starts with the same view as the game: the top left
    // corner of the world
    if (config.IsDevelopment()) {
        devCamera.Reset({renderer.GetWidth() / 2.0f, renderer.GetHeight() / 2.0f});
        renderer.SetCameraFocus(devCamera.Focus());
    }

    input.SetMovementEnabled(scene->Options().movement);

    // The last item uses the small font
    pauseMenu.SetEntries({
        {"Continue", ""},
        {"Settings", ""},
        {scene->Options().pauseLeaveLabel, "", false, 1}
    });

    ApplyTheme(scene->Options().theme);

    ApplyMusic(scene->Options().music);

    scene->Enter();
}

void Engine::ApplyTheme(const Theme &next) {
    theme = next;

    font = theme.font.empty()
               ? FontRenderer()
               : FontRenderer(assets.Texture().Get(theme.font), theme.letterWidth, theme.letterHeight);

    pauseMenu.SetTheme(theme);
    settingsMenu.SetTheme(theme);
}

void Engine::ApplyMusic(const std::string &music) {
    // Without its own music the scene is silent. In the Development mode the
    // music always stays off, see Config.
    if (!config.MusicEnabled() || music.empty()) {
        assets.Music().Stop();
        return;
    }

    // If the same track is already playing, it continues seamlessly
    assets.Music().Play(music);
}

void Engine::Update(float dt) {
    // Remember whether a menu was already open before this frame. The menu that
    // ESC just opened should not trigger anything in this frame yet, and the
    // scene should already stand still in the same frame.
    bool menuWasOpen = IsMenuOpen();

    // Menus, scene and scene bar report the mouse cursor anew every frame
    cursor = CursorState::Idle;

    // Shift + Enter plays the open scene, as long as nothing is being typed
    bool enterPressed = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);

    if (buildConfig.IsDevelopment() && !simulating && enterPressed && IsShiftDown() && !IsTyping()) {
        SetSimulating(true);
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        HandleEscape();
    }

    if (menuWasOpen) {
        // Only handle one menu per frame. Otherwise the same click that opens
        // the settings would immediately trigger an item there.
        if (pauseMenu.IsOpen()) {
            UpdatePauseMenu();
        } else if (settingsMenu.IsOpen()) {
            UpdateSettingsMenu();
        }
    } else if (!IsMenuOpen() && scene) {
        // While typing in a form, H is a letter and toggles nothing
        if (config.IsDevelopment() && IsKeyPressed(KEY_H) && !scene->CapturesKeyboard() &&
            !sceneBar.CapturesKeyboard()) {
            editorHelpVisible = !editorHelpVisible;
        }

        if (globalUpdate) {
            globalUpdate(*this);
        }

        // In the Development mode the world stands still: instead of the scene
        // only the free camera and the scene's tools run
        if (config.IsDevelopment()) {
            UpdateDevelopmentCamera(dt);

            bool barBusy = ShowsSceneTabs() && sceneBar.Update(
                *this,
                renderer.MouseViewportPosition(),
                IsMouseButtonPressed(MOUSE_BUTTON_LEFT),
                IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)
            );

            if (!barBusy) {
                scene->UpdateDevelopment(dt);
            }

            if (ShowsSceneTabs() && sceneBar.IsHovering()) {
                RequestCursor(CursorState::Hover);
            }
        } else {
            scene->Update(dt);
        }
    }

    // The music keeps playing in the menu too, otherwise the stream breaks off
    assets.Music().Update();
}

// ESC goes back one level: from the settings to where they were opened,
// from the pause menu back to the scene
void Engine::HandleEscape() {
    // Shift + ESC ends the simulation, no matter what is open
    if (simulating && IsShiftDown()) {
        SetSimulating(false);
        return;
    }

    if (settingsMenu.IsOpen()) {
        CloseSettings();
        return;
    }

    // The windows of the scene bar lie above the scene and close first
    if (!pauseMenu.IsOpen() && sceneBar.CloseWindows()) {
        return;
    }

    // If the scene itself has something open, e.g. a context menu, ESC
    // closes that first
    if (!pauseMenu.IsOpen() && scene && scene->OnEscape()) {
        return;
    }

    if (pauseMenu.IsOpen()) {
        pauseMenu.Close();
    } else {
        OpenPauseMenu();
    }
}

void Engine::UpdatePauseMenu() {
    Menu::Event event = pauseMenu.Update(
        renderer.MouseViewportPosition(),
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT),
        renderer.GetWidth(),
        renderer.GetHeight()
    );

    if (pauseMenu.IsHovering()) {
        RequestCursor(CursorState::Hover);
    }

    if (event.item == Menu::NOTHING) {
        return;
    }

    switch (static_cast<PauseItem>(event.item)) {
        case PauseItem::Continue:
            pauseMenu.Close();
            break;

        case PauseItem::Settings:
            settingsReturnToPause = true;
            pauseMenu.Close();
            settingsMenu.Open();
            break;

        case PauseItem::Leave:
            pauseMenu.Close();
            scene->LeaveFromPauseMenu();
            break;
    }
}

void Engine::CloseSettings() {
    settingsMenu.Close();

    if (settingsReturnToPause) {
        pauseMenu.Open();
    }
}

void Engine::UpdateSettingsMenu() {
    Menu::Event event = settingsMenu.Update(
        renderer.MouseViewportPosition(),
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT),
        renderer.GetWidth(),
        renderer.GetHeight()
    );

    if (settingsMenu.IsHovering()) {
        RequestCursor(CursorState::Hover);
    }

    if (event.item == Menu::NOTHING) {
        return;
    }

    switch (settingsItems.at(static_cast<std::size_t>(event.item))) {
        case SettingsItem::MasterVolume:
            settings.masterVolume = StepVolume(settings.masterVolume, event.action);
            break;

        case SettingsItem::MusicVolume:
            settings.musicVolume = StepVolume(settings.musicVolume, event.action);
            break;

        case SettingsItem::Fullscreen:
            settings.fullscreen = !settings.fullscreen;
            break;

        case SettingsItem::ShowFps:
            settings.showFps = !settings.showFps;
            break;

        case SettingsItem::Hitboxes:
            settings.showHitboxes = !settings.showHitboxes;
            break;

        case SettingsItem::Back:
            CloseSettings();
            return;
    }

    ApplySettings();
    RefreshSettingsMenu();

    // Save right away, so nothing is lost on quit or crash
    settings.Save(SETTINGS_FILE);
}

void Engine::RefreshSettingsMenu() {
    std::vector<Menu::Entry> entries;

    for (SettingsItem item: settingsItems) {
        switch (item) {
            case SettingsItem::MasterVolume:
                entries.push_back({
                    IconLabel(settings.masterVolume > 0 ? FontRenderer::ICON_SOUND : FontRenderer::ICON_SOUND_OFF, "Volume"),
                    std::to_string(settings.masterVolume),
                    true
                });
                break;

            case SettingsItem::MusicVolume:
                entries.push_back({
                    IconLabel(settings.musicVolume > 0 ? FontRenderer::ICON_MUSIC : FontRenderer::ICON_MUSIC_OFF, "Music"),
                    std::to_string(settings.musicVolume),
                    true
                });
                break;

            case SettingsItem::Fullscreen:
                entries.push_back(ToggleEntry("Fullscreen", settings.fullscreen));
                break;

            case SettingsItem::ShowFps:
                entries.push_back(ToggleEntry("Show FPS", settings.showFps));
                break;

            case SettingsItem::Hitboxes:
                entries.push_back(ToggleEntry("Hitboxes", settings.showHitboxes));
                break;

            case SettingsItem::Back:
                entries.push_back({"Back", ""});
                break;
        }
    }

    settingsMenu.SetEntries(std::move(entries));
}

void Engine::ApplySettings() {
    SetMasterVolume(settings.masterVolume / 100.0f);

    assets.Music().SetVolume(settings.musicVolume / 100.0f);

    // In the Release build hitboxes always stay off
    renderer.SetShowHitboxes(config.HasDebugTools() && settings.showHitboxes);

    // Borderless fullscreen instead of real fullscreen: it does not change the
    // monitor's resolution and behaves cleanly on macOS
    if (IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE) != settings.fullscreen) {
        ToggleBorderlessWindowed();
    }
}

void Engine::Draw() {
    renderer.BeginDraw();

    // The background image lies below the whole world, without an image it stays black
    Texture2D *background = GetBackground();

    if (background != nullptr) {
        DrawTexture(*background, 0, 0, WHITE);
    }

    if (scene) {
        scene->Draw();
    }

    renderer.BeginUI();

    if (scene) {
        scene->DrawUI();
    }

    // Above the scene's interface, below the engine's menus
    if (ShowsSceneTabs()) {
        sceneBar.DrawTabs(renderer.GetWidth(), font);
    }

    if (config.HasDebugTools() && settings.showFps) {
        DrawFps();
    }

    // Also in the simulation, so it is not confused with the real game
    if (buildConfig.IsDevelopment()) {
        DrawModeLabel();
    }

    if (ShowsSceneTabs()) {
        sceneBar.DrawWindows(font);
    }

    // Last, so the overlay also covers the scene's interface
    pauseMenu.Draw(renderer.GetWidth(), renderer.GetHeight());
    settingsMenu.Draw(renderer.GetWidth(), renderer.GetHeight());

    // The mouse cursor lies above everything
    DrawCursor();

    renderer.EndDraw();
}

void Engine::DrawCursor() const {
    // If the mouse is outside the window, the hand lands outside the viewport
    // and cannot be seen. The system shows its own cursor there.
    bool grab = cursor == CursorState::Grab;

    char icon = grab ? FontRenderer::ICON_GRAB : FontRenderer::ICON_POINTER;
    Vector2 hotspot = grab ? CURSOR_GRAB_HOTSPOT : CURSOR_POINTER_HOTSPOT;
    int variant = cursor == CursorState::Idle ? FontVariant::White : FontVariant::Yellow;

    Vector2 mouse = renderer.MouseViewportPosition();

    font.Draw(std::string(1, icon), {mouse.x - hotspot.x, mouse.y - hotspot.y}, variant);
}

void Engine::DrawFps() const {
    std::string fps = std::to_string(GetFPS()) + " FPS";

    int width = font.Measure(fps, TextSpacing::Narrow);

    font.Draw(
        fps,
        {static_cast<float>(renderer.GetWidth() - width - 8), UiTop()},
        theme.textVariant,
        TextSpacing::Narrow
    );
}

void Engine::DrawModeLabel() const {
    std::string label = buildConfig.ModeName();

    // If the view is zoomed, the level is shown next to it, e.g. "Development x2"
    if (config.IsDevelopment() && renderer.CameraZoom() != 1.0f) {
        label += std::string(" ") + devCamera.ZoomLabel();
    }

    font.Draw(
        label,
        {8.0f, static_cast<float>(renderer.GetHeight() - font.LetterHeight() - 8)},
        theme.hoverVariant,
        TextSpacing::Narrow
    );
}

void Engine::UpdateDevelopmentCamera(float dt) {
    if ((scene && scene->CapturesKeyboard()) || sceneBar.CapturesKeyboard()) {
        return;
    }

    // Mouse wheel and space zoom, if the scene allows it. Over the windows of
    // the scene bar the zoom rests.
    bool zoom = scene && scene->Options().developmentZoom;

    if (zoom && !sceneBar.HasOpenWindow()) {
        devCamera.Scroll(GetMouseWheelMove(), renderer.MouseWorldPosition());

        if (IsKeyPressed(KEY_SPACE)) {
            devCamera.ResetZoom();
        }
    }

    // Arrow keys and WASD, no matter how the game mapped its input. If the scene
    // or the scene bar currently needs the arrow keys itself, only WASD remains.
    bool arrows = !(scene && scene->UsesArrowKeys()) && !sceneBar.HasOpenWindow();

    Vector2 direction{0.0f, 0.0f};

    if ((arrows && IsKeyDown(KEY_LEFT)) || IsKeyDown(KEY_A)) {
        direction.x -= 1.0f;
    }

    if ((arrows && IsKeyDown(KEY_RIGHT)) || IsKeyDown(KEY_D)) {
        direction.x += 1.0f;
    }

    if ((arrows && IsKeyDown(KEY_UP)) || IsKeyDown(KEY_W)) {
        direction.y -= 1.0f;
    }

    if ((arrows && IsKeyDown(KEY_DOWN)) || IsKeyDown(KEY_S)) {
        direction.y += 1.0f;
    }

    float speed = DEV_CAMERA_SPEED;

    if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
        speed *= DEV_CAMERA_FAST_FACTOR;
    }

    devCamera.Update(direction, speed, dt, CurrentWorldArea());

    renderer.SetCameraFocus(devCamera.Focus());
    renderer.SetCameraZoom(zoom ? devCamera.Zoom() : 1.0f);
}

Rectangle Engine::CurrentWorldArea() const {
    Rectangle area = scene ? scene->WorldArea() : Rectangle{0.0f, 0.0f, 0.0f, 0.0f};

    if (area.width <= 0.0f || area.height <= 0.0f) {
        area = {
            0.0f,
            0.0f,
            static_cast<float>(renderer.GetWidth()),
            static_cast<float>(renderer.GetHeight())
        };
    }

    return area;
}
