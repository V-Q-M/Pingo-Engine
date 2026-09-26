#include "TileMapEditor.h"

#include <string>
#include <utility>

// Dark box behind the digit so it stays readable on every tile
constexpr Color EDITOR_LABEL_BACKGROUND{24, 20, 37, 170};

constexpr Color EDITOR_HOVER{255, 255, 255, 140};
constexpr Color EDITOR_SELECTED{255, 229, 26, 255};

constexpr int EDITOR_VARIANT_IDLE = FontVariant::White;
constexpr int EDITOR_VARIANT_SELECTED = FontVariant::Yellow;

// The corner of a cell with a script: light blue on a dark edge, so it stands
// out on grass, water and the yellow selection alike
constexpr Color EDITOR_SCRIPT_MARKER{41, 173, 255, 255};
constexpr Color EDITOR_SCRIPT_MARKER_EDGE{24, 20, 37, 255};
constexpr int EDITOR_SCRIPT_MARKER_SIZE = 5;

// "+New" at the end of the scripts, like in the add menu of normal scenes
constexpr const char *EDITOR_NEW_SCRIPT = "+New";
constexpr int EDITOR_NEW_SCRIPT_VARIANT = FontVariant::Grey;

TileMapEditor::TileMapEditor(TileMap &map)
    : map(map) {
    // Same order as Section
    sectionMenu.SetTitle("Tile");
    sectionMenu.SetItems({"Tiles", "Scripts"});

    scriptMenu.SetTitle("Scripts");
}

bool TileMapEditor::HasSelection() const {
    return selectedColumn >= 0 && selectedRow >= 0;
}

bool TileMapEditor::IsMenuOpen() const {
    return sectionMenu.IsOpen() || menu.IsOpen() || scriptMenu.IsOpen() || scriptActions.IsOpen();
}

void TileMapEditor::CloseMenu() {
    sectionMenu.Close();
    menu.Close();
    scriptMenu.Close();
    scriptActions.Close();
}

bool TileMapEditor::IsHovering() const {
    return sectionMenu.IsHovering() || menu.IsHovering() || scriptMenu.IsHovering() || scriptActions.IsHovering();
}

const ContextMenu &TileMapEditor::TileMenu() const {
    return menu;
}

const std::vector<int> &TileMapEditor::MenuTiles() const {
    return menuTiles;
}

void TileMapEditor::SetScripts(std::vector<std::string> names, std::vector<int> variants) {
    hasScripts = true;

    scriptNames = std::move(names);
    scriptVariants = std::move(variants);

    // The list may have changed under an open menu, e.g. after a rename
    scriptMenu.Close();
    scriptActions.Close();
}

void TileMapEditor::HandleRightClick(int column, int row, const Input &input, const FontRenderer &font) {
    // The actions lie above the scripts, a right click on them does nothing
    if (scriptActions.IsOpen() && CheckCollisionPointRec(input.mouseViewport, scriptActions.Bounds(font))) {
        return;
    }

    // In the open scripts menu a right click on a script opens its actions.
    // Nothing happens on the title or on "+New".
    if (scriptMenu.IsOpen() && CheckCollisionPointRec(input.mouseViewport, scriptMenu.Bounds(font))) {
        int item = scriptMenu.ItemAt(input.mouseViewport, font);

        scriptActions.Close();

        if (item != ContextMenu::NOTHING && static_cast<std::size_t>(item) < scriptNames.size()) {
            OpenScriptActions(static_cast<std::size_t>(item), input, font);
        }

        return;
    }

    for (const ContextMenu *open: {&sectionMenu, &menu}) {
        if (open->IsOpen() && CheckCollisionPointRec(input.mouseViewport, open->Bounds(font))) {
            return;
        }
    }

    // Anywhere else it selects the cell and opens its menu, also again at a
    // different position
    CloseMenu();

    selectedColumn = column;
    selectedRow = row;

    if (!HasSelection()) {
        return;
    }

    Vector2 position{input.mouseViewport.x + MENU_OFFSET, input.mouseViewport.y + MENU_OFFSET};

    if (hasScripts) {
        sectionMenu.Open(position, input.viewWidth, input.viewHeight, font);
    } else {
        OpenMenu(position, input, font);
    }
}

TileMapEditor::Request TileMapEditor::Update(const Input &input, const FontRenderer &font) {
    Request request;

    int column = -1;
    int row = -1;

    if (!map.CellAt(input.mouseWorld, column, row)) {
        column = -1;
        row = -1;
    }

    // As long as a menu is open, the left mouse button belongs to it. A click
    // next to it closes it without selecting a different cell. The actions of
    // a script lie above the scripts and only close themselves.
    if (input.rightClicked) {
        HandleRightClick(column, row, input, font);
    } else if (scriptActions.IsOpen()) {
        int action = scriptActions.Update(input.mouseViewport, input.leftClicked, font);

        if (action != ContextMenu::NOTHING) {
            request = ApplyScriptAction(action);
        }
    } else if (scriptMenu.IsOpen()) {
        int item = scriptMenu.Update(input.mouseViewport, input.leftClicked, font);

        if (item != ContextMenu::NOTHING) {
            auto index = static_cast<std::size_t>(item);

            request.kind = index < scriptNames.size() ? Request::Kind::Open : Request::Kind::New;
            request.script = index < scriptNames.size() ? scriptNames[index] : "";
            request.column = selectedColumn;
            request.row = selectedRow;
        }
    } else if (menu.IsOpen()) {
        int item = menu.Update(input.mouseViewport, input.leftClicked, font);

        if (item != ContextMenu::NOTHING) {
            ApplyDigit(menuTiles[static_cast<std::size_t>(item)]);
        }
    } else if (sectionMenu.IsOpen()) {
        // The chosen part opens where the first menu stood
        Rectangle bounds = sectionMenu.Bounds(font);

        int section = sectionMenu.Update(input.mouseViewport, input.leftClicked, font);

        if (section == static_cast<int>(Section::Tiles)) {
            OpenMenu({bounds.x, bounds.y}, input, font);
        } else if (section == static_cast<int>(Section::Scripts)) {
            OpenScriptMenu({bounds.x, bounds.y}, input, font);
        }
    } else if (input.leftClicked) {
        selectedColumn = column;
        selectedRow = row;
    }

    // Below the menu the mouse highlight rests
    hoveredColumn = IsMenuOpen() ? -1 : column;
    hoveredRow = IsMenuOpen() ? -1 : row;

    if (input.keyboardBusy) {
        return request;
    }

    // GetCharPressed returns characters instead of keys and therefore works with
    // every keyboard layout. A typed digit replaces the choice in the menu.
    for (int character = GetCharPressed(); character != 0; character = GetCharPressed()) {
        if (character >= '0' && character <= '9') {
            ApplyDigit(character - '0');
            CloseMenu();
        }
    }

    return request;
}

void TileMapEditor::OpenMenu(Vector2 position, const Input &input, const FontRenderer &font) {
    const TileSet &tileSet = map.GetTileSet();

    int current = map.TileAt(selectedColumn, selectedRow);

    menuTiles = tileSet.AvailableTiles();

    std::vector<std::string> items;

    for (int tile: menuTiles) {
        std::string name = tile == TileMap::EMPTY ? "Empty" : tileSet.Name(tile);

        items.push_back(name.empty() ? std::to_string(tile) : std::to_string(tile) + " " + name);
    }

    menu.SetTitle("Tiles");
    menu.SetItems(std::move(items));

    // The arrow shows what is currently on the cell
    for (std::size_t i = 0; i < menuTiles.size(); i++) {
        if (menuTiles[i] == current) {
            menu.SetItemIcon(i, FontRenderer::ARROW_RIGHT);
        }
    }

    menu.Open(position, input.viewWidth, input.viewHeight, font);
}

void TileMapEditor::OpenScriptMenu(Vector2 position, const Input &input, const FontRenderer &font) {
    std::vector<std::string> items = scriptNames;

    items.push_back(EDITOR_NEW_SCRIPT);

    scriptMenu.SetItems(std::move(items));

    for (std::size_t i = 0; i < scriptVariants.size() && i < scriptNames.size(); i++) {
        scriptMenu.SetItemVariant(i, scriptVariants[i]);
    }

    scriptMenu.SetItemVariant(scriptNames.size(), EDITOR_NEW_SCRIPT_VARIANT);

    MarkAssignedScript();

    scriptMenu.Open(position, input.viewWidth, input.viewHeight, font);
}

void TileMapEditor::MarkAssignedScript() {
    const std::string &current = map.ScriptAt(selectedColumn, selectedRow);

    for (std::size_t i = 0; i < scriptNames.size(); i++) {
        scriptMenu.SetItemIcon(i, scriptNames[i] == current ? FontRenderer::ARROW_RIGHT : 0);
    }
}

void TileMapEditor::OpenScriptActions(std::size_t script, const Input &input, const FontRenderer &font) {
    actionScript = script;

    bool assigned = map.ScriptAt(selectedColumn, selectedRow) == scriptNames[script];

    // Same order as ScriptAction. The first one takes the script off again
    // if the cell already has it.
    scriptActions.SetTitle(scriptNames[script]);
    scriptActions.SetItems({assigned ? "Unassign" : "Assign", "Rename", "Delete"});
    scriptActions.SetItemIcon(0, FontRenderer::ARROW_RIGHT);
    scriptActions.SetItemIcon(1, FontRenderer::ICON_EDIT);
    scriptActions.SetItemIcon(2, FontRenderer::ICON_DELETE);

    scriptActions.Open(
        {input.mouseViewport.x + MENU_OFFSET, input.mouseViewport.y + MENU_OFFSET},
        input.viewWidth,
        input.viewHeight,
        font
    );
}

TileMapEditor::Request TileMapEditor::ApplyScriptAction(int action) {
    Request request;

    if (actionScript >= scriptNames.size()) {
        return request;
    }

    const std::string script = scriptNames[actionScript];

    switch (static_cast<ScriptAction>(action)) {
        case ScriptAction::Assign: {
            bool assigned = map.ScriptAt(selectedColumn, selectedRow) == script;

            SetScript(selectedColumn, selectedRow, assigned ? "" : script);

            // The scripts stay open, the arrow moves along
            MarkAssignedScript();

            return request;
        }

        case ScriptAction::Rename:
            request.kind = Request::Kind::Rename;
            break;

        case ScriptAction::Delete:
            request.kind = Request::Kind::Delete;
            break;
    }

    // A window or a question follows, the list may change
    CloseMenu();

    request.script = script;
    request.column = selectedColumn;
    request.row = selectedRow;

    return request;
}

bool TileMapEditor::SetScript(int column, int row, const std::string &script) {
    if (map.ScriptAt(column, row) == script || !map.SetScript(column, row, script)) {
        return false;
    }

    return Save();
}

bool TileMapEditor::RenameScript(const std::string &from, const std::string &to) {
    return map.RenameScript(from, to) > 0 && Save();
}

bool TileMapEditor::RemoveScript(const std::string &script) {
    return map.RemoveScript(script) > 0 && Save();
}

bool TileMapEditor::ApplyDigit(int tile) {
    if (!HasSelection() || map.TileAt(selectedColumn, selectedRow) == tile) {
        return false;
    }

    if (!map.SetTile(selectedColumn, selectedRow, tile)) {
        return false;
    }

    return Save();
}

bool TileMapEditor::Save() const {
    bool saved = map.SaveAsset();

    if (saved) {
        TraceLog(LOG_INFO, "TILEMAP: [%s] gespeichert", map.Filename().c_str());
    } else {
        TraceLog(LOG_WARNING, "TILEMAP: [%s] konnte nicht gespeichert werden", map.Filename().c_str());
    }

    return saved;
}

void TileMapEditor::DrawMenu(const FontRenderer &font) const {
    sectionMenu.Draw(font);
    menu.Draw(font);
    scriptMenu.Draw(font);
    scriptActions.Draw(font);
}

// A triangle in the top right corner, built from single pixel columns so it
// stays sharp: every column one pixel longer than the one to its left. The
// dark one is one pixel larger and shows as an edge along the slope.
void TileMapEditor::DrawScriptMarker(int x, int y, int size) const {
    int right = x + size;

    for (int i = 0; i <= EDITOR_SCRIPT_MARKER_SIZE; i++) {
        DrawRectangle(right - EDITOR_SCRIPT_MARKER_SIZE - 1 + i, y, 1, i + 1, EDITOR_SCRIPT_MARKER_EDGE);
    }

    for (int i = 0; i < EDITOR_SCRIPT_MARKER_SIZE; i++) {
        DrawRectangle(right - EDITOR_SCRIPT_MARKER_SIZE + i, y, 1, i + 1, EDITOR_SCRIPT_MARKER);
    }
}

void TileMapEditor::Draw(const FontRenderer &font) const {
    int size = map.TileSize();

    // The frames first: the digits lie above them and so stay readable even on
    // the selected cell
    if (hoveredColumn >= 0) {
        DrawRectangleLines(hoveredColumn * size, hoveredRow * size, size, size, EDITOR_HOVER);
    }

    // Double frame, so the selection stands out even on yellow flowers
    if (HasSelection()) {
        int x = selectedColumn * size;
        int y = selectedRow * size;

        DrawRectangleLines(x, y, size, size, EDITOR_SELECTED);
        DrawRectangleLines(x + 1, y + 1, size - 2, size - 2, EDITOR_SELECTED);
    }

    for (int row = 0; row < map.Rows(); row++) {
        for (int column = 0; column < map.Columns(); column++) {
            int x = column * size;
            int y = row * size;

            bool selected = column == selectedColumn && row == selectedRow;

            if (!map.ScriptAt(column, row).empty()) {
                DrawScriptMarker(x, y, size);
            }

            DrawRectangle(x + 1, y + 1, font.LetterWidth() + 1, font.LetterHeight() + 2, EDITOR_LABEL_BACKGROUND);

            font.Draw(
                std::string(1, static_cast<char>('0' + map.TileAt(column, row))),
                {static_cast<float>(x + 1), static_cast<float>(y + 2)},
                selected ? EDITOR_VARIANT_SELECTED : EDITOR_VARIANT_IDLE
            );
        }
    }
}
