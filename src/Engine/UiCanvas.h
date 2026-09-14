#pragma once

#include <cstddef>
#include <vector>

#include "raylib.h"

#include "EditableObject.h"
#include "FontRenderer.h"
#include "UiLayout.h"

// An element of the interface at runtime: its data and its size, which
// depends on the font
class UiItem : public EditableObject {
public:
    UiItem(UiElement element, const FontRenderer &font, float depth);

    const UiElement &Element() const;

    Rectangle EditBounds() const override;

    Vector2 EditPosition() const override;

    void SetEditPosition(Vector2 position) override;

    float EditDepth() const override;

private:
    UiElement element;

    Vector2 size{0.0f, 0.0f};

    float depth = 0.0f;
};

// Texts and buttons of a screen interface, freely positioned
class UiCanvas {
public:
    // Rebuilds the elements. Pointers to the old ones become invalid.
    // Later elements lie above earlier ones.
    void SetElements(const std::vector<UiElement> &elements, const FontRenderer &font);

    std::vector<UiElement> Elements() const;

    std::size_t Count() const;

    const UiItem &At(std::size_t index) const;

    UiItem &At(std::size_t index);

    // Index of the element, -1 if it does not belong to this interface
    int IndexOf(const EditableObject *object) const;

    // All elements for the ObjectEditor
    std::vector<EditableObject *> Editables();

    // The topmost button under the point, otherwise -1
    int ButtonAt(Vector2 point) const;

    // Without their own color, texts use titleVariant and buttons textVariant.
    // The button under the mouse uses hoverVariant, hoveredButton -1 for none.
    void Draw(const FontRenderer &font, int textVariant, int hoverVariant, int titleVariant, int hoveredButton) const;

private:
    std::vector<UiItem> items;
};
