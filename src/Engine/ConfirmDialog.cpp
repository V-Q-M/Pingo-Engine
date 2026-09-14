#include "ConfirmDialog.h"

#include <algorithm>
#include <utility>

#include "Renderer.h"

constexpr float DIALOG_PADDING = 8.0f;
constexpr float DIALOG_ROW_GAP = 5.0f;
constexpr float DIALOG_BOX_PADDING = 2.0f;
constexpr float DIALOG_BUTTON_PADDING = 6.0f;
constexpr float DIALOG_BUTTON_GAP = 16.0f;
constexpr float DIALOG_MIN_WIDTH = 100.0f;

constexpr const char *DIALOG_YES = "Yes";
constexpr const char *DIALOG_NO = "No";

constexpr Color DIALOG_SHADE{0, 0, 0, 110};
constexpr Color DIALOG_BACKGROUND{24, 20, 37, 245};
constexpr Color DIALOG_BORDER{255, 255, 255, 140};
constexpr Color DIALOG_HOVER{255, 229, 26, 255};

constexpr int DIALOG_VARIANT_TEXT = FontVariant::White;
constexpr int DIALOG_VARIANT_HOVER = FontVariant::Yellow;

void ConfirmDialog::Open(std::string title,
                         std::string message,
                         int viewWidth,
                         int viewHeight,
                         const FontRenderer &font,
                         int titleVariant) {
    this->title = std::move(title);
    this->titleVariant = titleVariant;
    lines.clear();

    std::size_t start = 0;

    for (std::size_t end = message.find('\n'); end != std::string::npos; end = message.find('\n', start)) {
        lines.push_back(message.substr(start, end - start));
        start = end + 1;
    }

    lines.push_back(message.substr(start));
    this->viewWidth = viewWidth;
    this->viewHeight = viewHeight;

    Vector2 size = Size(font);

    topLeft = Renderer::SnapToPixel({(viewWidth - size.x) / 2.0f, (viewHeight - size.y) / 2.0f});

    open = true;
    hovered = 0;
}

void ConfirmDialog::Close() {
    open = false;
    hovered = 0;
}

bool ConfirmDialog::IsOpen() const {
    return open;
}

bool ConfirmDialog::IsHovering() const {
    return open && hovered != 0;
}

float ConfirmDialog::RowHeight(const FontRenderer &font) const {
    return static_cast<float>(font.LetterHeight()) + 2.0f * DIALOG_BOX_PADDING;
}

float ConfirmDialog::ButtonWidth(const std::string &label, const FontRenderer &font) const {
    return static_cast<float>(font.Measure(label, TextSpacing::Narrow)) + 2.0f * DIALOG_BUTTON_PADDING;
}

Vector2 ConfirmDialog::Size(const FontRenderer &font) const {
    float width = std::max({
        DIALOG_MIN_WIDTH,
        static_cast<float>(font.Measure(title, TextSpacing::Narrow)),
        ButtonWidth(DIALOG_YES, font) + DIALOG_BUTTON_GAP + ButtonWidth(DIALOG_NO, font)
    });

    for (const std::string &line: lines) {
        width = std::max(width, static_cast<float>(font.Measure(line, TextSpacing::Narrow)));
    }

    // Title, the lines of the question, buttons
    float rows = 2.0f + static_cast<float>(lines.size());

    return {
        width + 2.0f * DIALOG_PADDING,
        2.0f * DIALOG_PADDING + rows * RowHeight(font) + (rows - 1.0f) * DIALOG_ROW_GAP
    };
}

Rectangle ConfirmDialog::Bounds(const FontRenderer &font) const {
    Vector2 size = Size(font);

    return {topLeft.x, topLeft.y, size.x, size.y};
}

// The buttons sit together, centered in the last row
Rectangle ConfirmDialog::YesBounds(const FontRenderer &font) const {
    float buttons = ButtonWidth(DIALOG_YES, font) + DIALOG_BUTTON_GAP + ButtonWidth(DIALOG_NO, font);
    Vector2 size = Size(font);

    return {
        Renderer::SnapToPixel({topLeft.x + (size.x - buttons) / 2.0f, 0.0f}).x,
        topLeft.y + DIALOG_PADDING + (1.0f + static_cast<float>(lines.size())) * (RowHeight(font) + DIALOG_ROW_GAP),
        ButtonWidth(DIALOG_YES, font),
        RowHeight(font)
    };
}

Rectangle ConfirmDialog::NoBounds(const FontRenderer &font) const {
    Rectangle yes = YesBounds(font);

    return {yes.x + yes.width + DIALOG_BUTTON_GAP, yes.y, ButtonWidth(DIALOG_NO, font), RowHeight(font)};
}

ConfirmDialog::Result ConfirmDialog::Update(Vector2 mousePosition, bool clicked, const FontRenderer &font) {
    if (!open) {
        return Result::None;
    }

    hovered = 0;

    if (CheckCollisionPointRec(mousePosition, YesBounds(font))) {
        hovered = 1;
    } else if (CheckCollisionPointRec(mousePosition, NoBounds(font))) {
        hovered = 2;
    }

    if (!clicked || hovered == 0) {
        return Result::None;
    }

    Result result = hovered == 1 ? Result::Yes : Result::No;

    Close();

    return result;
}

void ConfirmDialog::Draw(const FontRenderer &font) const {
    if (!open) {
        return;
    }

    DrawRectangle(0, 0, viewWidth, viewHeight, DIALOG_SHADE);

    Rectangle bounds = Bounds(font);

    DrawRectangleRec(bounds, DIALOG_BACKGROUND);
    DrawRectangleLinesEx(bounds, 1.0f, DIALOG_BORDER);

    auto centered = [&](const std::string &text, float top, int variant) {
        float width = static_cast<float>(font.Measure(text, TextSpacing::Narrow));

        font.Draw(
            text,
            Renderer::SnapToPixel({bounds.x + (bounds.width - width) / 2.0f, top + DIALOG_BOX_PADDING}),
            variant,
            TextSpacing::Narrow
        );
    };

    centered(title, topLeft.y + DIALOG_PADDING, titleVariant);
    for (std::size_t i = 0; i < lines.size(); i++) {
        float top = topLeft.y + DIALOG_PADDING + static_cast<float>(i + 1) * (RowHeight(font) + DIALOG_ROW_GAP);

        centered(lines[i], top, DIALOG_VARIANT_TEXT);
    }

    auto button = [&](Rectangle rect, const char *label, bool isHovered) {
        DrawRectangleLinesEx(rect, 1.0f, isHovered ? DIALOG_HOVER : DIALOG_BORDER);

        font.Draw(
            label,
            {rect.x + DIALOG_BUTTON_PADDING, rect.y + DIALOG_BOX_PADDING},
            isHovered ? DIALOG_VARIANT_HOVER : DIALOG_VARIANT_TEXT,
            TextSpacing::Narrow
        );
    };

    button(YesBounds(font), DIALOG_YES, hovered == 1);
    button(NoBounds(font), DIALOG_NO, hovered == 2);
}
