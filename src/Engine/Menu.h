#pragma once

#include <string>
#include <vector>

#include "raylib.h"

#include "ContextMenu.h"
#include "FontRenderer.h"
#include "MenuAction.h"
#include "Theme.h"

// Menu that covers the screen and is operated with the mouse.
//
// Items without a value are centered and clickable themselves. Items with a
// value get two columns: on the left a label that only describes, on the
// right the clickable value. Adjustable values show a - and a + around the
// number there. Items with options are dropdowns: a click on the value opens
// the list of options below it.
class Menu {
public:
    // No item affected
    static constexpr int NOTHING = -1;

    struct Entry {
        std::string label;

        // Empty for items without a value, e.g. "Continue"
        std::string value;

        // Shows - and + around the value
        bool adjustable = false;

        // Own font size for this item, 0 for the menu's
        int scale = 0;

        // Makes the item a dropdown, value is the current option
        std::vector<std::string> options{};
    };

    struct Event {
        int item = NOTHING;
        MenuAction action = MenuAction::None;

        // The picked option for MenuAction::Choose, otherwise NOTHING
        int option = NOTHING;
    };

    // scale enlarges the font of the items by whole numbers, lineGap is the
    // space between two rows in viewport pixels
    Menu(const FontRenderer &font, int scale = 1, int lineGap = 6);

    // Heading above the items, with its own font size
    void SetTitle(std::string title, int titleScale);

    // Takes the overlay and font variants from the theme
    void SetTheme(const Theme &theme);

    // Items without values, e.g. for a pause menu
    void SetItems(const std::vector<std::string> &labels);

    void SetEntries(std::vector<Entry> entries);

    void Open();

    void Close();

    void Toggle();

    bool IsOpen() const;

    // Was the mouse over something clickable during the last Update?
    bool IsHovering() const;

    // Checks the mouse and reports what was clicked. The position is expected in
    // viewport coordinates: the menu lies on the screen layer. While a dropdown
    // is open, the mouse belongs to its list.
    Event Update(Vector2 mousePosition, bool clicked, int viewWidth, int viewHeight);

    void Draw(int viewWidth, int viewHeight) const;

private:
    struct Layout {
        int left;
        int top;
        int width;

        // Top edge of every row. The rows can have different heights.
        std::vector<int> rowTops;
    };

    static bool IsCentered(const Entry &entry);

    static bool IsDropdown(const Entry &entry);

    // Opens the list of a dropdown item below its value
    void OpenDropdown(const Layout &layout, std::size_t index, int viewWidth, int viewHeight);

    int EntryScale(const Entry &entry) const;

    Layout ComputeLayout(int viewWidth, int viewHeight) const;

    // The whole row, across the width of the menu
    Rectangle RowBounds(const Layout &layout, std::size_t index) const;

    // Text of a centered item
    Rectangle LabelBounds(const Layout &layout, std::size_t index) const;

    int ValueWidth(const Entry &entry) const;

    // The right column, for adjustable items from - to +
    Rectangle ValueBounds(const Layout &layout, std::size_t index) const;

    Rectangle DecreaseBounds(const Layout &layout, std::size_t index) const;

    Rectangle IncreaseBounds(const Layout &layout, std::size_t index) const;

    // Which item and which part of it is under the mouse
    Event HitTest(const Layout &layout, Vector2 mousePosition) const;

    int TextWidth(const std::string &text, int textScale) const;

    const FontRenderer &font;

    int scale;
    int lineGap;

    Color overlay{20, 20, 28, 180};

    int idleVariant = FontVariant::White;
    int hoverVariant = FontVariant::Yellow;
    int titleVariant = FontVariant::Cyan;

    std::string title;
    int titleScale = 1;

    std::vector<Entry> entries;

    bool open = false;

    // What is under the mouse right now, for highlighting
    Event hovered;

    // The open options of a dropdown, and the item they belong to
    ContextMenu dropdown;
    int dropdownItem = NOTHING;
};
