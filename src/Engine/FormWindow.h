#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "raylib.h"

#include "ContextMenu.h"
#include "FontRenderer.h"

// Small form in a window, e.g. to create something new in the Development
// mode. It sits centered in the UI layer and blocks everything behind it.
//
// There are text, number, color, choice and dropdown fields as well as
// checkboxes. A click sets the focus, Tab jumps to the next field, in color,
// choice and dropdown fields the arrow keys choose. A click on a dropdown
// opens its list. A click or space checks a box. Enter confirms.
// Typing uses characters instead of keys, which works with every keyboard layout.
class FormWindow {
public:
    enum class Result {
        None,
        Confirmed,
        Cancelled
    };

    void SetTitle(std::string title);

    void SetConfirmLabel(std::string label);

    // A row below the fields, e.g. as an explanation. Without a hint the row is
    // left out.
    void SetHint(std::string hint);

    // The fields are created in the order of the calls. Returns their index.
    std::size_t AddText(std::string label, std::string value, std::size_t maxLength);

    std::size_t AddNumber(std::string label, int value, int minimum, int maximum);

    std::size_t AddColor(std::string label, std::vector<Color> colors, std::size_t selected);

    // One option from a list, changed with the left and right arrows
    std::size_t AddChoice(std::string label, std::vector<std::string> options, std::size_t selected);

    // One option from a list that opens on click, like the add menu in the editor
    std::size_t AddDropdown(std::string label, std::vector<std::string> options, std::size_t selected);

    // A box that can be checked and unchecked
    std::size_t AddToggle(std::string label, bool checked);

    // Removes all fields, title and hint stay
    void Clear();

    void Open(int viewWidth, int viewHeight, const FontRenderer &font);

    void Close();

    bool IsOpen() const;

    // Was the mouse over a field or button during the last Update?
    bool IsHovering() const;

    // Reads mouse and keyboard, the mouse in viewport coordinates. On Confirmed
    // the form stays open: the caller checks the input and closes it itself or
    // sets an error.
    Result Update(Vector2 mousePosition, bool clicked, const FontRenderer &font);

    // The same paths Update uses, also callable directly
    void Focus(std::size_t field);

    // A typed character as Unicode, as GetCharPressed returns it
    void Type(int codepoint);

    void Erase();

    void SelectColor(std::size_t field, std::size_t index);

    void SelectChoice(std::size_t field, std::size_t index);

    void SetChecked(std::size_t field, bool checked);

    // Sets a number field, limited to minimum and maximum
    void SetNumber(std::size_t field, int value);

    const std::string &Text(std::size_t field) const;

    // Empty gives the minimum, values outside are clamped
    int Number(std::size_t field) const;

    Color ColorValue(std::size_t field) const;

    std::size_t ColorIndex(std::size_t field) const;

    // The chosen option of a choice or dropdown field
    std::size_t Choice(std::size_t field) const;

    bool IsChecked(std::size_t field) const;

    std::size_t FieldCount() const;

    // Red in the hint row, without a hint instead of the title. Disappears as
    // soon as something in the form changes.
    void SetError(std::string error);

    const std::string &Error() const;

    Rectangle Bounds(const FontRenderer &font) const;

    Rectangle FieldBounds(std::size_t field, const FontRenderer &font) const;

    Rectangle SwatchBounds(std::size_t field, std::size_t index, const FontRenderer &font) const;

    // Is the list of a dropdown field open?
    bool IsListOpen() const;

    const ContextMenu &List() const;

    // The left or right arrow in a choice field
    Rectangle ChoiceArrowBounds(std::size_t field, bool left, const FontRenderer &font) const;

    Rectangle ConfirmBounds(const FontRenderer &font) const;

    Rectangle CancelBounds(const FontRenderer &font) const;

    void Draw(const FontRenderer &font) const;

private:
    enum class FieldType {
        Text,
        Number,
        Color,
        Choice,
        Dropdown,
        Toggle
    };

    struct Field {
        std::string label;
        FieldType type = FieldType::Text;

        std::string text;
        std::size_t maxLength = 0;

        int minimum = 0;
        int maximum = 0;

        std::vector<Color> colors;
        std::size_t colorIndex = 0;

        std::vector<std::string> options;
        std::size_t choiceIndex = 0;

        bool checked = false;
    };

    float RowHeight(const FontRenderer &font) const;

    // Opens the list below the dropdown field
    void OpenList(std::size_t field, const FontRenderer &font);

    // Row 0 is the title, then the fields, the hint and the buttons
    float RowTop(std::size_t row, const FontRenderer &font) const;

    // The row the buttons are in
    std::size_t ButtonRow() const;

    float LabelWidth(const FontRenderer &font) const;

    float ControlWidth(const Field &field, const FontRenderer &font) const;

    float ButtonWidth(const std::string &label, const FontRenderer &font) const;

    Vector2 Size(const FontRenderer &font) const;

    std::string title;
    std::string confirmLabel = "OK";
    std::string hint;
    std::string error;

    std::vector<Field> fields;

    std::size_t focused = 0;

    bool open = false;

    Vector2 topLeft{0.0f, 0.0f};

    int viewWidth = 0;
    int viewHeight = 0;

    // 0 none, 1 confirm, 2 cancel
    int hoveredButton = 0;

    bool hoveringField = false;

    // The open list and the field it belongs to
    ContextMenu list;
    std::size_t listField = 0;
};
