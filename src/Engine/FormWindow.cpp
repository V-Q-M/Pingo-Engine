#include "FormWindow.h"

#include <algorithm>
#include <utility>

#include "Renderer.h"

constexpr float FORM_PADDING = 6.0f;
constexpr float FORM_ROW_GAP = 4.0f;
constexpr float FORM_BOX_PADDING = 2.0f;
constexpr float FORM_LABEL_GAP = 8.0f;
constexpr float FORM_BUTTON_GAP = 12.0f;
constexpr float FORM_BUTTON_PADDING = 4.0f;
constexpr float FORM_SWATCH_GAP = 3.0f;

// More fields than this scroll, even if the screen would fit more
constexpr std::size_t FORM_MAX_ROWS = 5;

// The scrollbar on the right of the field rows
constexpr float FORM_SCROLLBAR_WIDTH = 3.0f;
constexpr float FORM_SCROLLBAR_GAP = 4.0f;
constexpr float FORM_CHOICE_GAP = 4.0f;

constexpr const char *FORM_CANCEL_LABEL = "Cancel";

// Dims the scene behind the form
constexpr Color FORM_SHADE{0, 0, 0, 110};

constexpr Color FORM_BACKGROUND{24, 20, 37, 245};
constexpr Color FORM_BORDER{255, 255, 255, 140};
constexpr Color FORM_BOX{0, 0, 0, 110};
constexpr Color FORM_BOX_BORDER{255, 255, 255, 90};
constexpr Color FORM_FOCUS{255, 229, 26, 255};
constexpr Color FORM_CARET{255, 255, 255, 255};
constexpr Color FORM_SCROLL_TRACK{0, 0, 0, 120};
constexpr Color FORM_SCROLL_THUMB{255, 255, 255, 150};
constexpr Color FORM_SCROLL_THUMB_HELD{255, 229, 26, 255};

constexpr int FORM_VARIANT_TEXT = FontVariant::White;
constexpr int FORM_VARIANT_ERROR = FontVariant::Red;
constexpr int FORM_VARIANT_HOVER = FontVariant::Yellow;
constexpr int FORM_VARIANT_TITLE = FontVariant::Cyan;

// The extra item at the end of a dropdown list, like "+New" in the add menu
constexpr int FORM_VARIANT_EXTRA = FontVariant::Grey;

// The menu of an option opens slightly offset next to the mouse
constexpr float FORM_OPTION_MENU_OFFSET = 5.0f;

static int DigitCount(int value) {
    int digits = 1;

    while (value >= 10) {
        value /= 10;
        digits++;
    }

    return digits;
}

// How many characters a number field needs: the digits of the largest value,
// with a negative minimum also its minus
static int NumberLength(int minimum, int maximum) {
    int length = DigitCount(std::max(maximum, 0));

    if (minimum < 0) {
        length = std::max(length, DigitCount(-minimum) + 1);
    }

    return length;
}

static int WidestOption(const std::vector<std::string> &options, const FontRenderer &font) {
    int widest = 0;

    for (const std::string &option: options) {
        widest = std::max(widest, font.Measure(option, TextSpacing::Narrow));
    }

    return widest;
}

static Rectangle Expand(Rectangle rect, float amount) {
    return {rect.x - amount, rect.y - amount, rect.width + 2.0f * amount, rect.height + 2.0f * amount};
}

void FormWindow::SetTitle(std::string title) {
    this->title = std::move(title);
}

void FormWindow::SetConfirmLabel(std::string label) {
    confirmLabel = std::move(label);
}

void FormWindow::SetHint(std::string hint) {
    this->hint = std::move(hint);
}

std::size_t FormWindow::AddText(std::string label, std::string value, std::size_t maxLength) {
    Field field;
    field.label = std::move(label);
    field.type = FieldType::Text;
    field.maxLength = maxLength;
    field.text = value.substr(0, maxLength);

    fields.push_back(std::move(field));

    return fields.size() - 1;
}

std::size_t FormWindow::AddNumber(std::string label, int value, int minimum, int maximum) {
    Field field;
    field.label = std::move(label);
    field.type = FieldType::Number;
    field.minimum = minimum;
    field.maximum = std::max(minimum, maximum);
    field.text = std::to_string(std::clamp(value, field.minimum, field.maximum));

    fields.push_back(std::move(field));

    return fields.size() - 1;
}

std::size_t FormWindow::AddColor(std::string label, std::vector<Color> colors, std::size_t selected) {
    Field field;
    field.label = std::move(label);
    field.type = FieldType::Color;
    field.colors = std::move(colors);
    field.colorIndex = field.colors.empty() ? 0 : std::min(selected, field.colors.size() - 1);

    fields.push_back(std::move(field));

    return fields.size() - 1;
}

std::size_t FormWindow::AddChoice(std::string label, std::vector<std::string> options, std::size_t selected) {
    Field field;
    field.label = std::move(label);
    field.type = FieldType::Choice;
    field.options = std::move(options);
    field.choiceIndex = field.options.empty() ? 0 : std::min(selected, field.options.size() - 1);

    fields.push_back(std::move(field));

    return fields.size() - 1;
}

std::size_t FormWindow::AddDropdown(std::string label, std::vector<std::string> options, std::size_t selected) {
    std::size_t field = AddChoice(std::move(label), std::move(options), selected);

    fields[field].type = FieldType::Dropdown;

    return field;
}

std::size_t FormWindow::AddToggle(std::string label, bool checked) {
    Field field;
    field.label = std::move(label);
    field.type = FieldType::Toggle;
    field.checked = checked;

    fields.push_back(std::move(field));

    return fields.size() - 1;
}

std::size_t FormWindow::AddButton(std::string label) {
    Field field;
    field.type = FieldType::Button;

    // The caption sits on the button, the label column stays empty
    field.text = std::move(label);

    fields.push_back(std::move(field));

    return fields.size() - 1;
}

void FormWindow::Clear() {
    list.Close();
    optionMenu.Close();
    fields.clear();
    focused = 0;
    error.clear();
}

void FormWindow::Open(int viewWidth, int viewHeight, const FontRenderer &) {
    this->viewWidth = viewWidth;
    this->viewHeight = viewHeight;

    center = {static_cast<float>(viewWidth) / 2.0f, static_cast<float>(viewHeight) / 2.0f};

    open = true;
    focused = 0;
    lastFocused = 0;
    scroll = 0;
    hoveredButton = 0;
    error.clear();
}

void FormWindow::Close() {
    list.Close();
    optionMenu.Close();
    open = false;
    hoveredButton = 0;
}

bool FormWindow::IsHovering() const {
    return open && (hoveredButton != 0 || hoveringField || list.IsHovering() || optionMenu.IsHovering());
}

bool FormWindow::IsOpen() const {
    return open;
}

void FormWindow::Focus(std::size_t field) {
    if (field < fields.size() && fields[field].visible) {
        focused = field;
    }
}

void FormWindow::Type(int codepoint) {
    if (focused >= fields.size()) {
        return;
    }

    Field &field = fields[focused];

    // Space checks or unchecks
    if (field.type == FieldType::Toggle) {
        if (codepoint == ' ') {
            field.checked = !field.checked;
            error.clear();
        }

        return;
    }

    bool digit = codepoint >= '0' && codepoint <= '9';

    if (field.type == FieldType::Text) {
        // Only what the font can actually draw
        if (!FontRenderer::CanDraw(codepoint) || FontRenderer::GlyphCount(field.text) >= field.maxLength) {
            return;
        }
    } else if (field.type == FieldType::Number) {
        // A minus only at the start, and only if the field allows negative numbers
        bool minus = codepoint == '-' && field.minimum < 0 && field.text.empty();

        if ((!digit && !minus) || static_cast<int>(field.text.size()) >= NumberLength(field.minimum, field.maximum)) {
            return;
        }
    } else {
        return;
    }

    // Umlauts consist of two bytes in UTF-8
    field.text += FontRenderer::Encode(codepoint);
    error.clear();
}

void FormWindow::Erase() {
    // The caption of a button is not typed
    if (focused >= fields.size() || fields[focused].text.empty() || fields[focused].type == FieldType::Button) {
        return;
    }

    FontRenderer::RemoveLastGlyph(fields[focused].text);
    error.clear();
}

void FormWindow::SelectColor(std::size_t field, std::size_t index) {
    if (field >= fields.size() || index >= fields[field].colors.size()) {
        return;
    }

    fields[field].colorIndex = index;
    error.clear();
}

void FormWindow::SelectChoice(std::size_t field, std::size_t index) {
    if (field >= fields.size() || index >= fields[field].options.size()) {
        return;
    }

    fields[field].choiceIndex = index;
    error.clear();
}

void FormWindow::SetChecked(std::size_t field, bool checked) {
    if (field >= fields.size() || fields[field].type != FieldType::Toggle) {
        return;
    }

    fields[field].checked = checked;
    error.clear();
}

void FormWindow::SetNumber(std::size_t field, int value) {
    if (field >= fields.size() || fields[field].type != FieldType::Number) {
        return;
    }

    fields[field].text = std::to_string(std::clamp(value, fields[field].minimum, fields[field].maximum));
    error.clear();
}

void FormWindow::SetFieldVisible(std::size_t field, bool visible) {
    if (field >= fields.size()) {
        return;
    }

    fields[field].visible = visible;

    if (visible) {
        return;
    }

    if (list.IsOpen() && listField == field) {
        list.Close();
        optionMenu.Close();
    }

    // A hidden field cannot keep the focus, the first visible one gets it
    if (focused == field) {
        for (std::size_t i = 0; i < fields.size(); i++) {
            if (fields[i].visible) {
                focused = i;
                break;
            }
        }
    }
}

bool FormWindow::IsFieldVisible(std::size_t field) const {
    return fields.at(field).visible;
}

bool FormWindow::IsListOpen() const {
    return list.IsOpen();
}

const ContextMenu &FormWindow::List() const {
    return list;
}

void FormWindow::OpenList(std::size_t field, const FontRenderer &font) {
    Rectangle box = FieldBounds(field, font);

    listField = field;

    list.SetTitle(fields[field].label);
    SetListItems();
    list.Open({box.x, box.y + box.height + 1.0f}, viewWidth, viewHeight, font);
}

void FormWindow::SetListItems() {
    const Field &field = fields[listField];

    std::vector<std::string> items = field.options;

    if (!field.extra.empty()) {
        items.push_back(field.extra);
    }

    list.SetItems(std::move(items));

    if (!field.extra.empty()) {
        list.SetItemVariant(field.options.size(), FORM_VARIANT_EXTRA);
    }
}

void FormWindow::OpenOptionMenu(std::size_t option, Vector2 mousePosition, const FontRenderer &font) {
    const Field &field = fields[listField];

    optionMenuOption = option;

    optionMenu.SetTitle(field.options[option]);
    optionMenu.SetItems(field.actions);

    for (std::size_t i = 0; i < field.actions.size(); i++) {
        if (i < field.actionIcons.size()) {
            optionMenu.SetItemIcon(i, field.actionIcons[i]);
        }

        optionMenu.SetItemEnabled(i, !field.actionFilter || field.actionFilter(option, i));
    }

    optionMenu.Open(
        {mousePosition.x + FORM_OPTION_MENU_OFFSET, mousePosition.y + FORM_OPTION_MENU_OFFSET},
        viewWidth,
        viewHeight,
        font
    );
}

FormWindow::Result FormWindow::UpdateList(Vector2 mousePosition, bool clicked, bool rightClicked, const FontRenderer &font) {
    const Field &field = fields[listField];

    // The menu of an option lies above the list. A click next to it only closes
    // the menu, the list stays.
    if (optionMenu.IsOpen()) {
        int action = optionMenu.Update(mousePosition, clicked, font);

        if (action == ContextMenu::NOTHING) {
            return Result::None;
        }

        eventField = listField;
        eventOption = optionMenuOption;
        eventAction = static_cast<std::size_t>(action);

        return Result::OptionAction;
    }

    if (rightClicked) {
        int item = list.ItemAt(mousePosition, font);

        if (item != ContextMenu::NOTHING && static_cast<std::size_t>(item) < field.options.size() &&
            !field.actions.empty()) {
            OpenOptionMenu(static_cast<std::size_t>(item), mousePosition, font);
        }

        return Result::None;
    }

    int item = list.Update(mousePosition, clicked, font);

    if (item == ContextMenu::NOTHING) {
        return Result::None;
    }

    // The extra item keeps the list open, so whatever it creates can be chosen
    // right away
    if (static_cast<std::size_t>(item) >= field.options.size()) {
        eventField = listField;

        OpenList(listField, font);

        return Result::ListExtra;
    }

    SelectChoice(listField, static_cast<std::size_t>(item));

    return Result::None;
}

void FormWindow::SetDropdownExtra(std::size_t field, std::string label) {
    if (field < fields.size()) {
        fields[field].extra = std::move(label);
    }
}

void FormWindow::SetOptionActions(std::size_t field, std::vector<std::string> labels, std::vector<char> icons) {
    if (field < fields.size()) {
        fields[field].actions = std::move(labels);
        fields[field].actionIcons = std::move(icons);
    }
}

void FormWindow::SetOptionActionFilter(std::size_t field, std::function<bool(std::size_t, std::size_t)> filter) {
    if (field < fields.size()) {
        fields[field].actionFilter = std::move(filter);
    }
}

void FormWindow::SetOptions(std::size_t field, std::vector<std::string> options, std::size_t selected) {
    if (field >= fields.size()) {
        return;
    }

    Field &entry = fields[field];

    entry.options = std::move(options);
    entry.choiceIndex = entry.options.empty() ? 0 : std::min(selected, entry.options.size() - 1);

    error.clear();

    if (list.IsOpen() && listField == field) {
        SetListItems();
    }
}

bool FormWindow::CloseList() {
    if (optionMenu.IsOpen()) {
        optionMenu.Close();
        return true;
    }

    if (list.IsOpen()) {
        list.Close();
        return true;
    }

    return false;
}

std::size_t FormWindow::EventField() const {
    return eventField;
}

std::size_t FormWindow::EventOption() const {
    return eventOption;
}

std::size_t FormWindow::EventAction() const {
    return eventAction;
}

bool FormWindow::IsChecked(std::size_t field) const {
    return fields.at(field).checked;
}

std::size_t FormWindow::ColorIndex(std::size_t field) const {
    return fields.at(field).colorIndex;
}

std::size_t FormWindow::Choice(std::size_t field) const {
    return fields.at(field).choiceIndex;
}

std::size_t FormWindow::FieldCount() const {
    return fields.size();
}

const std::string &FormWindow::Text(std::size_t field) const {
    return fields.at(field).text;
}

int FormWindow::Number(std::size_t field) const {
    const Field &entry = fields.at(field);

    if (entry.text.empty() || entry.text == "-") {
        return std::clamp(0, entry.minimum, entry.maximum);
    }

    // The text consists only of digits and is not longer than the maximum
    return std::clamp(std::stoi(entry.text), entry.minimum, entry.maximum);
}

Color FormWindow::ColorValue(std::size_t field) const {
    const Field &entry = fields.at(field);

    return entry.colors.empty() ? WHITE : entry.colors[entry.colorIndex];
}

void FormWindow::SetError(std::string error) {
    this->error = std::move(error);
}

const std::string &FormWindow::Error() const {
    return error;
}

float FormWindow::RowHeight(const FontRenderer &font) const {
    return static_cast<float>(font.LetterHeight()) + 2.0f * FORM_BOX_PADDING;
}

float FormWindow::RowTop(std::size_t row, const FontRenderer &font) const {
    return TopLeft(font).y + FORM_PADDING + static_cast<float>(row) * (RowHeight(font) + FORM_ROW_GAP);
}

std::size_t FormWindow::ButtonRow(const FontRenderer &font) const {
    return ShownRows(font) + (hint.empty() ? 1 : 2);
}

std::size_t FormWindow::FieldRow(std::size_t field) const {
    // Row 0 is the title, the scrolled away fields above take none
    return 1 + VisibleIndex(field) - std::min(scroll, VisibleIndex(field));
}

std::size_t FormWindow::VisibleIndex(std::size_t field) const {
    std::size_t index = 0;

    for (std::size_t i = 0; i < field && i < fields.size(); i++) {
        if (fields[i].visible) {
            index++;
        }
    }

    return index;
}

// At most five rows, and never more than the viewport can hold: title, hint
// and buttons stay, the fields in between scroll.
std::size_t FormWindow::FittingRows(const FontRenderer &font) const {
    float row = RowHeight(font) + FORM_ROW_GAP;

    // Title, buttons and the hint row, if there is a hint
    float taken = 2.0f * FORM_PADDING + static_cast<float>(hint.empty() ? 2 : 3) * row;
    float free = static_cast<float>(viewHeight) - taken;

    std::size_t fits = static_cast<std::size_t>(std::max(static_cast<int>(free / row), 1));

    return std::min(fits, FORM_MAX_ROWS);
}

std::size_t FormWindow::ShownRows(const FontRenderer &font) const {
    return std::min(VisibleFieldCount(), FittingRows(font));
}

bool FormWindow::IsScrollable(const FontRenderer &font) const {
    return VisibleFieldCount() > FittingRows(font);
}

bool FormWindow::IsFieldShown(std::size_t field, const FontRenderer &font) const {
    if (field >= fields.size() || !fields[field].visible) {
        return false;
    }

    std::size_t index = VisibleIndex(field);

    return index >= scroll && index < scroll + ShownRows(font);
}

void FormWindow::Scroll(int rows, const FontRenderer &font) {
    std::size_t last = VisibleFieldCount() - ShownRows(font);
    int target = static_cast<int>(scroll) + rows;

    scroll = static_cast<std::size_t>(std::clamp(target, 0, static_cast<int>(last)));
}

// A click on the track jumps there and holds on to the bar, so it follows the
// mouse until the button is released
void FormWindow::UpdateScrollbar(Vector2 mousePosition, bool clicked, const FontRenderer &font) {
    Rectangle track = ScrollbarBounds(font);

    if (clicked && CheckCollisionPointRec(mousePosition, Expand(track, 2.0f))) {
        draggingScroll = true;
    }

    if (draggingScroll && !IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        draggingScroll = false;
    }

    if (!draggingScroll) {
        return;
    }

    float thumb = ScrollThumbBounds(font).height;
    float free = track.height - thumb;
    float steps = static_cast<float>(VisibleFieldCount() - ShownRows(font));

    if (free <= 0.0f || steps <= 0.0f) {
        return;
    }

    // The mouse holds the middle of the bar
    float share = (mousePosition.y - track.y - thumb / 2.0f) / free;

    scroll = static_cast<std::size_t>(std::clamp(std::round(share * steps), 0.0f, steps));
}

void FormWindow::ScrollTo(std::size_t field, const FontRenderer &font) {
    if (field >= fields.size() || !fields[field].visible) {
        return;
    }

    std::size_t index = VisibleIndex(field);
    std::size_t rows = ShownRows(font);

    if (index < scroll) {
        scroll = index;
    } else if (index >= scroll + rows) {
        scroll = index - rows + 1;
    }
}

std::size_t FormWindow::VisibleFieldCount() const {
    return static_cast<std::size_t>(std::count_if(fields.begin(), fields.end(), [](const Field &field) {
        return field.visible;
    }));
}

Vector2 FormWindow::TopLeft(const FontRenderer &font) const {
    Vector2 size = Size(font);

    return Renderer::SnapToPixel({center.x - size.x / 2.0f, center.y - size.y / 2.0f});
}

float FormWindow::LabelWidth(const FontRenderer &font) const {
    int width = 0;

    for (const Field &field: fields) {
        width = std::max(width, font.Measure(field.label, TextSpacing::Narrow));
    }

    return static_cast<float>(width);
}

float FormWindow::ControlWidth(const Field &field, const FontRenderer &font) const {
    float advance = static_cast<float>(font.Advance(TextSpacing::Narrow));

    switch (field.type) {
        case FieldType::Text:
            return static_cast<float>(field.maxLength) * advance + 2.0f * FORM_BOX_PADDING + 1.0f;

        case FieldType::Number:
            return static_cast<float>(NumberLength(field.minimum, field.maximum)) * advance +
                   2.0f * FORM_BOX_PADDING + 1.0f;

        case FieldType::Color:
            return static_cast<float>(field.colors.size()) * (RowHeight(font) + FORM_SWATCH_GAP) - FORM_SWATCH_GAP;

        case FieldType::Choice:
            return 2.0f * FORM_BOX_PADDING + 2.0f * static_cast<float>(font.LetterWidth()) +
                   2.0f * FORM_CHOICE_GAP + static_cast<float>(WidestOption(field.options, font));

        case FieldType::Dropdown:
            // The option on the left, the down arrow on the right
            return 2.0f * FORM_BOX_PADDING + static_cast<float>(WidestOption(field.options, font)) +
                   FORM_CHOICE_GAP + static_cast<float>(font.LetterWidth());

        case FieldType::Toggle:
            // A square box
            return RowHeight(font);

        case FieldType::Button:
            return ButtonWidth(field.text, font);
    }

    return 0.0f;
}

float FormWindow::ButtonWidth(const std::string &label, const FontRenderer &font) const {
    return static_cast<float>(font.Measure(label, TextSpacing::Narrow)) + 2.0f * FORM_BUTTON_PADDING;
}

Vector2 FormWindow::Size(const FontRenderer &font) const {
    float controls = 0.0f;

    for (const Field &field: fields) {
        controls = std::max(controls, ControlWidth(field, font));
    }

    float width = std::max({
        static_cast<float>(font.Measure(title, TextSpacing::Narrow)),
        LabelWidth(font) + FORM_LABEL_GAP + controls,
        static_cast<float>(font.Measure(hint, TextSpacing::Narrow)),
        static_cast<float>(font.Measure(error, TextSpacing::Narrow)),
        ButtonWidth(confirmLabel, font) + FORM_BUTTON_GAP + ButtonWidth(FORM_CANCEL_LABEL, font)
    });

    // The scrollbar needs its own column on the right
    if (IsScrollable(font)) {
        width += FORM_SCROLLBAR_GAP + FORM_SCROLLBAR_WIDTH;
    }

    // Title, fields, buttons and the hint row, if there is a hint
    std::size_t rows = ButtonRow(font) + 1;

    return {
        width + 2.0f * FORM_PADDING,
        2.0f * FORM_PADDING + static_cast<float>(rows) * RowHeight(font) +
        static_cast<float>(rows - 1) * FORM_ROW_GAP
    };
}

Rectangle FormWindow::Bounds(const FontRenderer &font) const {
    Vector2 size = Size(font);
    Vector2 topLeft = TopLeft(font);

    return {topLeft.x, topLeft.y, size.x, size.y};
}

// The track of the scrollbar, on the right next to the field rows
Rectangle FormWindow::ScrollbarBounds(const FontRenderer &font) const {
    Rectangle bounds = Bounds(font);
    float rows = static_cast<float>(ShownRows(font));

    return {
        bounds.x + bounds.width - FORM_PADDING - FORM_SCROLLBAR_WIDTH,
        RowTop(1, font),
        FORM_SCROLLBAR_WIDTH,
        rows * (RowHeight(font) + FORM_ROW_GAP) - FORM_ROW_GAP
    };
}

// The part of the track that shows where the view sits
Rectangle FormWindow::ScrollThumbBounds(const FontRenderer &font) const {
    Rectangle track = ScrollbarBounds(font);

    float total = static_cast<float>(VisibleFieldCount());
    float shown = static_cast<float>(ShownRows(font));

    float height = std::max(track.height * shown / total, RowHeight(font) / 2.0f);
    float free = track.height - height;
    float steps = total - shown;

    return {
        track.x,
        track.y + (steps > 0.0f ? free * static_cast<float>(scroll) / steps : 0.0f),
        track.width,
        height
    };
}

Rectangle FormWindow::FieldBounds(std::size_t field, const FontRenderer &font) const {
    return {
        TopLeft(font).x + FORM_PADDING + LabelWidth(font) + FORM_LABEL_GAP,
        RowTop(FieldRow(field), font),
        ControlWidth(fields.at(field), font),
        RowHeight(font)
    };
}

Rectangle FormWindow::SwatchBounds(std::size_t field, std::size_t index, const FontRenderer &font) const {
    Rectangle bounds = FieldBounds(field, font);
    float size = RowHeight(font);

    return {bounds.x + static_cast<float>(index) * (size + FORM_SWATCH_GAP), bounds.y, size, size};
}

Rectangle FormWindow::ChoiceArrowBounds(std::size_t field, bool left, const FontRenderer &font) const {
    Rectangle box = FieldBounds(field, font);

    float width = FORM_BOX_PADDING + static_cast<float>(font.LetterWidth()) + FORM_CHOICE_GAP / 2.0f;

    return {left ? box.x : box.x + box.width - width, box.y, width, box.height};
}

Rectangle FormWindow::ConfirmBounds(const FontRenderer &font) const {
    return {
        TopLeft(font).x + FORM_PADDING,
        RowTop(ButtonRow(font), font),
        ButtonWidth(confirmLabel, font),
        RowHeight(font)
    };
}

Rectangle FormWindow::CancelBounds(const FontRenderer &font) const {
    Rectangle confirm = ConfirmBounds(font);

    return {
        confirm.x + confirm.width + FORM_BUTTON_GAP,
        confirm.y,
        ButtonWidth(FORM_CANCEL_LABEL, font),
        RowHeight(font)
    };
}

FormWindow::Result FormWindow::Update(Vector2 mousePosition, bool clicked, const FontRenderer &font, bool rightClicked) {
    if (!open) {
        return Result::None;
    }

    // An open list belongs to the mouse alone, a click next to it closes it
    if (list.IsOpen()) {
        hoveredButton = 0;
        hoveringField = false;

        while (GetCharPressed() != 0) {
        }

        return UpdateList(mousePosition, clicked, rightClicked, font);
    }

    // More fields than fit: the wheel scrolls through them, the scrollbar can
    // be dragged
    float wheel = GetMouseWheelMove();

    if (IsScrollable(font)) {
        if (wheel != 0.0f) {
            Scroll(wheel > 0.0f ? -1 : 1, font);
        }

        UpdateScrollbar(mousePosition, clicked, font);
    } else {
        draggingScroll = false;
    }

    hoveredButton = 0;

    if (CheckCollisionPointRec(mousePosition, ConfirmBounds(font))) {
        hoveredButton = 1;
    } else if (CheckCollisionPointRec(mousePosition, CancelBounds(font))) {
        hoveredButton = 2;
    }

    hoveringField = draggingScroll ||
                    (IsScrollable(font) && CheckCollisionPointRec(mousePosition, Expand(ScrollbarBounds(font), 2.0f)));
    hoveredField = -1;

    for (std::size_t i = 0; i < fields.size(); i++) {
        if (!IsFieldShown(i, font)) {
            continue;
        }

        if (CheckCollisionPointRec(mousePosition, FieldBounds(i, font))) {
            hoveringField = true;
            hoveredField = static_cast<int>(i);
        }

        for (std::size_t s = 0; s < fields[i].colors.size(); s++) {
            if (CheckCollisionPointRec(mousePosition, SwatchBounds(i, s, font))) {
                hoveringField = true;
            }
        }
    }

    if (clicked) {
        if (hoveredButton == 1) {
            return Result::Confirmed;
        }

        if (hoveredButton == 2) {
            Close();
            return Result::Cancelled;
        }

        for (std::size_t i = 0; i < fields.size(); i++) {
            if (!IsFieldShown(i, font)) {
                continue;
            }

            if (fields[i].type == FieldType::Color) {
                for (std::size_t s = 0; s < fields[i].colors.size(); s++) {
                    if (CheckCollisionPointRec(mousePosition, SwatchBounds(i, s, font))) {
                        Focus(i);
                        SelectColor(i, s);
                    }
                }
            } else if (fields[i].type == FieldType::Choice && !fields[i].options.empty() &&
                       CheckCollisionPointRec(mousePosition, FieldBounds(i, font))) {
                // The left arrow goes back, everything else in the field goes forward
                std::size_t count = fields[i].options.size();
                bool back = CheckCollisionPointRec(mousePosition, ChoiceArrowBounds(i, true, font));

                Focus(i);
                SelectChoice(i, back ? (fields[i].choiceIndex + count - 1) % count : (fields[i].choiceIndex + 1) % count);
            } else if (fields[i].type == FieldType::Dropdown && !fields[i].options.empty() &&
                       CheckCollisionPointRec(mousePosition, FieldBounds(i, font))) {
                Focus(i);
                OpenList(i, font);
            } else if (fields[i].type == FieldType::Button && CheckCollisionPointRec(mousePosition, FieldBounds(i, font))) {
                eventField = i;
                return Result::FieldButton;
            } else if (fields[i].type == FieldType::Toggle && CheckCollisionPointRec(mousePosition, FieldBounds(i, font))) {
                Focus(i);
                SetChecked(i, !fields[i].checked);
            } else if (CheckCollisionPointRec(mousePosition, FieldBounds(i, font))) {
                Focus(i);
            }
        }
    }

    for (int character = GetCharPressed(); character != 0; character = GetCharPressed()) {
        Type(character);
    }

    if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
        Erase();
    }

    // Tab skips hidden fields
    if (IsKeyPressed(KEY_TAB) && VisibleFieldCount() > 0) {
        std::size_t next = focused;

        do {
            next = (next + 1) % fields.size();
        } while (!fields[next].visible);

        Focus(next);
    }

    if (focused < fields.size() && fields[focused].type == FieldType::Color && !fields[focused].colors.empty()) {
        std::size_t count = fields[focused].colors.size();
        std::size_t index = fields[focused].colorIndex;

        if (IsKeyPressed(KEY_LEFT)) {
            SelectColor(focused, (index + count - 1) % count);
        }

        if (IsKeyPressed(KEY_RIGHT)) {
            SelectColor(focused, (index + 1) % count);
        }
    }

    bool focusedOnOptions = focused < fields.size() &&
                            (fields[focused].type == FieldType::Choice || fields[focused].type == FieldType::Dropdown);

    if (focusedOnOptions && !fields[focused].options.empty()) {
        std::size_t count = fields[focused].options.size();
        std::size_t index = fields[focused].choiceIndex;

        if (IsKeyPressed(KEY_LEFT)) {
            SelectChoice(focused, (index + count - 1) % count);
        }

        if (IsKeyPressed(KEY_RIGHT)) {
            SelectChoice(focused, (index + 1) % count);
        }
    }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
        return Result::Confirmed;
    }

    // A field that was just focused, with Tab or from outside, scrolls into view
    if (focused != lastFocused) {
        ScrollTo(focused, font);
        lastFocused = focused;
    }

    return Result::None;
}

void FormWindow::Draw(const FontRenderer &font) const {
    if (!open) {
        return;
    }

    DrawRectangle(0, 0, viewWidth, viewHeight, FORM_SHADE);

    Rectangle bounds = Bounds(font);

    DrawRectangleRec(bounds, FORM_BACKGROUND);
    DrawRectangleLinesEx(bounds, 1.0f, FORM_BORDER);

    float left = TopLeft(font).x + FORM_PADDING;

    // Without a hint row an error appears instead of the title. That way the
    // window does not grow, and the buttons do not jump away under the mouse.
    bool errorInTitle = hint.empty() && !error.empty();

    font.Draw(
        errorInTitle ? error : title,
        {left, RowTop(0, font) + FORM_BOX_PADDING},
        errorInTitle ? FORM_VARIANT_ERROR : FORM_VARIANT_TITLE,
        TextSpacing::Narrow
    );

    // The scrollbar next to the fields shows how much more there is
    if (IsScrollable(font)) {
        DrawRectangleRec(ScrollbarBounds(font), FORM_SCROLL_TRACK);
        DrawRectangleRec(ScrollThumbBounds(font), draggingScroll ? FORM_SCROLL_THUMB_HELD : FORM_SCROLL_THUMB);
    }

    for (std::size_t i = 0; i < fields.size(); i++) {
        const Field &field = fields[i];
        bool isFocused = i == focused;

        if (!IsFieldShown(i, font)) {
            continue;
        }

        font.Draw(field.label, {left, RowTop(FieldRow(i), font) + FORM_BOX_PADDING}, FORM_VARIANT_TEXT, TextSpacing::Narrow);

        // Looks like Save and Cancel, just in the row of its field
        if (field.type == FieldType::Button) {
            Rectangle button = FieldBounds(i, font);
            bool hovered = hoveredField == static_cast<int>(i);

            DrawRectangleLinesEx(button, 1.0f, hovered ? FORM_FOCUS : FORM_BORDER);

            font.Draw(
                field.text,
                {button.x + FORM_BUTTON_PADDING, button.y + FORM_BOX_PADDING},
                hovered ? FORM_VARIANT_HOVER : FORM_VARIANT_TEXT,
                TextSpacing::Narrow
            );

            continue;
        }

        if (field.type == FieldType::Color) {
            for (std::size_t s = 0; s < field.colors.size(); s++) {
                Rectangle swatch = SwatchBounds(i, s, font);

                DrawRectangleRec(swatch, field.colors[s]);

                if (s == field.colorIndex) {
                    DrawRectangleLinesEx(Expand(swatch, 1.0f), 1.0f, isFocused ? FORM_FOCUS : FORM_CARET);
                }
            }

            continue;
        }

        Rectangle box = FieldBounds(i, font);

        DrawRectangleRec(box, FORM_BOX);
        DrawRectangleLinesEx(box, 1.0f, isFocused ? FORM_FOCUS : FORM_BOX_BORDER);

        if (field.type == FieldType::Dropdown) {
            float top = box.y + FORM_BOX_PADDING;
            bool listOpen = list.IsOpen() && listField == i;

            if (!field.options.empty()) {
                font.Draw(field.options[field.choiceIndex], {box.x + FORM_BOX_PADDING, top}, FORM_VARIANT_TEXT, TextSpacing::Narrow);
            }

            font.Draw(
                std::string(1, FontRenderer::ARROW_DOWN),
                {box.x + box.width - FORM_BOX_PADDING - static_cast<float>(font.LetterWidth()), top},
                isFocused || listOpen ? FORM_VARIANT_HOVER : FORM_VARIANT_TEXT
            );

            continue;
        }

        if (field.type == FieldType::Toggle) {
            if (field.checked) {
                float width = static_cast<float>(font.Measure(FontRenderer::CROSS, TextSpacing::Narrow));

                font.Draw(
                    FontRenderer::CROSS,
                    Renderer::SnapToPixel({box.x + (box.width - width) / 2.0f, box.y + FORM_BOX_PADDING}),
                    FORM_VARIANT_TEXT,
                    TextSpacing::Narrow
                );
            }

            continue;
        }

        if (field.type == FieldType::Choice) {
            float top = box.y + FORM_BOX_PADDING;
            int arrowVariant = isFocused ? FORM_VARIANT_HOVER : FORM_VARIANT_TEXT;

            font.Draw(std::string(1, FontRenderer::ARROW_LEFT), {box.x + FORM_BOX_PADDING, top}, arrowVariant);
            font.Draw(
                std::string(1, FontRenderer::ARROW_RIGHT),
                {box.x + box.width - FORM_BOX_PADDING - static_cast<float>(font.LetterWidth()), top},
                arrowVariant
            );

            if (!field.options.empty()) {
                const std::string &option = field.options[field.choiceIndex];
                float width = static_cast<float>(font.Measure(option, TextSpacing::Narrow));

                font.Draw(
                    option,
                    Renderer::SnapToPixel({box.x + (box.width - width) / 2.0f, top}),
                    FORM_VARIANT_TEXT,
                    TextSpacing::Narrow
                );
            }

            continue;
        }

        Vector2 textPosition = {box.x + FORM_BOX_PADDING, box.y + FORM_BOX_PADDING};

        font.Draw(field.text, textPosition, FORM_VARIANT_TEXT, TextSpacing::Narrow);

        if (isFocused) {
            float caretX = textPosition.x + static_cast<float>(font.Measure(field.text, TextSpacing::Narrow));

            DrawRectangle(
                static_cast<int>(caretX),
                static_cast<int>(textPosition.y),
                1,
                font.LetterHeight(),
                FORM_CARET
            );
        }
    }

    // An error replaces the hint
    if (!hint.empty()) {
        const std::string &message = error.empty() ? hint : error;

        font.Draw(
            message,
            {left, RowTop(ShownRows(font) + 1, font) + FORM_BOX_PADDING},
            error.empty() ? FORM_VARIANT_TEXT : FORM_VARIANT_ERROR,
            TextSpacing::Narrow
        );
    }

    auto button = [&](Rectangle rect, const std::string &label, bool hovered) {
        DrawRectangleLinesEx(rect, 1.0f, hovered ? FORM_FOCUS : FORM_BORDER);

        font.Draw(
            label,
            {rect.x + FORM_BUTTON_PADDING, rect.y + FORM_BOX_PADDING},
            hovered ? FORM_VARIANT_HOVER : FORM_VARIANT_TEXT,
            TextSpacing::Narrow
        );
    };

    button(ConfirmBounds(font), confirmLabel, hoveredButton == 1);
    button(CancelBounds(font), FORM_CANCEL_LABEL, hoveredButton == 2);

    // The open list lies above everything, the menu of an option above it
    list.Draw(font);
    optionMenu.Draw(font);
}
