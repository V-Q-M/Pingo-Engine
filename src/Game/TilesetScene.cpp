#include "TilesetScene.h"

#include <algorithm>

#include "Themes.h"
#include "Engine/Engine.h"
#include "Scripting/Api/ScriptRegistry.h"

// A script whose file exists but which is not compiled into the game yet,
// the same grey as in the add menu of normal scenes
constexpr int UNBUILT_TILE_SCRIPT_VARIANT = FontVariant::Grey;

constexpr const char *TRAVEL_CONFIRM = "Confirm";
constexpr const char *TRAVEL_CANCEL = "Cancel";

static SceneOptions TilesetOptions(Engine &engine, const std::string &id) {
    SceneOptions options;

    options.id = id;
    options.pauseMenu = true;
    options.pauseLeaveLabel = "Return to Main Menu";
    options.movement = true;
    options.developmentZoom = true;

    // On the map the player character only walks, clicking it does nothing
    options.selectablePlayer = false;

    // Tile scripts, see TilesetScene::ScriptKind
    options.scripts = true;
    options.theme = DefaultTheme();

    // Chosen in the scene bar, like tile atlas and tile data
    options.music = engine.GetScenes().Choice(id, SceneCatalog::MUSIC_CHOICE);

    return options;
}

// The start cell, limited to the map
static int StartCell(int preferred, int count) {
    return std::clamp(preferred, 0, std::max(count - 1, 0));
}

// Tile data and tile atlas as chosen for the scene in the scene bar
static TileSet LoadTileSet(Engine &engine, const std::string &id) {
    const SceneCatalog &scenes = engine.GetScenes();

    return TileSet::Load(
        engine.GetAssets(),
        scenes.Choice(id, TilesetScene::TILE_DATA_CHOICE),
        scenes.Choice(id, TilesetScene::TILE_ATLAS_CHOICE)
    );
}

TilesetScene::TilesetScene(Engine &engine, const std::string &id)
    : FieldScene(engine, TilesetOptions(engine, id), "Tileset"),
      map(TileMap::Load("maps/" + id + ".txt", LoadTileSet(engine, id))),
      startColumn(StartCell(START_COLUMN, map.Columns())),
      startRow(StartCell(START_ROW, map.Rows())),
      mover(map, startColumn, startRow),
      previousColumn(startColumn),
      previousRow(startRow),
      unreadyColumn(startColumn),
      unreadyRow(startRow),
      editor(map) {
    // The map only has the player character
    characters.Add(engine.GetAssets(), "characters/pingo.json", map.CellFeet(startColumn, startRow), ObjectGroups::ALLY);

    SetPlayer(0);

    SetWorld(map.Area(), BoundsMode::Follow);

    travelDialog.SetButtons(TRAVEL_CONFIRM, TRAVEL_CANCEL);

    // The editor offers the script files, the game runs them
    if (engine.GetConfig().IsDevelopment()) {
        LoadScripts();
    } else {
        LoadTileScripts();
    }
}

void TilesetScene::LoadTileScripts() {
    for (int row = 0; row < map.Rows(); row++) {
        for (int column = 0; column < map.Columns(); column++) {
            const std::string &script = map.ScriptAt(column, row);

            if (!script.empty()) {
                scripts.AddTileScript(Options().id, script, *this, column, row);
            }
        }
    }
}

ScriptFiles::Kind TilesetScene::ScriptKind() const {
    return ScriptFiles::Kind::Tile;
}

// A script the game does not know yet only runs after the next build
void TilesetScene::ScriptsChanged() {
    std::vector<int> variants;

    for (const std::string &script: scriptNames) {
        bool built = ScriptRegistry::Knows(Options().id, script);

        variants.push_back(built ? FontVariant::White : UNBUILT_TILE_SCRIPT_VARIANT);
    }

    editor.SetScripts(scriptNames, variants);
}

void TilesetScene::ScriptCreated(const std::string &name) {
    if (newScriptColumn >= 0 && newScriptRow >= 0) {
        editor.SetScript(newScriptColumn, newScriptRow, name);
    }

    newScriptColumn = -1;
    newScriptRow = -1;
}

void TilesetScene::ScriptRenamed(const std::string &from, const std::string &to) {
    editor.RenameScript(from, to);
}

// A cell without its script file would only log a warning on every start
void TilesetScene::ScriptRemoved(const std::string &name) {
    editor.RemoveScript(name);
}

void TilesetScene::ApplyEditorRequest(const TileMapEditor::Request &request) {
    switch (request.kind) {
        case TileMapEditor::Request::Kind::Open:
            OpenScript(request.script);
            break;

        case TileMapEditor::Request::Kind::New:
            newScriptColumn = request.column;
            newScriptRow = request.row;

            OpenScriptForm("");
            break;

        case TileMapEditor::Request::Kind::Rename:
            OpenScriptForm(request.script);
            break;

        case TileMapEditor::Request::Kind::Delete:
            AskDeleteScript(request.script);
            break;

        case TileMapEditor::Request::Kind::None:
            break;
    }
}

void TilesetScene::Update(float dt) {
    if (travelDialog.IsOpen()) {
        UpdateTravelDialog();
        return;
    }

    FieldScene::Update(dt);
}

void TilesetScene::UpdateTravelDialog() {
    // The hands are on the keyboard while walking, so Enter confirms as well
    // (ESC cancels, see OnEscape)
    ConfirmDialog::Result result = engine.WasKeyPressed(KEY_ENTER) || engine.WasKeyPressed(KEY_KP_ENTER)
                                       ? ConfirmDialog::Result::Yes
                                       : travelDialog.Update(
                                           engine.GetRenderer().MouseViewportPosition(),
                                           engine.GetMouse().IsLeftClicked(),
                                           engine.GetFont()
                                       );

    if (travelDialog.IsHovering()) {
        engine.RequestCursor(CursorState::Hover);
    }

    if (result == ConfirmDialog::Result::Yes) {
        travelDialog.Close();
        engine.EnterScene(pendingTravel);
        pendingTravel.clear();
    } else if (result == ConfirmDialog::Result::No) {
        CancelTravel();
    }
}

// The player still stands on the tile that asked: without being unready it
// would ask again in the next frame, e.g. from IsPlayerOn
void TilesetScene::CancelTravel() {
    travelDialog.Close();
    pendingTravel.clear();

    SetPlayerReady(false);
}

// In the Development mode: click tiles and set them with digit keys, or
// choose a tile or a script from the menu with a right click
void TilesetScene::UpdateDevelopment(float) {
    // Code editor, script window and delete question lie above the map
    if (UpdateScriptWindows()) {
        return;
    }

    Renderer &renderer = engine.GetRenderer();

    TileMapEditor::Input input;
    input.mouseWorld = renderer.MouseWorldPosition();
    input.mouseViewport = renderer.MouseViewportPosition();
    input.leftClicked = engine.GetMouse().IsLeftClicked();
    input.rightClicked = engine.GetMouse().IsRightClicked();
    input.keyboardBusy = engine.GetConsole().IsOpen();
    input.viewWidth = renderer.GetWidth();
    input.viewHeight = renderer.GetHeight();

    ApplyEditorRequest(editor.Update(input, engine.GetFont()));

    if (editor.IsHovering()) {
        engine.RequestCursor(CursorState::Hover);
    }
}

void TilesetScene::DrawUI() {
    FieldScene::DrawUI();

    travelDialog.Draw(engine.GetFont());

    // The menu belongs above everything else of the scene
    if (engine.GetConfig().IsDevelopment()) {
        editor.DrawMenu(engine.GetFont());
    }
}

// The travel question and the script windows first, they lie above the menu
bool TilesetScene::OnEscape() {
    if (travelDialog.IsOpen()) {
        CancelTravel();
        return true;
    }

    if (FieldScene::OnEscape()) {
        return true;
    }

    if (engine.GetConfig().IsDevelopment() && editor.IsMenuOpen()) {
        editor.CloseMenu();
        return true;
    }

    return false;
}

void TilesetScene::LeaveFromPauseMenu() {
    engine.EnterEntryPoint();
}

// On the grid the map decides where to go. Free collision and world bounds
// of the FieldScene are not needed for that.
void TilesetScene::MovePlayer(Character &player, float dt, const std::vector<SpriteInstance *> &) {
    // The scripts run after the step and compare, see Tile::HasPlayerEntered
    previousColumn = mover.Column();
    previousRow = mover.Row();

    mover.Update(
        player.GetCharacter(),
        engine.GetInput().MovementDirection(),
        player.MoveSpeed(),
        dt
    );

    // Arrived somewhere else: from here on tiles react again, before the
    // scripts of this frame run
    if (!playerReady && (mover.Column() != unreadyColumn || mover.Row() != unreadyRow)) {
        playerReady = true;
    }
}

void TilesetScene::DrawGround() {
    map.Draw();
}

void TilesetScene::DrawOverlay() {
    if (engine.GetConfig().IsDevelopment()) {
        editor.Draw(engine.GetFont());
    }
}

int TilesetScene::MapColumns() const {
    return map.Columns();
}

int TilesetScene::MapRows() const {
    return map.Rows();
}

int TilesetScene::TileNumberAt(int column, int row) const {
    return map.TileAt(column, row);
}

// Only the map in memory changes: while the game runs nobody saves it
bool TilesetScene::SetTileNumberAt(int column, int row, int number) {
    return map.SetTile(column, row, number);
}

std::string TilesetScene::TileNameAt(int column, int row) const {
    return map.GetTileSet().Name(map.TileAt(column, row));
}

bool TilesetScene::IsTileWalkable(int column, int row) const {
    return map.IsWalkable(column, row);
}

int TilesetScene::PlayerColumn() const {
    return mover.Column();
}

int TilesetScene::PlayerRow() const {
    return mover.Row();
}

int TilesetScene::PreviousPlayerColumn() const {
    return previousColumn;
}

int TilesetScene::PreviousPlayerRow() const {
    return previousRow;
}

bool TilesetScene::IsPlayerPushingAgainst(int column, int row) const {
    return mover.IsPushingAgainst(column, row);
}

bool TilesetScene::IsPlayerReady() const {
    return playerReady;
}

void TilesetScene::SetPlayerReady(bool ready) {
    playerReady = ready;

    unreadyColumn = mover.Column();
    unreadyRow = mover.Row();
}

bool TilesetScene::SwitchToScene(const std::string &id, bool confirm) {
    const SceneEntry *entry = engine.GetScenes().Find(id);

    if (entry == nullptr) {
        TraceLog(LOG_WARNING, "SCRIPTS: Szene \"%s\" gibt es nicht", id.c_str());
        return false;
    }

    if (!confirm) {
        return engine.EnterScene(id);
    }

    // Several tiles may ask in the same frame, the first question stays
    if (travelDialog.IsOpen()) {
        return false;
    }

    pendingTravel = id;

    Renderer &renderer = engine.GetRenderer();

    travelDialog.Open(
        "Travel to " + entry->name + "?",
        "",
        renderer.GetWidth(),
        renderer.GetHeight(),
        engine.GetFont()
    );

    return true;
}
