#include "Console.h"

#include <sstream>
#include <utility>

#include "Renderer.h"

constexpr float CONSOLE_PADDING = 3.0f;
constexpr float CONSOLE_ROW_GAP = 1.0f;

constexpr Color CONSOLE_BACKGROUND{24, 20, 37, 235};
constexpr Color CONSOLE_CARET{255, 255, 255, 255};

constexpr int CONSOLE_VARIANT_INPUT = FontVariant::White;
constexpr int CONSOLE_VARIANT_HISTORY = FontVariant::Grey;
constexpr int CONSOLE_VARIANT_PROMPT = FontVariant::Cyan;

static bool IsShiftDown() {
    return IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
}

// Held keys repeat, like everywhere else when typing
static bool Pressed(int key) {
    return IsKeyPressed(key) || IsKeyPressedRepeat(key);
}

void Console::Open() {
    open = true;
    browsed = entered.size();

    SetInput("");
}

void Console::Close() {
    open = false;

    SetInput("");
}

bool Console::IsOpen() const {
    return open;
}

// ":" is Shift and the semicolon on an English keyboard, Shift and the period
// on a German one. Only then are the typed characters read, everything else
// stays for whoever needs it, e.g. the digits of the tile editor.
void Console::CheckOpen() {
    if (open || !IsShiftDown() || !(IsKeyPressed(KEY_SEMICOLON) || IsKeyPressed(KEY_PERIOD))) {
        return;
    }

    for (int character = GetCharPressed(); character != 0; character = GetCharPressed()) {
        if (character == OPEN_CHARACTER) {
            Open();
            return;
        }
    }
}

const std::string &Console::Input() const {
    return input;
}

std::size_t Console::CaretPosition() const {
    return caret;
}

std::string Console::Update() {
    if (!open) {
        return "";
    }

    // Characters instead of keys, so every keyboard layout works. They land
    // where the caret stands, so a line can be fixed without retyping it.
    for (int character = GetCharPressed(); character != 0; character = GetCharPressed()) {
        if (input.size() < MAX_LENGTH && FontRenderer::CanDraw(character)) {
            input.insert(caret, 1, static_cast<char>(character));
            caret++;
        }
    }

    if (Pressed(KEY_BACKSPACE) && caret > 0) {
        input.erase(caret - 1, 1);
        caret--;
    }

    if (Pressed(KEY_DELETE) && caret < input.size()) {
        input.erase(caret, 1);
    }

    if (Pressed(KEY_LEFT)) {
        MoveCaret(-1);
    }

    if (Pressed(KEY_RIGHT)) {
        MoveCaret(1);
    }

    if (IsKeyPressed(KEY_HOME)) {
        caret = 0;
    }

    if (IsKeyPressed(KEY_END)) {
        caret = input.size();
    }

    if (IsKeyPressed(KEY_UP)) {
        Browse(-1);
    }

    if (IsKeyPressed(KEY_DOWN)) {
        Browse(1);
    }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
        std::string line = input;

        SetInput("");

        // The same line twice in a row is only remembered once
        if (!line.empty() && (entered.empty() || entered.back() != line)) {
            entered.push_back(line);
        }

        browsed = entered.size();

        return line;
    }

    return "";
}

void Console::Browse(int step) {
    if (entered.empty()) {
        return;
    }

    if (step < 0 && browsed > 0) {
        browsed--;
    } else if (step > 0 && browsed < entered.size()) {
        browsed++;
    }

    // Past the newest line the input is empty again, ready for a new one
    SetInput(browsed < entered.size() ? entered[browsed] : "");
}

void Console::SetInput(std::string line) {
    input = std::move(line);
    caret = input.size();
}

void Console::MoveCaret(int step) {
    if (step < 0 && caret > 0) {
        caret--;
    } else if (step > 0 && caret < input.size()) {
        caret++;
    }
}

void Console::Print(const std::string &text) {
    std::istringstream lines(text);

    for (std::string line; std::getline(lines, line);) {
        history.push_back(line);
    }

    while (history.size() > HISTORY) {
        history.pop_front();
    }
}

void Console::ClearHistory() {
    history.clear();
}

void Console::Draw(int viewWidth, int viewHeight, const FontRenderer &font) const {
    if (!open) {
        return;
    }

    const float row = static_cast<float>(font.LetterHeight()) + CONSOLE_ROW_GAP;
    const float height = 2.0f * CONSOLE_PADDING + static_cast<float>(history.size() + 1) * row - CONSOLE_ROW_GAP;
    const float top = static_cast<float>(viewHeight) - height;

    DrawRectangle(0, static_cast<int>(top), viewWidth, static_cast<int>(height), CONSOLE_BACKGROUND);

    float y = top + CONSOLE_PADDING;

    for (const std::string &line: history) {
        font.Draw(line, {CONSOLE_PADDING, y}, CONSOLE_VARIANT_HISTORY, TextSpacing::Narrow);
        y += row;
    }

    const std::string prompt(1, static_cast<char>(OPEN_CHARACTER));

    font.Draw(prompt, {CONSOLE_PADDING, y}, CONSOLE_VARIANT_PROMPT, TextSpacing::Narrow);

    float left = CONSOLE_PADDING + static_cast<float>(font.Measure(prompt, TextSpacing::Narrow));

    font.Draw(input, {left, y}, CONSOLE_VARIANT_INPUT, TextSpacing::Narrow);

    // The caret stands where the next character lands, like in the forms
    float caretX = left + static_cast<float>(font.Measure(input.substr(0, caret), TextSpacing::Narrow));

    if (caret > 0) {
        caretX += static_cast<float>(font.CaretSpace());
    }

    DrawRectangle(static_cast<int>(caretX), static_cast<int>(y), 1, font.LetterHeight(), CONSOLE_CARET);
}
