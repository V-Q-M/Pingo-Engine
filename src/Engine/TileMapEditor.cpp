#include "TileMapEditor.h"

#include <string>
#include <utility>

// Dark box behind the digit so it stays readable on every tile
constexpr Color EDITOR_LABEL_BACKGROUND{24, 20, 37, 170};

constexpr Color EDITOR_HOVER{255, 255, 255, 140};
constexpr Color EDITOR_SELECTED{255, 229, 26, 255};

constexpr int EDITOR_VARIANT_IDLE = FontVariant::White;
constexpr int EDITOR_VARIANT_SELECTED = FontVariant::Yellow;

TileMapEditor::TileMapEditor(TileMap &map)
    : map(map) {
}

bool TileMapEditor::HasSelection() const {
    return selectedColumn >= 0 && selectedRow >= 0;
}

bool TileMapEditor::IsMenuOpen() const {
    return menu.IsOpen();
}

void TileMapEditor::CloseMenu() {
    menu.Close();
}

bool TileMapEditor::IsHovering() const {
    return menu.IsHovering();
}

const ContextMenu &TileMapEditor::TileMenu() const {
    return menu;
}

const std::vector<int> &TileMapEditor::MenuTiles() const {
    return menuTiles;
}

void TileMapEditor::Update(const Input &input, const FontRenderer &font) {
    int column = -1;
    int row = -1;

    if (!map.CellAt(input.mouseWorld, column, row)) {
        column = -1;
        row = -1;
    }

    bool onMenu = menu.IsOpen() && CheckCollisionPointRec(input.mouseViewport, menu.Bounds(font));

    if (input.rightClicked && !onMenu) {
        // A right click selects the cell and opens its menu, also again at a
        // different position
        menu.Close();

        selectedColumn = column;
        selectedRow = row;

        if (HasSelection()) {
            OpenMenu(input, font);
        }
    } else if (menu.IsOpen()) {
        // As long as the menu is open, the left mouse button belongs to it. A click
        // next to it closes it without selecting a different cell.
        int item = menu.Update(input.mouseViewport, input.leftClicked, font);

        if (item != ContextMenu::NOTHING) {
            ApplyDigit(menuTiles[static_cast<std::size_t>(item)]);
        }
    } else if (input.leftClicked) {
        selectedColumn = column;
        selectedRow = row;
    }

    // Below the menu the mouse highlight rests
    hoveredColumn = menu.IsOpen() ? -1 : column;
    hoveredRow = menu.IsOpen() ? -1 : row;

    // GetCharPressed returns characters instead of keys and therefore works with
    // every keyboard layout. A typed digit replaces the choice in the menu.
    for (int character = GetCharPressed(); character != 0; character = GetCharPressed()) {
        if (character >= '0' && character <= '9') {
            ApplyDigit(character - '0');
            menu.Close();
        }
    }
}

void TileMapEditor::OpenMenu(const Input &input, const FontRenderer &font) {
    const TileSet &tileSet = map.GetTileSet();

    int current = map.TileAt(selectedColumn, selectedRow);

    menuTiles = tileSet.AvailableTiles();

    std::vector<std::string> items;

    for (int tile: menuTiles) {
        std::string name = tile == TileMap::EMPTY ? "Empty" : tileSet.Name(tile);

        items.push_back(name.empty() ? std::to_string(tile) : std::to_string(tile) + " " + name);
    }

    menu.SetTitle("Tile");
    menu.SetItems(std::move(items));

    // The arrow shows what is currently on the cell
    for (std::size_t i = 0; i < menuTiles.size(); i++) {
        if (menuTiles[i] == current) {
            menu.SetItemIcon(i, FontRenderer::ARROW_RIGHT);
        }
    }

    menu.Open(
        {input.mouseViewport.x + MENU_OFFSET, input.mouseViewport.y + MENU_OFFSET},
        input.viewWidth,
        input.viewHeight,
        font
    );
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
    menu.Draw(font);
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

            DrawRectangle(x + 1, y + 1, font.LetterWidth() + 1, font.LetterHeight() + 2, EDITOR_LABEL_BACKGROUND);

            font.Draw(
                std::string(1, static_cast<char>('0' + map.TileAt(column, row))),
                {static_cast<float>(x + 1), static_cast<float>(y + 2)},
                selected ? EDITOR_VARIANT_SELECTED : EDITOR_VARIANT_IDLE
            );
        }
    }
}
