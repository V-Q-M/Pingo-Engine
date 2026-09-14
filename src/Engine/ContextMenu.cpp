#include "ContextMenu.h"

#include <algorithm>
#include <utility>

#include "Renderer.h"

constexpr float CONTEXT_PADDING = 4.0f;
constexpr float CONTEXT_LINE_GAP = 3.0f;

constexpr Color CONTEXT_BACKGROUND{24, 20, 37, 235};
constexpr Color CONTEXT_BORDER{255, 255, 255, 140};
constexpr Color CONTEXT_HOVER_BACKGROUND{255, 255, 255, 40};

constexpr int CONTEXT_VARIANT_TEXT = FontVariant::White;
constexpr int CONTEXT_VARIANT_HOVER = FontVariant::Yellow;
constexpr int CONTEXT_VARIANT_TITLE = FontVariant::Cyan;
constexpr int CONTEXT_VARIANT_DISABLED = FontVariant::Grey;

void ContextMenu::SetTitle(std::string title) {
    this->title = std::move(title);
}

void ContextMenu::SetItems(std::vector<std::string> items) {
    this->items = std::move(items);

    variants.assign(this->items.size(), -1);
    icons.assign(this->items.size(), 0);
    enabled.assign(this->items.size(), true);

    if (hovered >= static_cast<int>(this->items.size())) {
        hovered = NOTHING;
    }
}

void ContextMenu::SetItemVariant(std::size_t index, int variant) {
    if (index < variants.size()) {
        variants[index] = variant;
    }
}

void ContextMenu::SetItemIcon(std::size_t index, char icon) {
    if (index < icons.size()) {
        icons[index] = icon;
    }
}

void ContextMenu::SetItemEnabled(std::size_t index, bool enabled) {
    if (index < this->enabled.size()) {
        this->enabled[index] = enabled;
    }
}

bool ContextMenu::IsItemEnabled(std::size_t index) const {
    return index < enabled.size() && enabled[index];
}

bool ContextMenu::IsHovering() const {
    return open && hovered != NOTHING;
}

bool ContextMenu::HasItems() const {
    return !items.empty();
}

std::size_t ContextMenu::ItemCount() const {
    return items.size();
}

const std::string &ContextMenu::Item(std::size_t index) const {
    return items.at(index);
}

int ContextMenu::ItemAt(Vector2 mousePosition, const FontRenderer &font) const {
    if (!open) {
        return NOTHING;
    }

    for (std::size_t i = 0; i < items.size(); i++) {
        if (enabled[i] && CheckCollisionPointRec(mousePosition, ItemBounds(i, font))) {
            return static_cast<int>(i);
        }
    }

    return NOTHING;
}

float ContextMenu::LineHeight(const FontRenderer &font) const {
    return static_cast<float>(font.LetterHeight()) + CONTEXT_LINE_GAP;
}

float ContextMenu::IconColumn(const FontRenderer &font) const {
    for (char icon: icons) {
        if (icon != 0) {
            // Icons bring their own spacing to the text
            return static_cast<float>(font.Measure(std::string(1, icon), TextSpacing::Narrow));
        }
    }

    return 0.0f;
}

Vector2 ContextMenu::Size(const FontRenderer &font) const {
    float width = static_cast<float>(font.Measure(title, TextSpacing::Narrow));

    for (const std::string &item: items) {
        width = std::max(width, IconColumn(font) + static_cast<float>(font.Measure(item, TextSpacing::Narrow)));
    }

    std::size_t lines = items.size() + (title.empty() ? 0 : 1);

    return {
        width + 2.0f * CONTEXT_PADDING,
        static_cast<float>(lines) * LineHeight(font) - CONTEXT_LINE_GAP + 2.0f * CONTEXT_PADDING
    };
}

void ContextMenu::Open(Vector2 position, int viewWidth, int viewHeight, const FontRenderer &font) {
    this->viewWidth = viewWidth;
    this->viewHeight = viewHeight;

    topLeft = position;

    KeepInside(font);

    open = true;
    hovered = NOTHING;
}

void ContextMenu::KeepInside(const FontRenderer &font) {
    Vector2 size = Size(font);

    // If the window is larger than the viewport, the top left corner wins
    float maxX = std::max(1.0f, viewWidth - size.x - 1.0f);
    float maxY = std::max(1.0f, viewHeight - size.y - 1.0f);

    topLeft = Renderer::SnapToPixel({
        std::clamp(topLeft.x, 1.0f, maxX),
        std::clamp(topLeft.y, 1.0f, maxY)
    });
}

void ContextMenu::Close() {
    open = false;
    hovered = NOTHING;
}

bool ContextMenu::IsOpen() const {
    return open;
}

Rectangle ContextMenu::Bounds(const FontRenderer &font) const {
    Vector2 size = Size(font);

    return {topLeft.x, topLeft.y, size.x, size.y};
}

Rectangle ContextMenu::ItemBounds(std::size_t index, const FontRenderer &font) const {
    std::size_t row = index + (title.empty() ? 0 : 1);

    float top = topLeft.y + CONTEXT_PADDING + static_cast<float>(row) * LineHeight(font);

    return {
        topLeft.x,
        top - CONTEXT_LINE_GAP / 2.0f,
        Size(font).x,
        LineHeight(font)
    };
}

int ContextMenu::Update(Vector2 mousePosition, bool clicked, const FontRenderer &font) {
    if (!open) {
        return NOTHING;
    }

    // The items may have changed while the menu was open
    KeepInside(font);

    hovered = ItemAt(mousePosition, font);

    if (!clicked) {
        return NOTHING;
    }

    if (hovered != NOTHING) {
        int chosen = hovered;
        Close();
        return chosen;
    }

    // A click on the title or border keeps the window open, one next to it does not
    if (!CheckCollisionPointRec(mousePosition, Bounds(font))) {
        Close();
    }

    return NOTHING;
}

void ContextMenu::Draw(const FontRenderer &font) const {
    if (!open) {
        return;
    }

    Rectangle bounds = Bounds(font);

    DrawRectangleRec(bounds, CONTEXT_BACKGROUND);
    DrawRectangleLinesEx(bounds, 1.0f, CONTEXT_BORDER);

    float textLeft = topLeft.x + CONTEXT_PADDING;

    if (!title.empty()) {
        font.Draw(title, {textLeft, topLeft.y + CONTEXT_PADDING}, CONTEXT_VARIANT_TITLE, TextSpacing::Narrow);
    }

    for (std::size_t i = 0; i < items.size(); i++) {
        Rectangle row = ItemBounds(i, font);

        bool isHovered = static_cast<int>(i) == hovered;

        if (isHovered) {
            DrawRectangleRec(row, CONTEXT_HOVER_BACKGROUND);
        }

        int variant = variants[i] >= 0 ? variants[i] : CONTEXT_VARIANT_TEXT;

        if (!enabled[i]) {
            variant = CONTEXT_VARIANT_DISABLED;
        } else if (isHovered) {
            variant = CONTEXT_VARIANT_HOVER;
        }

        float top = row.y + CONTEXT_LINE_GAP / 2.0f;

        if (icons[i] != 0) {
            font.Draw(std::string(1, icons[i]), {textLeft, top}, variant);
        }

        font.Draw(items[i], {textLeft + IconColumn(font), top}, variant, TextSpacing::Narrow);
    }
}
