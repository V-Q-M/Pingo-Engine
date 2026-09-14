#include "Menu.h"

#include <algorithm>

constexpr TextSpacing MENU_SPACING = TextSpacing::Narrow;

// Space between label and value, in character widths
constexpr int MENU_COLUMN_GAP_LETTERS = 4;

// Fixed width of the number between - and +. That way the buttons stay in
// their place when 100 becomes 90, and you keep hitting them when clicking
// quickly several times.
constexpr int MENU_VALUE_DIGITS = 3;

// Pixels between - or + and the number field
constexpr int MENU_BUTTON_GAP = 2;

// The buttons are only one character wide. A few pixels of space around
// them make them easier to hit.
constexpr int MENU_HIT_PADDING = 2;

static Rectangle Expand(Rectangle rect, float dx, float dy) {
    return {
        rect.x - dx,
        rect.y - dy,
        rect.width + 2.0f * dx,
        rect.height + 2.0f * dy
    };
}

Menu::Menu(const FontRenderer &font, int scale, int lineGap)
    : font(font),
      scale(std::max(scale, 1)),
      lineGap(lineGap) {
}

void Menu::SetTitle(std::string title, int titleScale) {
    this->title = std::move(title);
    this->titleScale = std::max(titleScale, 1);
}

void Menu::SetTheme(const Theme &theme) {
    overlay = theme.menuOverlay;
    idleVariant = theme.textVariant;
    hoverVariant = theme.hoverVariant;
    titleVariant = theme.titleVariant;
}

void Menu::SetItems(const std::vector<std::string> &labels) {
    entries.clear();

    for (const std::string &label: labels) {
        entries.push_back({label, ""});
    }
}

void Menu::SetEntries(std::vector<Entry> entries) {
    this->entries = std::move(entries);
}

void Menu::Open() {
    open = true;
    hovered = {};
}

void Menu::Close() {
    open = false;
    hovered = {};
}

void Menu::Toggle() {
    if (open) {
        Close();
    } else {
        Open();
    }
}

bool Menu::IsOpen() const {
    return open;
}

bool Menu::IsHovering() const {
    return open && hovered.item != NOTHING;
}

bool Menu::IsCentered(const Entry &entry) {
    return entry.value.empty() && !entry.adjustable;
}

int Menu::EntryScale(const Entry &entry) const {
    return entry.scale > 0 ? entry.scale : scale;
}

int Menu::TextWidth(const std::string &text, int textScale) const {
    return font.Measure(text, MENU_SPACING, textScale);
}

int Menu::ValueWidth(const Entry &entry) const {
    int entryScale = EntryScale(entry);

    if (!entry.adjustable) {
        return TextWidth(entry.value, entryScale);
    }

    int advance = font.Advance(MENU_SPACING) * entryScale;
    int gap = MENU_BUTTON_GAP * entryScale;

    return advance + gap + MENU_VALUE_DIGITS * advance + gap + advance;
}

Menu::Layout Menu::ComputeLayout(int viewWidth, int viewHeight) const {
    int labelWidth = 0;
    int valueWidth = 0;
    int columnGap = 0;
    int centeredWidth = TextWidth(title, titleScale);

    for (const Entry &entry: entries) {
        int entryScale = EntryScale(entry);

        if (IsCentered(entry)) {
            centeredWidth = std::max(centeredWidth, TextWidth(entry.label, entryScale));
            continue;
        }

        labelWidth = std::max(labelWidth, TextWidth(entry.label, entryScale));
        valueWidth = std::max(valueWidth, ValueWidth(entry));
        columnGap = std::max(columnGap, MENU_COLUMN_GAP_LETTERS * font.Advance(MENU_SPACING) * entryScale);
    }

    int columnsWidth = valueWidth > 0 ? labelWidth + columnGap + valueWidth : 0;

    // The heading gets double spacing below it
    int titleHeight = title.empty()
                          ? 0
                          : font.LetterHeight() * titleScale + lineGap * 2;

    int rowsHeight = 0;

    for (std::size_t i = 0; i < entries.size(); i++) {
        if (i > 0) {
            rowsHeight += lineGap;
        }

        rowsHeight += font.LetterHeight() * EntryScale(entries[i]);
    }

    Layout layout{};
    layout.width = std::max(columnsWidth, centeredWidth);
    layout.left = (viewWidth - layout.width) / 2;
    layout.top = (viewHeight - titleHeight - rowsHeight) / 2;

    int y = layout.top + titleHeight;

    for (const Entry &entry: entries) {
        layout.rowTops.push_back(y);

        y += font.LetterHeight() * EntryScale(entry) + lineGap;
    }

    return layout;
}

Rectangle Menu::RowBounds(const Layout &layout, std::size_t index) const {
    return {
        static_cast<float>(layout.left),
        static_cast<float>(layout.rowTops[index]),
        static_cast<float>(layout.width),
        static_cast<float>(font.LetterHeight() * EntryScale(entries[index]))
    };
}

Rectangle Menu::LabelBounds(const Layout &layout, std::size_t index) const {
    Rectangle row = RowBounds(layout, index);

    int width = TextWidth(entries[index].label, EntryScale(entries[index]));

    row.x = static_cast<float>(layout.left + (layout.width - width) / 2);
    row.width = static_cast<float>(width);

    return row;
}

Rectangle Menu::ValueBounds(const Layout &layout, std::size_t index) const {
    Rectangle row = RowBounds(layout, index);

    int width = ValueWidth(entries[index]);

    row.x = static_cast<float>(layout.left + layout.width - width);
    row.width = static_cast<float>(width);

    return row;
}

Rectangle Menu::DecreaseBounds(const Layout &layout, std::size_t index) const {
    Rectangle value = ValueBounds(layout, index);

    value.width = static_cast<float>(font.Advance(MENU_SPACING) * EntryScale(entries[index]));

    return value;
}

Rectangle Menu::IncreaseBounds(const Layout &layout, std::size_t index) const {
    Rectangle value = ValueBounds(layout, index);

    float advance = static_cast<float>(font.Advance(MENU_SPACING) * EntryScale(entries[index]));

    value.x += value.width - advance;
    value.width = advance;

    return value;
}

Menu::Event Menu::HitTest(const Layout &layout, Vector2 mousePosition) const {
    // Up and down only by half the line gap, so the areas of neighboring rows
    // do not overlap
    float padY = lineGap / 2.0f;

    for (std::size_t i = 0; i < entries.size(); i++) {
        const Entry &entry = entries[i];
        int item = static_cast<int>(i);

        float padX = static_cast<float>(MENU_HIT_PADDING * EntryScale(entry));

        if (IsCentered(entry)) {
            if (CheckCollisionPointRec(mousePosition, LabelBounds(layout, i))) {
                return {item, MenuAction::Activate};
            }

            continue;
        }

        // The label on the left only describes, the right column is clickable
        if (entry.adjustable) {
            if (CheckCollisionPointRec(mousePosition, Expand(DecreaseBounds(layout, i), padX, padY))) {
                return {item, MenuAction::Decrease};
            }

            if (CheckCollisionPointRec(mousePosition, Expand(IncreaseBounds(layout, i), padX, padY))) {
                return {item, MenuAction::Increase};
            }

            continue;
        }

        if (CheckCollisionPointRec(mousePosition, Expand(ValueBounds(layout, i), padX, padY))) {
            return {item, MenuAction::Activate};
        }
    }

    return {};
}

Menu::Event Menu::Update(Vector2 mousePosition, bool clicked, int viewWidth, int viewHeight) {
    if (!open) {
        return {};
    }

    hovered = HitTest(ComputeLayout(viewWidth, viewHeight), mousePosition);

    if (!clicked) {
        return {};
    }

    return hovered;
}

void Menu::Draw(int viewWidth, int viewHeight) const {
    if (!open) {
        return;
    }

    DrawRectangle(0, 0, viewWidth, viewHeight, overlay);

    Layout layout = ComputeLayout(viewWidth, viewHeight);

    if (!title.empty()) {
        int x = layout.left + (layout.width - TextWidth(title, titleScale)) / 2;

        font.Draw(
            title,
            {static_cast<float>(x), static_cast<float>(layout.top)},
            titleVariant,
            MENU_SPACING,
            titleScale
        );
    }

    for (std::size_t i = 0; i < entries.size(); i++) {
        const Entry &entry = entries[i];

        int entryScale = EntryScale(entry);
        bool entryHovered = hovered.item == static_cast<int>(i);

        if (IsCentered(entry)) {
            Rectangle label = LabelBounds(layout, i);

            font.Draw(
                entry.label,
                {label.x, label.y},
                entryHovered ? hoverVariant : idleVariant,
                MENU_SPACING,
                entryScale
            );

            continue;
        }

        Rectangle row = RowBounds(layout, i);

        font.Draw(entry.label, {row.x, row.y}, idleVariant, MENU_SPACING, entryScale);

        if (!entry.adjustable) {
            Rectangle value = ValueBounds(layout, i);

            font.Draw(
                entry.value,
                {value.x, value.y},
                entryHovered ? hoverVariant : idleVariant,
                MENU_SPACING,
                entryScale
            );

            continue;
        }

        Rectangle minus = DecreaseBounds(layout, i);
        Rectangle plus = IncreaseBounds(layout, i);

        bool minusHovered = entryHovered && hovered.action == MenuAction::Decrease;
        bool plusHovered = entryHovered && hovered.action == MenuAction::Increase;

        font.Draw("-", {minus.x, minus.y}, minusHovered ? hoverVariant : idleVariant, MENU_SPACING, entryScale);
        font.Draw("+", {plus.x, plus.y}, plusHovered ? hoverVariant : idleVariant, MENU_SPACING, entryScale);

        // The number sits centered in the fixed field between the buttons
        int fieldLeft = static_cast<int>(minus.x + minus.width) + MENU_BUTTON_GAP * entryScale;
        int fieldWidth = MENU_VALUE_DIGITS * font.Advance(MENU_SPACING) * entryScale;
        int numberX = fieldLeft + (fieldWidth - TextWidth(entry.value, entryScale)) / 2;

        font.Draw(
            entry.value,
            {static_cast<float>(numberX), minus.y},
            idleVariant,
            MENU_SPACING,
            entryScale
        );
    }
}
