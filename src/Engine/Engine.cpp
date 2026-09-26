#include "Engine.h"

#include <algorithm>
#include <utility>

#include "Soundcard.h"

// Step by which - and + change a volume
constexpr int VOLUME_STEP = 10;

// Distance of the mode label from the bottom left corner, see DrawModeLabel
constexpr float MODE_LABEL_MARGIN = 8.0f;

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
      fontSpacing(FontSpacing::Load()),
      renderer(options.virtualWidth, options.virtualHeight),
      pauseMenu(font, 2, 12),
      settingsMenu(font, 1, 6) {
    settingsMenu.SetTitle("Settings", 2);

    settingsItems = {
        SettingsItem::MasterVolume,
        SettingsItem::MusicVolume,
        SettingsItem::Soundcard,
        SettingsItem::Fullscreen
    };

    // FPS and hitboxes are debug tools and do not show up in Release
    if (config.HasDebugTools()) {
        settingsItems.push_back(SettingsItem::ShowFps);
        settingsItems.push_back(SettingsItem::Hitboxes);
        settingsItems.push_back(SettingsItem::ObjectIds);
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

const MouseButtons &Engine::GetMouse() const {
    return mouse;
}

bool Engine::WasKeyPressed(int key) const {
    return std::find(pressedKeys.begin(), pressedKeys.end(), key) != pressedKeys.end();
}

bool Engine::WasAnyKeyPressed() const {
    return !pressedKeys.empty();
}

// Empties raylib's key queue into this frame. It has to be emptied, otherwise
// the presses of the last frames would pile up in it.
void Engine::TakePressedKeys() {
    pressedKeys.clear();

    for (int key = GetKeyPressed(); key != 0; key = GetKeyPressed()) {
        pressedKeys.push_back(key);
    }
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

Console &Engine::GetConsole() {
    return console;
}

const ConsoleCommands &Engine::GetCommands() const {
    return commands;
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

// Where the free screen ends at the bottom: over the mode label of the
// Development build, so a window of a scene does not lie on top of it
float Engine::UiBottom() const {
    float height = static_cast<float>(renderer.GetHeight());

    if (!buildConfig.IsDevelopment()) {
        return height;
    }

    return height - static_cast<float>(font.LetterHeight()) - MODE_LABEL_MARGIN;
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

    // The windows of the scene bar belong to the editor, not to the running
    // game: they would stay open invisibly behind the simulation
    sceneBar.CloseWindows();

    // The open scene is rebuilt in the new mode
    pendingScene = sceneFactory;
}

bool Engine::IsSimulating() const {
    return simulating;
}

bool Engine::IsTyping() const {
    return console.IsOpen() || (scene && scene->CapturesKeyboard()) || sceneBar.CapturesKeyboard();
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
    input.SetBlocked(console.IsOpen());

    // The last item uses the small font
    pauseMenu.SetEntries({
        {"Continue", ""},
        {"Settings", ""},
        {scene->Options().pauseLeaveLabel, "", false, 1}
    });

    ApplyTheme(scene->Options().theme);

    ApplyMusic(scene->Options().music);

    scene->Enter();

    // The commands of the old scene went with it
    commands.Clear();
    AddEngineCommands();
    scene->AddCommands(commands);
}

void Engine::ApplyTheme(const Theme &next) {
    theme = next;

    font = theme.font.empty()
               ? FontRenderer()
               : FontRenderer(assets.Texture().Get(theme.font), theme.letterWidth, theme.letterHeight);

    // How wide every character is stands outside the game, in
    // assets/fontSpacing.json, and is read once with the engine
    font.SetSpacing(fontSpacing);

    pauseMenu.SetTheme(theme);
    settingsMenu.SetTheme(theme);
}

void Engine::ApplyMusic(const std::string &music, bool keepPosition) {
    sceneMusic = music;

    // Without its own music the scene is silent. In the Development mode the
    // music always stays off, see Config.
    if (!config.MusicEnabled() || music.empty()) {
        assets.Music().Stop();
        return;
    }

    // If the same track is already playing, it continues seamlessly
    assets.Music().Play(Soundcard::Find(settings.soundcard).Track(music), keepPosition);
}

void Engine::Update(float dt) {
    // Remember whether a menu was already open before this frame. The menu that
    // ESC just opened should not trigger anything in this frame yet, and the
    // scene should already stand still in the same frame.
    bool menuWasOpen = IsMenuOpen();

    // Menus, scene and scene bar report the mouse cursor anew every frame
    cursor = CursorState::Idle;

    // The clicks and the keys of this frame, before anything asks for them
    mouse.Update();
    TakePressedKeys();

    // Shift + Enter plays the open scene, as long as nothing is being typed
    bool enterPressed = WasKeyPressed(KEY_ENTER) || WasKeyPressed(KEY_KP_ENTER);

    if (buildConfig.IsDevelopment() && !simulating && enterPressed && IsShiftDown() && !IsTyping()) {
        SetSimulating(true);
    }

    // ESC belongs to the engine alone and is never blocked: whatever is open
    // gets it first, see HandleEscape
    if (WasKeyPressed(KEY_ESCAPE)) {
        HandleEscape();
    }

    // The console lies above the scene: while it is open it gets the keyboard
    if (config.HasDebugTools() && !IsMenuOpen()) {
        UpdateConsole();
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
        // The console takes the keyboard, the game keeps running: the hotkeys
        // of the engine and of the game rest until it is closed. What the
        // scenes and their tools do about it is in their own Update, see
        // ObjectEditor::Input::keyboardBusy.
        const bool consoleTypes = console.IsOpen();

        // While typing, H is a letter and toggles nothing
        if (config.IsDevelopment() && !consoleTypes && WasKeyPressed(KEY_H) && !scene->CapturesKeyboard() &&
            !sceneBar.CapturesKeyboard()) {
            editorHelpVisible = !editorHelpVisible;
        }

        // The scene keys belong to the editor, not to a window that is open
        // on top of it: while something takes the keyboard, F1 and friends
        // rest instead of throwing the open window away
        if (globalUpdate && !IsTyping()) {
            globalUpdate(*this);
        }

        // In the Development mode the world stands still: instead of the scene
        // only the free camera and the scene's tools run
        if (config.IsDevelopment()) {
            UpdateDevelopmentCamera(dt);

            bool barBusy = ShowsSceneTabs() && sceneBar.Update(
                *this,
                renderer.MouseViewportPosition(),
                mouse.IsLeftClicked(),
                mouse.IsRightClicked()
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

// ESC goes back one level. Nothing ever blocks the key itself: whatever lies
// on top answers it, from the top down, and only that one thing happens.
//
//   1. Shift and ESC end the simulation from anywhere
//   2. the console
//   3. the settings
//   4. a window of the scene bar
//   5. the scene, e.g. its code editor or a context menu
//   6. the pause menu: it opens, and a second ESC closes it again
//
// That way it always finds a way out and never runs into nothing.
void Engine::HandleEscape() {
    // Shift + ESC ends the simulation, no matter what is open
    if (simulating && IsShiftDown()) {
        SetSimulating(false);
        return;
    }

    // The console lies above everything of the scene and closes first
    if (console.IsOpen()) {
        CloseConsole();
        return;
    }

    if (settingsMenu.IsOpen()) {
        CloseSettings();
        return;
    }

    // The windows of the scene bar lie above the scene and close first. Only
    // while the bar is really there: in the simulation it is hidden, and a
    // window that stayed open behind it would eat every ESC.
    if (!pauseMenu.IsOpen() && ShowsSceneTabs() && sceneBar.CloseWindows()) {
        return;
    }

    // If the scene itself has something open, e.g. a context menu, ESC
    // closes that first
    if (!pauseMenu.IsOpen() && scene && scene->OnEscape()) {
        return;
    }

    if (pauseMenu.IsOpen()) {
        pauseMenu.Close();

        // One level further back: out of the simulation, into the editor
        if (simulating) {
            SetSimulating(false);
        }

        return;
    }

    OpenPauseMenu();

    // A scene without a pause menu, e.g. the main menu, would swallow ESC
    // during the simulation. It ends the simulation instead.
    if (!pauseMenu.IsOpen() && simulating) {
        SetSimulating(false);
    }
}

void Engine::UpdatePauseMenu() {
    Menu::Event event = pauseMenu.Update(
        renderer.MouseViewportPosition(),
        mouse.IsLeftClicked(),
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

// ":" opens the console, as long as nothing else is being typed. An entered
// line runs one of the commands, see ConsoleCommands.
void Engine::UpdateConsole() {
    if (!console.IsOpen()) {
        if (!IsTyping()) {
            console.CheckOpen();
        }

        if (console.IsOpen()) {
            input.SetBlocked(true);
        }

        return;
    }

    std::string line = console.Update();

    if (line.empty()) {
        return;
    }

    console.Print(std::string(1, static_cast<char>(Console::OPEN_CHARACTER)) + line);
    console.Print(commands.Run(line));
}

void Engine::CloseConsole() {
    console.Close();

    input.SetBlocked(false);
}

void Engine::AddEngineCommands() {
    commands.Add({"help", "", 0, [this](const std::vector<std::string> &) {
        std::string lines;

        for (const ConsoleCommand &command: commands.All()) {
            lines += (lines.empty() ? "" : "\n") + ConsoleCommands::Usage(command);
        }

        return lines;
    }});

    commands.Add({"clear", "", 0, [this](const std::vector<std::string> &) {
        console.ClearHistory();

        return std::string();
    }});
}

void Engine::UpdateSettingsMenu() {
    Menu::Event event = settingsMenu.Update(
        renderer.MouseViewportPosition(),
        mouse.IsLeftClicked(),
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

        case SettingsItem::Soundcard: {
            // A dropdown: only picking an option changes something
            const std::vector<Soundcard> &cards = Soundcard::All();

            if (event.action != MenuAction::Choose || event.option < 0 ||
                event.option >= static_cast<int>(cards.size())) {
                return;
            }

            settings.soundcard = cards[static_cast<std::size_t>(event.option)].id;

            // The other version of the same music continues where it was
            ApplyMusic(sceneMusic, true);
            break;
        }

        case SettingsItem::Fullscreen:
            settings.fullscreen = !settings.fullscreen;
            break;

        case SettingsItem::ShowFps:
            settings.showFps = !settings.showFps;
            break;

        case SettingsItem::Hitboxes:
            settings.showHitboxes = !settings.showHitboxes;
            break;

        case SettingsItem::ObjectIds:
            settings.showObjectIds = !settings.showObjectIds;
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

            case SettingsItem::Soundcard: {
                Menu::Entry entry{"Soundcard", Soundcard::Find(settings.soundcard).label};

                for (const Soundcard &card: Soundcard::All()) {
                    entry.options.push_back(card.label);
                }

                entries.push_back(std::move(entry));
                break;
            }

            case SettingsItem::Fullscreen:
                entries.push_back(ToggleEntry("Fullscreen", settings.fullscreen));
                break;

            case SettingsItem::ShowFps:
                entries.push_back(ToggleEntry("Show FPS", settings.showFps));
                break;

            case SettingsItem::Hitboxes:
                entries.push_back(ToggleEntry("Hitboxes", settings.showHitboxes));
                break;

            case SettingsItem::ObjectIds:
                entries.push_back(ToggleEntry("Object ids", settings.showObjectIds));
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

    // Above the scene and its windows, below the menus of the engine
    console.Draw(renderer.GetWidth(), renderer.GetHeight(), font);

    // Last, so the overlay also covers the scene's interface
    pauseMenu.Draw(renderer.GetWidth(), renderer.GetHeight());
    settingsMenu.Draw(renderer.GetWidth(), renderer.GetHeight());

    // The mouse cursor lies above everything
    DrawCursor();

    // A window in the pixels of the screen, e.g. the code editor, lies above
    // the finished picture. The cursor is drawn again on top of it.
    renderer.EndDraw([this] {
        if (!scene || !scene->DrawsScreenLayer()) {
            return;
        }

        scene->DrawScreen();

        DrawScreenCursor();
    });
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

// The same hand as DrawCursor, but in the pixels of the window: the screen
// layer lies above the viewport the other one was drawn in
void Engine::DrawScreenCursor() const {
    bool grab = cursor == CursorState::Grab;

    char icon = grab ? FontRenderer::ICON_GRAB : FontRenderer::ICON_POINTER;
    Vector2 hotspot = grab ? CURSOR_GRAB_HOTSPOT : CURSOR_POINTER_HOTSPOT;
    int variant = cursor == CursorState::Idle ? FontVariant::White : FontVariant::Yellow;

    int scale = renderer.GetScale();
    Vector2 mouse = GetMousePosition();

    font.Draw(
        std::string(1, icon),
        {mouse.x - hotspot.x * static_cast<float>(scale), mouse.y - hotspot.y * static_cast<float>(scale)},
        variant,
        TextSpacing::Normal,
        scale
    );
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
        {8.0f, static_cast<float>(renderer.GetHeight()) - static_cast<float>(font.LetterHeight()) - MODE_LABEL_MARGIN},
        theme.hoverVariant,
        TextSpacing::Narrow
    );
}

void Engine::UpdateDevelopmentCamera(float dt) {
    if (IsTyping()) {
        return;
    }

    // Mouse wheel and space zoom, if the scene allows it. Over the windows of
    // the scene bar the zoom rests.
    bool zoom = scene && scene->Options().developmentZoom;

    if (zoom && !sceneBar.HasOpenWindow()) {
        devCamera.Scroll(GetMouseWheelMove(), renderer.MouseWorldPosition());

        if (WasKeyPressed(KEY_SPACE)) {
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
