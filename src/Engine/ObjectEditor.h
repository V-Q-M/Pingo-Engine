#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include "raylib.h"

#include "ContextMenu.h"
#include "CursorState.h"
#include "FontRenderer.h"
#include "EditableObject.h"

// Developer tool for placing objects, e.g. characters in the world or texts
// in the main menu. The scene passes the mouse coordinates in the space its
// objects live in, and draws the editor in the matching layer.
//
// A click selects a character, its coordinates and four arrows appear next
// to it. It can be dragged with the mouse held down, the arrow keys move it
// by one pixel, with Shift by ten. Holding a key repeats the step.
//
// A right click into empty space opens the add menu, which creates a new
// character at this position. A right click on a type in it opens that
// type's menu. A right click on a character opens a menu with its actions.
// The scene defines the items of these menus.
//
// The editor does not know what a character consists of or where it is
// saved. It only reports changes and requests, the scene carries them out.
// Delete or Backspace report deleting the selection, Z and Y go back and
// forth in the history.
class ObjectEditor {
public:
    struct Input {
        Vector2 mouseWorld{0.0f, 0.0f};

        // The same mouse in viewport coordinates, for the menus
        Vector2 mouseViewport{0.0f, 0.0f};

        // Just pressed or held, for dragging
        bool leftClicked = false;
        bool leftDown = false;

        bool rightClicked = false;

        int viewWidth = 0;
        int viewHeight = 0;
    };

    struct Changes {
        // The selection was moved
        bool moved = false;

        // The selection should be deleted
        bool deleteRequested = false;

        // Item from the add menu, -1 for none. Equal to the number of types means:
        // the extra item, see SetAddExtra.
        int addType = -1;

        Vector2 addPosition{0.0f, 0.0f};

        // Z: one step back, Y: one step forward
        bool undoRequested = false;
        bool redoRequested = false;

        // Chosen item from the character menu, -1 for none, and the character that
        // was right clicked
        int objectAction = -1;
        EditableObject *actionTarget = nullptr;

        // Chosen item from the menu of a type, -1 for none, and the type
        int typeAction = -1;
        std::size_t typeIndex = 0;
    };

    ObjectEditor();

    // instances are the current characters. font is needed for the size of the
    // menus.
    Changes Update(const std::vector<EditableObject *> &instances,
                   const Input &input,
                   const FontRenderer &font);

    // Whoever removes or adds a character must clear the selection first: the
    // editor only holds a pointer, and the list may move in the process
    void Select(EditableObject *instance);

    EditableObject *Selected() const;

    // Is the selection currently being dragged with the mouse?
    bool IsDragging() const;

    // Moves the selection and rounds to whole pixels. false without a selection.
    bool MoveSelected(Vector2 delta);

    // Names of the character types for the add menu, in the order of addType
    void SetTypeNames(std::vector<std::string> names);

    // Additional last item in the add menu, e.g. to create a new type, in its
    // own font variant. It has no type menu.
    void SetAddExtra(std::string label, int variant);

    // Items for the menu a right click on a type in the add menu opens. The
    // scene decides what they do based on Changes::typeAction.
    // icons runs parallel to labels, 0 for no icon.
    void SetTypeActions(std::vector<std::string> labels, const std::vector<char> &icons = {});

    // Items for the menu a right click on a character opens. The scene decides
    // what they do based on Changes::objectAction.
    void SetObjectActions(std::vector<std::string> labels, const std::vector<char> &icons = {});

    // Returns the heading of the character menu, e.g. the character's name
    void SetObjectTitle(std::function<std::string(const EditableObject &)> title);

    // May this object have a right click menu? Without a filter: every one.
    // Objects without a menu only get selected by a right click.
    void SetObjectMenuFilter(std::function<bool(const EditableObject &)> filter);

    // Space between the object and the top arrow, e.g. for a health bar
    void SetTopArrowClearance(float clearance);

    // The topmost object under a point, otherwise nullptr
    static EditableObject *FindAt(Vector2 point, const std::vector<EditableObject *> &objects);

    bool IsMenuOpen() const;

    // Matching mouse cursor, see Engine::RequestCursor: Grab while dragging,
    // Hover over an object or a menu item, otherwise Idle
    CursorState Cursor(const std::vector<EditableObject *> &instances, Vector2 mouseWorld) const;

    void CloseMenu();

    const ContextMenu &AddMenu() const;

    const ContextMenu &ObjectMenu() const;

    const ContextMenu &TypeMenu() const;

    // Is an arrow key pressed right now?
    static bool IsMoveKeyDown();

    // Overlay layer: arrows and coordinates at the selection, caption below.
    // With the add menu open, a cross where the new character is created.
    void Draw(const FontRenderer &font, const std::string &caption) const;

    // Screen layer: key help at the bottom left, extra as its own row above it
    void DrawHelp(const FontRenderer &font, int viewHeight, const std::string &extra) const;

    // Screen layer: the context menus, they belong above everything else
    void DrawMenu(const FontRenderer &font) const;

private:
    void HandleRightClick(const std::vector<EditableObject *> &instances,
                          const Input &input,
                          const FontRenderer &font);

    void RebuildAddMenu();

    EditableObject *selected = nullptr;

    bool dragging = false;

    // Distance from the mouse to the character's position when grabbing. That
    // way the character does not jump to the mouse with its feet.
    Vector2 dragOffset{0.0f, 0.0f};

    ContextMenu addMenu;

    std::vector<std::string> typeNames;

    std::string addExtraLabel;
    int addExtraVariant = -1;

    // Where the character from the add menu is created, in world coordinates
    Vector2 addPosition{0.0f, 0.0f};

    ContextMenu typeMenu;

    std::size_t typeMenuIndex = 0;

    ContextMenu objectMenu;

    // The character the character menu is open for. As long as a menu is open,
    // nothing changes about the characters, so the pointer stays valid.
    EditableObject *menuTarget = nullptr;

    std::function<std::string(const EditableObject &)> objectTitle;

    std::function<bool(const EditableObject &)> objectMenuFilter;

    float topArrowClearance = 8.0f;
};
