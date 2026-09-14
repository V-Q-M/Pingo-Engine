#pragma once

#include <string>
#include <vector>

#include "raylib.h"

#include "FontRenderer.h"

// Confirmation in the middle of the screen, e.g. "Are you sure" before deleting.
//
// Lives in the UI layer, dims everything behind it and blocks it until Yes
// or No is clicked. A click next to it does nothing.
class ConfirmDialog {
public:
    enum class Result {
        None,
        Yes,
        No
    };

    // title is shown at the top, e.g. what is being deleted, message below. A \n in
    // message starts a new line. titleVariant is the font color of the
    // title, e.g. FontVariant::Red before deleting.
    void Open(std::string title,
              std::string message,
              int viewWidth,
              int viewHeight,
              const FontRenderer &font,
              int titleVariant = FontVariant::Cyan);

    void Close();

    bool IsOpen() const;

    // Was the mouse over Yes or No during the last Update?
    bool IsHovering() const;

    // The mouse in viewport coordinates. On Yes and No the dialog closes.
    Result Update(Vector2 mousePosition, bool clicked, const FontRenderer &font);

    Rectangle Bounds(const FontRenderer &font) const;

    Rectangle YesBounds(const FontRenderer &font) const;

    Rectangle NoBounds(const FontRenderer &font) const;

    void Draw(const FontRenderer &font) const;

private:
    float RowHeight(const FontRenderer &font) const;

    float ButtonWidth(const std::string &label, const FontRenderer &font) const;

    Vector2 Size(const FontRenderer &font) const;

    std::string title;

    // message, split at the line breaks
    std::vector<std::string> lines;

    int titleVariant = FontVariant::Cyan;

    bool open = false;

    Vector2 topLeft{0.0f, 0.0f};

    int viewWidth = 0;
    int viewHeight = 0;

    // 0 none, 1 Yes, 2 No
    int hovered = 0;
};
