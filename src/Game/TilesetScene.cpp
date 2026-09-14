#include "TilesetScene.h"

#include <algorithm>

#include "Themes.h"
#include "Engine/Engine.h"

static SceneOptions TilesetOptions(const std::string &id) {
    SceneOptions options;

    options.id = id;
    options.pauseMenu = true;
    options.pauseLeaveLabel = "Return to Main Menu";
    options.movement = true;
    options.developmentZoom = true;
    options.theme = DefaultTheme();

    return options;
}

// The start cell, limited to the map
static int StartCell(int preferred, int count) {
    return std::clamp(preferred, 0, std::max(count - 1, 0));
}

TilesetScene::TilesetScene(Engine &engine, const std::string &id)
    : FieldScene(engine, TilesetOptions(id), "Tileset"),
      map(TileMap::Load("maps/" + id + ".txt", TileSet::Load(engine.GetAssets(), "tilesets/overworld.json"))),
      startColumn(StartCell(START_COLUMN, map.Columns())),
      startRow(StartCell(START_ROW, map.Rows())),
      mover(map, startColumn, startRow),
      editor(map) {
    // The map only has the player character
    heroes.Add(engine.GetAssets(), "characters/pingo.json", map.CellFeet(startColumn, startRow));

    SetPlayer(0);

    SetWorld(map.Area(), BoundsMode::Follow);
}

// In the Development mode: click tiles and set them with digit keys, or
// choose one from the menu with a right click
void TilesetScene::UpdateDevelopment(float) {
    Renderer &renderer = engine.GetRenderer();

    TileMapEditor::Input input;
    input.mouseWorld = renderer.MouseWorldPosition();
    input.mouseViewport = renderer.MouseViewportPosition();
    input.leftClicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    input.rightClicked = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
    input.viewWidth = renderer.GetWidth();
    input.viewHeight = renderer.GetHeight();

    editor.Update(input, engine.GetFont());

    if (editor.IsHovering()) {
        engine.RequestCursor(CursorState::Hover);
    }
}

void TilesetScene::DrawUI() {
    FieldScene::DrawUI();

    // The menu belongs above everything else of the scene
    if (engine.GetConfig().IsDevelopment()) {
        editor.DrawMenu(engine.GetFont());
    }
}

bool TilesetScene::OnEscape() {
    if (engine.GetConfig().IsDevelopment() && editor.IsMenuOpen()) {
        editor.CloseMenu();
        return true;
    }

    return FieldScene::OnEscape();
}

void TilesetScene::LeaveFromPauseMenu() {
    engine.EnterEntryPoint();
}

// On the grid the map decides where to go. Free collision and world bounds
// of the FieldScene are not needed for that.
void TilesetScene::MovePlayer(Character &player, float dt, const std::vector<SpriteInstance *> &) {
    mover.Update(
        player.GetCharacter(),
        engine.GetInput().MovementDirection(),
        player.MoveSpeed(),
        dt
    );
}

void TilesetScene::DrawGround() {
    map.Draw();
}

void TilesetScene::DrawOverlay() {
    if (engine.GetConfig().IsDevelopment()) {
        editor.Draw(engine.GetFont());
    }
}
