#include "TabBar.h"

#include <algorithm>
#include <cmath>
#include <utility>

constexpr float TAB_PADDING_X = 5.0f;
constexpr float TAB_PADDING_Y = 2.0f;

constexpr Color TAB_BAR_BACKGROUND{24, 20, 37, 235};
constexpr Color TAB_BAR_BORDER{255, 255, 255, 90};
constexpr Color TAB_ACTIVE_BACKGROUND{255, 255, 255, 45};
constexpr Color TAB_ACTIVE_ACCENT{255, 229, 26, 255};
constexpr Color TAB_SEPARATOR{255, 255, 255, 40};

void TabBar::SetTabs(std::vector<std::string> labels) {
    this->labels = std::move(labels);

    if (active >= static_cast<int>(this->labels.size())) {
        active = NOTHING;
    }

    hovered = NOTHING;
}

std::size_t TabBar::TabCount() const {
    return labels.size();
}

void TabBar::SetActive(int index) {
    active = index >= 0 && index < static_cast<int>(labels.size()) ? index : NOTHING;
}

int TabBar::Active() const {
    return active;
}

void TabBar::SetMaxWidth(int width) {
    maxWidth = width;
}

float TabBar::NaturalWidth(const std::string &label, const FontRenderer &font) const {
    return static_cast<float>(font.Measure(label, TextSpacing::Narrow)) + 2.0f * TAB_PADDING_X;
}

float TabBar::PlusWidth(const FontRenderer &font) const {
    return NaturalWidth("+", font);
}

float TabBar::WidthLimit(const FontRenderer &font) const {
    if (maxWidth <= 0 || labels.empty()) {
        return 0.0f;
    }

    std::vector<float> widths;
    float total = 0.0f;

    for (const std::string &label: labels) {
        widths.push_back(NaturalWidth(label, font));
        total += widths.back();
    }

    float available = static_cast<float>(maxWidth) - PlusWidth(font);

    if (total <= available) {
        return 0.0f;
    }

    // The narrow tabs keep their width, the wide ones share the rest
    std::sort(widths.begin(), widths.end());

    for (std::size_t i = 0; i < widths.size(); i++) {
        float limit = available / static_cast<float>(widths.size() - i);

        if (widths[i] > limit) {
            // Rounded down, so the tabs stand on whole pixels
            return std::max(1.0f, std::floor(limit));
        }

        available -= widths[i];
    }

    return 0.0f;
}

float TabBar::TabWidth(std::size_t index, const FontRenderer &font) const {
    float natural = NaturalWidth(labels.at(index), font);
    float limit = WidthLimit(font);

    return limit > 0.0f ? std::min(natural, limit) : natural;
}

bool TabBar::IsHovering() const {
    return hovered != NOTHING;
}

std::string TabBar::FittedLabel(std::size_t index, const FontRenderer &font) const {
    std::string label = labels.at(index);

    float available = TabWidth(index, font) - 2.0f * TAB_PADDING_X;

    if (static_cast<float>(font.Measure(label, TextSpacing::Narrow)) <= available) {
        return label;
    }

    const std::string ellipsis(1, FontRenderer::ICON_ELLIPSIS);

    while (!label.empty() && static_cast<float>(font.Measure(label + ellipsis, TextSpacing::Narrow)) > available) {
        FontRenderer::RemoveLastGlyph(label);
    }

    // No space directly before the dots
    while (!label.empty() && label.back() == ' ') {
        label.pop_back();
    }

    return label + ellipsis;
}

float TabBar::Height(const FontRenderer &font) const {
    return static_cast<float>(font.LetterHeight()) + 2.0f * TAB_PADDING_Y;
}

bool TabBar::Contains(Vector2 point, int viewWidth, const FontRenderer &font) const {
    return point.x >= 0.0f && point.x < static_cast<float>(viewWidth) &&
           point.y >= 0.0f && point.y < Height(font);
}

Rectangle TabBar::TabBounds(std::size_t index, const FontRenderer &font) const {
    float x = 0.0f;

    for (std::size_t i = 0; i < index && i < labels.size(); i++) {
        x += TabWidth(i, font);
    }

    return {x, 0.0f, TabWidth(index, font), Height(font)};
}

Rectangle TabBar::PlusBounds(const FontRenderer &font) const {
    float left = 0.0f;

    if (!labels.empty()) {
        Rectangle last = TabBounds(labels.size() - 1, font);

        left = last.x + last.width;
    }

    return {left, 0.0f, PlusWidth(font), Height(font)};
}

int TabBar::TabAt(Vector2 point, const FontRenderer &font) const {
    if (CheckCollisionPointRec(point, PlusBounds(font))) {
        return PLUS;
    }

    for (std::size_t i = 0; i < labels.size(); i++) {
        if (CheckCollisionPointRec(point, TabBounds(i, font))) {
            return static_cast<int>(i);
        }
    }

    return NOTHING;
}

int TabBar::Update(Vector2 mousePosition, bool clicked, const FontRenderer &font) {
    hovered = TabAt(mousePosition, font);

    return clicked ? hovered : NOTHING;
}

void TabBar::Draw(int viewWidth, const FontRenderer &font) const {
    int height = static_cast<int>(Height(font));

    DrawRectangle(0, 0, viewWidth, height, TAB_BAR_BACKGROUND);
    DrawRectangle(0, height, viewWidth, 1, TAB_BAR_BORDER);

    for (std::size_t i = 0; i < labels.size(); i++) {
        Rectangle tab = TabBounds(i, font);

        bool isActive = static_cast<int>(i) == active;
        bool isHovered = static_cast<int>(i) == hovered;

        if (isActive) {
            DrawRectangleRec(tab, TAB_ACTIVE_BACKGROUND);
            DrawRectangle(static_cast<int>(tab.x), height - 1, static_cast<int>(tab.width), 1, TAB_ACTIVE_ACCENT);
        }

        int variant = FontVariant::Grey;

        if (isHovered) {
            variant = FontVariant::Yellow;
        } else if (isActive) {
            variant = FontVariant::White;
        }

        font.Draw(FittedLabel(i, font), {tab.x + TAB_PADDING_X, tab.y + TAB_PADDING_Y}, variant, TextSpacing::Narrow);

        DrawRectangle(static_cast<int>(tab.x + tab.width), 0, 1, height, TAB_SEPARATOR);
    }

    Rectangle plus = PlusBounds(font);

    DrawRectangle(static_cast<int>(plus.x + plus.width), 0, 1, height, TAB_SEPARATOR);

    font.Draw(
        "+",
        {plus.x + TAB_PADDING_X, plus.y + TAB_PADDING_Y},
        hovered == PLUS ? FontVariant::Yellow : FontVariant::Grey,
        TextSpacing::Narrow
    );
}
