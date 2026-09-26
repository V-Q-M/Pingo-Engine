#pragma once

#include <string>
#include <vector>

#include "raylib.h"

#include "ContextMenu.h"
#include "FontRenderer.h"
#include "TileMap.h"

// Developer tool for a TileMap.
//
// Shows its digit on every cell. A click selects a cell, a digit key sets it
// and saves the map right away. A right click also selects the cell and opens
// a menu with two parts, like the add menu of the ObjectEditor:
//
// - Tiles: all tiles of the TileSet, a click on one sets it.
// - Scripts: the scripts the scene offers, see SetScripts. A click opens one,
//   "+New" asks for a new one, both are left to the scene, see Request. A
//   right click on a script assigns it to the cell or takes it off, renames
//   or deletes it. The arrow marks the script on the cell.
//
// Without scripts, see SetScripts, the right click opens the tiles right
// away. Cells with a script carry a small marker in their top right corner.
// The selection stays afterwards, a click next to the map clears it.
//
// It saves to the copy in the build directory, which the game reads from,
// and in the Development build also to the asset folder of the sources. Only
// then does the change survive the next build.
class TileMapEditor {
public:
    struct Input {
        // The mouse in world coordinates, for the cells
        Vector2 mouseWorld{0.0f, 0.0f};

        // The same mouse in viewport coordinates, for the menu
        Vector2 mouseViewport{0.0f, 0.0f};

        bool leftClicked = false;
        bool rightClicked = false;

        // Something else owns the keyboard, e.g. the console: typed digits
        // then do not change a tile
        bool keyboardBusy = false;

        int viewWidth = 0;
        int viewHeight = 0;
    };

    // What the scripts menu asks the scene for. Assigning a script to a cell
    // is done by the editor itself, everything that touches the script files
    // is up to the scene.
    struct Request {
        enum class Kind {
            None,

            // Open script in the code editor
            Open,

            // Ask for the name of a new script, for the cell
            New,

            Rename,
            Delete
        };

        Kind kind = Kind::None;

        // The script it is about, empty for New
        std::string script;

        // The cell the menu was opened for
        int column = -1;
        int row = -1;
    };

    explicit TileMapEditor(TileMap &map);

    // Also reads the digits typed in this frame. font is needed for the size of
    // the menu.
    Request Update(const Input &input, const FontRenderer &font);

    // The scripts the menu offers, e.g. every script of the scene. variants
    // runs parallel to them, e.g. grey for one that is not built in yet, and
    // may be shorter. Turns the scripts part of the menu on.
    void SetScripts(std::vector<std::string> names, std::vector<int> variants);

    // Sets the selected cell and saves. false if nothing is selected, the cell
    // already has this digit or saving fails.
    bool ApplyDigit(int tile);

    // Puts a script on a cell, an empty name takes it off, and saves. false
    // if nothing changed or saving fails.
    bool SetScript(int column, int row, const std::string &script);

    // Follows a renamed or deleted script file on every cell and saves.
    // false if no cell had it or saving fails.
    bool RenameScript(const std::string &from, const std::string &to);

    bool RemoveScript(const std::string &script);

    bool HasSelection() const;

    bool IsMenuOpen() const;

    void CloseMenu();

    // Is the mouse over an item of one of the menus?
    bool IsHovering() const;

    const ContextMenu &TileMenu() const;

    // The digits of the menu items, in their order
    const std::vector<int> &MenuTiles() const;

    // Belongs in the overlay layer: world coordinates, above the characters
    void Draw(const FontRenderer &font) const;

    // Belongs in the screen layer: the menu above everything
    void DrawMenu(const FontRenderer &font) const;

private:
    // Like the add menu, the menu opens slightly offset next to the mouse
    static constexpr float MENU_OFFSET = 5.0f;

    // Same order as the items of the first menu
    enum class Section {
        Tiles,
        Scripts
    };

    // Same order as the menu a right click on a script opens
    enum class ScriptAction {
        Assign,
        Rename,
        Delete
    };

    bool Save() const;

    void OpenMenu(Vector2 position, const Input &input, const FontRenderer &font);

    void OpenScriptMenu(Vector2 position, const Input &input, const FontRenderer &font);

    // The arrow in front of the script on the selected cell
    void MarkAssignedScript();

    void OpenScriptActions(std::size_t script, const Input &input, const FontRenderer &font);

    // Handles a right click, see the class comment
    void HandleRightClick(int column, int row, const Input &input, const FontRenderer &font);

    // What the chosen item of the script actions asks for
    Request ApplyScriptAction(int action);

    // The small corner that marks a cell with a script
    void DrawScriptMarker(int x, int y, int size) const;

    TileMap &map;

    int hoveredColumn = -1;
    int hoveredRow = -1;

    int selectedColumn = -1;
    int selectedRow = -1;

    // Tiles or Scripts, only with scripts
    ContextMenu sectionMenu;

    ContextMenu menu;

    std::vector<int> menuTiles;

    // Only with SetScripts is there a scripts part
    bool hasScripts = false;

    std::vector<std::string> scriptNames;
    std::vector<int> scriptVariants;

    // The scripts and "+New" behind them
    ContextMenu scriptMenu;

    // Assign, Rename and Delete for one script, and which one it is
    ContextMenu scriptActions;

    std::size_t actionScript = 0;
};
