#include "UiCanvas.h"

#include <algorithm>
#include <utility>

#include "Renderer.h"

UiItem::UiItem(UiElement element, const FontRenderer &font, float depth)
    : element(std::move(element)),
      depth(depth) {
    int scale = std::max(this->element.scale, 1);

    size = {
        static_cast<float>(font.Measure(this->element.text, TextSpacing::Narrow, scale)),
        static_cast<float>(font.LetterHeight() * scale)
    };
}

const UiElement &UiItem::Element() const {
    return element;
}

Rectangle UiItem::EditBounds() const {
    return {element.position.x, element.position.y, size.x, size.y};
}

Vector2 UiItem::EditPosition() const {
    return element.position;
}

void UiItem::SetEditPosition(Vector2 position) {
    element.position = position;
}

float UiItem::EditDepth() const {
    return depth;
}

void UiCanvas::SetElements(const std::vector<UiElement> &elements, const FontRenderer &font) {
    items.clear();
    items.reserve(elements.size());

    for (std::size_t i = 0; i < elements.size(); i++) {
        items.emplace_back(elements[i], font, static_cast<float>(i));
    }
}

std::vector<UiElement> UiCanvas::Elements() const {
    std::vector<UiElement> elements;
    elements.reserve(items.size());

    for (const UiItem &item: items) {
        elements.push_back(item.Element());
    }

    return elements;
}

std::size_t UiCanvas::Count() const {
    return items.size();
}

const UiItem &UiCanvas::At(std::size_t index) const {
    return items.at(index);
}

UiItem &UiCanvas::At(std::size_t index) {
    return items.at(index);
}

int UiCanvas::IndexOf(const EditableObject *object) const {
    for (std::size_t i = 0; i < items.size(); i++) {
        if (&items[i] == object) {
            return static_cast<int>(i);
        }
    }

    return -1;
}

std::vector<EditableObject *> UiCanvas::Editables() {
    std::vector<EditableObject *> editables;
    editables.reserve(items.size());

    for (UiItem &item: items) {
        editables.push_back(&item);
    }

    return editables;
}

int UiCanvas::ButtonAt(Vector2 point) const {
    // From the back, so the topmost button wins
    for (std::size_t i = items.size(); i > 0; i--) {
        const UiItem &item = items[i - 1];

        if (item.Element().type == UiElement::Type::Button && CheckCollisionPointRec(point, item.EditBounds())) {
            return static_cast<int>(i - 1);
        }
    }

    return -1;
}

void UiCanvas::Draw(const FontRenderer &font,
                    int textVariant,
                    int hoverVariant,
                    int titleVariant,
                    int hoveredButton) const {
    for (std::size_t i = 0; i < items.size(); i++) {
        const UiElement &element = items[i].Element();

        int variant = element.type == UiElement::Type::Button ? textVariant : titleVariant;

        if (element.variant >= 0) {
            variant = element.variant;
        }

        if (element.type == UiElement::Type::Button && static_cast<int>(i) == hoveredButton) {
            variant = hoverVariant;
        }

        font.Draw(
            element.text,
            Renderer::SnapToPixel(element.position),
            variant,
            TextSpacing::Narrow,
            std::max(element.scale, 1)
        );
    }
}
