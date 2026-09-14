#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "raylib.h"

#include "FontRenderer.h"

// Bar with tabs at the top edge of the viewport, plus a "+" directly after the
// last tab, like when opening a new tab.
// Works in viewport coordinates and therefore belongs in the UI layer.
class TabBar {
public:
    // Nothing hit
    static constexpr int NOTHING = -1;

    // The "+" after the last tab
    static constexpr int PLUS = -2;

    void SetTabs(std::vector<std::string> labels);

    std::size_t TabCount() const;

    // The highlighted tab, NOTHING for none
    void SetActive(int index);

    int Active() const;

    // Width all tabs including the "+" may take up at most, e.g. that of the
    // viewport. If they do not fit, the longest labels are shortened and end with
    // three dots. Short ones stay as they are. 0 for unlimited.
    void SetMaxWidth(int width);

    // Was the mouse over a tab or the "+" during the last Update?
    bool IsHovering() const;

    // The label as it is drawn: shortened to the width of its tab
    std::string FittedLabel(std::size_t index, const FontRenderer &font) const;

    float Height(const FontRenderer &font) const;

    // Is the point inside the bar?
    bool Contains(Vector2 point, int viewWidth, const FontRenderer &font) const;

    Rectangle TabBounds(std::size_t index, const FontRenderer &font) const;

    Rectangle PlusBounds(const FontRenderer &font) const;

    // What is under the point: the index of the tab, PLUS or NOTHING
    int TabAt(Vector2 point, const FontRenderer &font) const;

    // Remembers what is under the mouse. On a click: the index of the tab, PLUS
    // or NOTHING.
    int Update(Vector2 mousePosition, bool clicked, const FontRenderer &font);

    void Draw(int viewWidth, const FontRenderer &font) const;

private:
    float NaturalWidth(const std::string &label, const FontRenderer &font) const;

    float PlusWidth(const FontRenderer &font) const;

    // Maximum width of a tab so all of them fit. 0 if all fit.
    float WidthLimit(const FontRenderer &font) const;

    float TabWidth(std::size_t index, const FontRenderer &font) const;

    std::vector<std::string> labels;

    int maxWidth = 0;

    int active = NOTHING;

    int hovered = NOTHING;
};
