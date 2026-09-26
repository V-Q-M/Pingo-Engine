#include "CodeEditor.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <sstream>
#include <utility>

#include "Renderer.h"

constexpr float EDITOR_PADDING = 3.0f;

// Space between two tabs, so their names do not stick together
constexpr float EDITOR_TAB_GAP = 3.0f;

// Space between two lines of code
constexpr float EDITOR_LINE_GAP = 1.0f;

// Room for the line numbers, in characters
constexpr int EDITOR_NUMBER_WIDTH = 3;

constexpr float EDITOR_SCROLLBAR_WIDTH = 3.0f;
constexpr float EDITOR_SCROLLBAR_GAP = 2.0f;

constexpr Color EDITOR_SHADE{0, 0, 0, 120};
constexpr Color EDITOR_BACKGROUND{13, 15, 24, 250};
constexpr Color EDITOR_BORDER{255, 255, 255, 140};
constexpr Color EDITOR_TAB{24, 20, 37, 255};
constexpr Color EDITOR_CARET{255, 255, 255, 255};

// The caret of the normal mode covers a whole character, so the letter below
// it still shows through
constexpr Color EDITOR_CARET_BLOCK{255, 255, 255, 110};
constexpr Color EDITOR_SELECTION{90, 140, 255, 110};
constexpr Color EDITOR_CARET_LINE{255, 255, 255, 18};
constexpr Color EDITOR_SCROLL_TRACK{0, 0, 0, 120};
constexpr Color EDITOR_SCROLL_THUMB{255, 255, 255, 150};
constexpr Color EDITOR_SCROLL_THUMB_HELD{255, 229, 26, 255};

constexpr int EDITOR_VARIANT_TEXT = FontVariant::White;
constexpr int EDITOR_VARIANT_NUMBER = FontVariant::Grey;
constexpr int EDITOR_VARIANT_TAB = FontVariant::Grey;
constexpr int EDITOR_VARIANT_TAB_OPEN = FontVariant::Cyan;
constexpr int EDITOR_VARIANT_HINT = FontVariant::Grey;
constexpr int EDITOR_VARIANT_COMMAND = FontVariant::Cyan;

// The colors of the code itself
constexpr int EDITOR_VARIANT_KEYWORD = FontVariant::Cyan;
constexpr int EDITOR_VARIANT_STRING = FontVariant::Green;
constexpr int EDITOR_VARIANT_NUMBER_LITERAL = FontVariant::Yellow;
constexpr int EDITOR_VARIANT_COMMENT = FontVariant::Grey;
constexpr int EDITOR_VARIANT_PREPROCESSOR = FontVariant::Red;

// What the editor paints in the color of a keyword
static const std::array<const char *, 39> KEYWORDS{
    "auto", "bool", "break", "case", "char", "class", "const", "constexpr", "continue",
    "default", "delete", "do", "double", "else", "enum", "explicit", "false", "float",
    "for", "if", "inline", "int", "namespace", "new", "nullptr", "override", "private",
    "protected", "public", "return", "static", "struct", "switch", "template", "this",
    "true", "using", "virtual", "void"
};

static bool IsWordCharacter(char letter) {
    return std::isalnum(static_cast<unsigned char>(letter)) || letter == '_';
}

// Spaces, the characters of a word and everything else, the three kinds a
// word object tells apart
static int CharacterClass(char letter) {
    if (letter == ' ' || letter == '\t') {
        return 0;
    }

    return IsWordCharacter(letter) ? 1 : 2;
}

// The column of the next word of a line, or its length when none follows
static std::size_t NextWordStart(const std::string &line, std::size_t column) {
    while (column < line.size() && IsWordCharacter(line[column])) {
        column++;
    }

    while (column < line.size() && !IsWordCharacter(line[column])) {
        column++;
    }

    return column;
}

// The column of the last character of the word that follows after from, or
// npos when the line has no more. Pass -1 to include the first column.
static std::size_t WordEnd(const std::string &line, long from) {
    auto at = static_cast<std::size_t>(from + 1);

    while (at < line.size() && CharacterClass(line[at]) == 0) {
        at++;
    }

    if (at >= line.size()) {
        return std::string::npos;
    }

    int kind = CharacterClass(line[at]);

    while (at + 1 < line.size() && CharacterClass(line[at + 1]) == kind) {
        at++;
    }

    return at;
}

static bool IsKeyword(const std::string &word) {
    return std::any_of(KEYWORDS.begin(), KEYWORDS.end(), [&](const char *keyword) {
        return word == keyword;
    });
}

// Held keys repeat, like everywhere else when typing
static bool Pressed(int key) {
    return IsKeyPressed(key) || IsKeyPressedRepeat(key);
}

// Control zooms, on macOS the command key does as well
static bool ZoomModifier() {
    return IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL) ||
           IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER);
}

// Nothing but spaces
static bool IsBlank(const std::string &line) {
    return line.find_first_not_of(" \t") == std::string::npos;
}

void CodeEditor::Open(std::vector<File> files, Rectangle area, int viewWidth, int viewHeight, int pixelScale) {
    this->files = std::move(files);

    SetLayout(area, viewWidth, viewHeight, pixelScale);

    // It opens in the size of the game's own font, from there it is zoomed
    scale = this->pixelScale;

    if (this->files.empty()) {
        this->files.push_back(File{});
    }

    lines.clear();

    for (const File &file: this->files) {
        lines.push_back(SplitLines(file.text));
    }

    current = 0;
    caret = Caret{};
    firstRow = 0;
    firstColumn = 0;
    draggingScroll = false;
    draggingSideScroll = false;
    open = true;

    // The file may well start with lines of the engine
    SettleCaret(1);

    history.assign(this->files.size(), History{});
    sessionStart.reset();
    historyMove = false;

    mode = vim ? VimMode::Normal : VimMode::Insert;
    pendingG = false;
    pendingOperator = 0;
    pendingObject = 0;
    count = 0;
    request = Request::None;
    answer.clear();

    command.Close();
    AddCommands();
}

void CodeEditor::SetLayout(Rectangle area, int viewWidth, int viewHeight, int pixelScale) {
    this->area = area;
    this->viewWidth = viewWidth;
    this->viewHeight = viewHeight;
    this->pixelScale = std::max(pixelScale, 1);
}

void CodeEditor::Close() {
    open = false;
    draggingScroll = false;
    draggingSideScroll = false;
}

// The three commands of Vim that the editor understands. They only say what
// the editor wants, the work is done by whoever opened it.
void CodeEditor::AddCommands() {
    commands.Clear();

    commands.Add({"w", "", 0, [this](const std::vector<std::string> &) {
        request = Request::Save;

        return std::string("written");
    }});

    commands.Add({"wq", "", 0, [this](const std::vector<std::string> &) {
        request = Request::SaveAndClose;

        return std::string();
    }});

    commands.Add({"q", "", 0, [this](const std::vector<std::string> &) {
        request = Request::Close;

        return std::string();
    }});
}

void CodeEditor::RunCommand(const std::string &line) {
    answer = commands.Run(line);

    command.Close();
}

CodeEditor::Request CodeEditor::OnEscape() {
    if (!open) {
        return Request::None;
    }

    // A command line is thrown away, nothing of it runs
    if (command.IsOpen()) {
        command.Close();
        answer.clear();

        return Request::None;
    }

    if (!vim) {
        return Request::SaveAndClose;
    }

    // Like in Vim: out of the typing, and in the normal mode ESC does nothing
    SetMode(VimMode::Normal);

    return Request::None;
}

void CodeEditor::MarkSaved() {
    for (File &file: files) {
        file.changed = false;
    }
}

void CodeEditor::SetVimEnabled(bool enabled) {
    vim = enabled;

    SetMode(enabled ? VimMode::Normal : VimMode::Insert);
}

bool CodeEditor::IsVimEnabled() const {
    return vim;
}

CodeEditor::VimMode CodeEditor::GetMode() const {
    return mode;
}

void CodeEditor::SetMode(VimMode mode) {
    // Leaving the typing ends the step that started with the command
    if (this->mode == VimMode::Insert && mode != VimMode::Insert && sessionStart) {
        State start = std::move(*sessionStart);

        sessionStart.reset();

        if (start.file < lines.size() && lines[start.file] != start.lines) {
            Record(std::move(start));
        }
    }

    this->mode = mode;
    pendingG = false;
    pendingOperator = 0;
    pendingObject = 0;
    count = 0;
    answer.clear();

    const std::string &line = Lines()[std::min(caret.line, Lines().size() - 1)];

    // Leaving the typing, the caret stands on the last character again, not
    // behind it, like in Vim
    if (mode != VimMode::Insert && caret.column >= line.size() && !line.empty()) {
        caret.column = line.size() - 1;
    }

    followCaret = true;
}

void CodeEditor::SetHiddenLines(std::function<bool(const std::string &line)> filter) {
    hiddenLine = std::move(filter);
}

int CodeEditor::GetScale() const {
    return scale;
}

void CodeEditor::SetScale(int scale) {
    this->scale = std::clamp(scale, MIN_SCALE, MAX_SCALE);

    // What was in view stays in view, the caret leads the way
    followCaret = true;
}

bool CodeEditor::IsOpen() const {
    return open;
}

bool CodeEditor::IsHovering() const {
    return open && hovering;
}

// The lines are what the editor works on, the text of a file is written from
// them whenever somebody asks for it
std::vector<CodeEditor::File> CodeEditor::Files() const {
    std::vector<File> result = files;

    for (std::size_t i = 0; i < result.size() && i < lines.size(); i++) {
        result[i].text = JoinLines(lines[i]);
    }

    return result;
}

std::vector<std::string> CodeEditor::SplitLines(const std::string &text) {
    std::vector<std::string> lines;
    std::string line;

    for (char letter: text) {
        if (letter == '\n') {
            lines.push_back(line);
            line.clear();
        } else if (letter != '\r') {
            line += letter;
        }
    }

    lines.push_back(line);

    return lines;
}

std::string CodeEditor::JoinLines(const std::vector<std::string> &lines) {
    std::string text;

    for (std::size_t i = 0; i < lines.size(); i++) {
        text += lines[i];

        if (i + 1 < lines.size()) {
            text += "\n";
        }
    }

    return text;
}

std::vector<std::string> &CodeEditor::Lines() {
    return lines[std::min(current, lines.size() - 1)];
}

const std::vector<std::string> &CodeEditor::Lines() const {
    return lines[std::min(current, lines.size() - 1)];
}

// Lines of the engine: what the filter picks, plus the empty line under a
// block that opens the file. Without that, the editor would start with an
// empty row where the engine's include used to be.
//
// Deliberately nothing else: every line the developer writes themselves
// stays where they put it, even right above the engine's lines.
std::vector<bool> CodeEditor::HiddenLines() const {
    const std::vector<std::string> &text = Lines();
    std::vector<bool> hidden(text.size(), false);

    if (!hiddenLine) {
        return hidden;
    }

    for (std::size_t line = 0; line < text.size(); line++) {
        hidden[line] = hiddenLine(text[line]);
    }

    if (!hidden.empty() && hidden[0]) {
        std::size_t at = 0;

        while (at < text.size()) {
            if (hidden[at]) {
                at++;
                continue;
            }

            if (!IsBlank(text[at])) {
                break;
            }

            std::size_t next = at;

            while (next < text.size() && IsBlank(text[next])) {
                next++;
            }

            // Empty lines between two lines of the engine belong to them
            if (next < text.size() && hidden[next]) {
                for (std::size_t line = at; line < next; line++) {
                    hidden[line] = true;
                }

                at = next;
                continue;
            }

            // One empty line closes the block, a second one is the
            // developer's own and stays
            hidden[at] = true;
            break;
        }
    }

    // The same at the end: what comes after the last line of the engine is
    // only the empty end of the file
    std::size_t last = text.size();

    while (last > 0 && IsBlank(text[last - 1])) {
        last--;
    }

    if (last > 0 && hidden[last - 1]) {
        for (std::size_t line = last; line < text.size(); line++) {
            hidden[line] = true;
        }
    }

    return hidden;
}

bool CodeEditor::IsHidden(std::size_t line) const {
    std::vector<bool> hidden = HiddenLines();

    return line < hidden.size() && hidden[line];
}

std::vector<std::size_t> CodeEditor::Rows() const {
    const std::vector<std::string> &text = Lines();
    std::vector<bool> hidden = HiddenLines();

    std::vector<std::size_t> rows;

    for (std::size_t line = 0; line < text.size(); line++) {
        if (!hidden[line]) {
            rows.push_back(line);
        }
    }

    // A file that is nothing but engine lines still shows one row
    if (rows.empty()) {
        rows.push_back(0);
    }

    return rows;
}

std::size_t CodeEditor::RowOf(std::size_t line) const {
    std::vector<std::size_t> rows = Rows();

    for (std::size_t row = 0; row < rows.size(); row++) {
        if (rows[row] >= line) {
            return row;
        }
    }

    return rows.size() - 1;
}

std::size_t CodeEditor::Visible(std::size_t line, int direction) const {
    std::vector<bool> hidden = HiddenLines();

    if (hidden.empty()) {
        return 0;
    }

    line = std::min(line, hidden.size() - 1);

    if (!hidden[line]) {
        return line;
    }

    // First in the direction of travel, then back: a file that ends with
    // lines of the engine still has a line for the caret
    for (int step: {direction >= 0 ? 1 : -1, direction >= 0 ? -1 : 1}) {
        auto at = static_cast<long>(line);

        while (at >= 0 && at < static_cast<long>(hidden.size())) {
            if (!hidden[static_cast<std::size_t>(at)]) {
                return static_cast<std::size_t>(at);
            }

            at += step;
        }
    }

    return line;
}

void CodeEditor::SettleCaret(int direction) {
    const std::vector<std::string> &text = Lines();

    caret.line = Visible(std::min(caret.line, text.size() - 1), direction);
    caret.column = std::min(caret.column, text[caret.line].size());
}

void CodeEditor::Changed() {
    files[std::min(current, files.size() - 1)].changed = true;

    followCaret = true;
}

void CodeEditor::Insert(const std::string &text) {
    std::vector<std::string> &current = Lines();

    std::string &line = current[std::min(caret.line, current.size() - 1)];

    caret.column = std::min(caret.column, line.size());

    line.insert(caret.column, text);
    caret.column += text.size();

    Changed();
}

// Enter keeps the indent of the line it was pressed in
void CodeEditor::NewLine() {
    std::vector<std::string> &current = Lines();

    std::size_t at = std::min(caret.line, current.size() - 1);
    std::string &line = current[at];

    caret.column = std::min(caret.column, line.size());

    std::string indent;

    for (char letter: line) {
        if (letter != ' ') {
            break;
        }

        indent += letter;
    }

    std::string rest = line.substr(caret.column);

    line.erase(caret.column);
    current.insert(current.begin() + static_cast<long>(at) + 1, indent + rest);

    caret.line = at + 1;
    caret.column = indent.size();

    Changed();
}

void CodeEditor::Backspace() {
    std::vector<std::string> &current = Lines();

    std::size_t at = std::min(caret.line, current.size() - 1);

    caret.column = std::min(caret.column, current[at].size());

    if (caret.column > 0) {
        current[at].erase(caret.column - 1, 1);
        caret.column--;
    } else if (at > 0) {
        // Never onto a line of the engine: what is folded away stays as it is
        if (IsHidden(at - 1)) {
            return;
        }

        // At the start of a line it hangs the line onto the one above
        caret.column = current[at - 1].size();
        current[at - 1] += current[at];
        current.erase(current.begin() + static_cast<long>(at));
        caret.line = at - 1;
    } else {
        return;
    }

    Changed();
}

void CodeEditor::DeleteAhead() {
    std::vector<std::string> &current = Lines();

    std::size_t at = std::min(caret.line, current.size() - 1);

    caret.column = std::min(caret.column, current[at].size());

    if (caret.column < current[at].size()) {
        current[at].erase(caret.column, 1);
    } else if (at + 1 < current.size()) {
        if (IsHidden(at + 1)) {
            return;
        }

        current[at] += current[at + 1];
        current.erase(current.begin() + static_cast<long>(at) + 1);
    } else {
        return;
    }

    Changed();
}

void CodeEditor::MoveCaret(int lines, int columns) {
    const std::vector<std::string> &text = Lines();

    followCaret = true;

    if (columns < 0 && caret.column == 0 && caret.line > 0) {
        // Left at the start of a line goes to the end of the one above
        caret.line = Visible(caret.line - 1, -1);
        caret.column = text[caret.line].size();
        return;
    }

    if (columns > 0 && caret.column >= text[std::min(caret.line, text.size() - 1)].size() &&
        caret.line + 1 < text.size()) {
        caret.line = Visible(caret.line + 1, 1);
        caret.column = 0;
        return;
    }

    int direction = lines < 0 ? -1 : 1;

    if (lines < 0) {
        caret.line = caret.line > static_cast<std::size_t>(-lines) ? caret.line + static_cast<std::size_t>(lines) : 0;
    } else if (lines > 0) {
        caret.line = std::min(caret.line + static_cast<std::size_t>(lines), text.size() - 1);
    }

    caret.line = Visible(caret.line, direction);

    std::size_t length = text[std::min(caret.line, text.size() - 1)].size();

    if (columns < 0) {
        caret.column = caret.column > 0 ? caret.column - 1 : 0;
    } else if (columns > 0) {
        caret.column = std::min(caret.column + 1, length);
    } else {
        caret.column = std::min(caret.column, length);
    }
}

void CodeEditor::MoveToLineStart() {
    caret.column = 0;

    followCaret = true;
}

void CodeEditor::MoveToLineEnd() {
    const std::vector<std::string> &text = Lines();

    caret.column = text[std::min(caret.line, text.size() - 1)].size();

    followCaret = true;
}

// To the start of the next word, over the end of the line if need be
void CodeEditor::WordForward() {
    const std::vector<std::string> &text = Lines();

    std::size_t line = std::min(caret.line, text.size() - 1);
    std::size_t column = std::min(caret.column, text[line].size());

    followCaret = true;

    column = NextWordStart(text[line], column);

    if (column < text[line].size()) {
        caret.column = column;
        return;
    }

    // At the end of the line the next one takes over, its first word
    std::size_t next = Visible(line + 1, 1);

    if (next == line || next >= text.size()) {
        caret.column = text[line].empty() ? 0 : text[line].size() - 1;
        return;
    }

    std::size_t first = text[next].find_first_not_of(' ');

    caret.line = next;
    caret.column = first == std::string::npos ? 0 : first;
}

void CodeEditor::WordBackward() {
    const std::vector<std::string> &text = Lines();

    std::size_t line = std::min(caret.line, text.size() - 1);
    std::size_t column = std::min(caret.column, text[line].size());

    followCaret = true;

    if (column == 0) {
        // At the start of the line the one above takes over, its last word
        std::size_t previous = line > 0 ? Visible(line - 1, -1) : line;

        if (previous == line) {
            return;
        }

        caret.line = previous;
        caret.column = text[previous].empty() ? 0 : text[previous].size() - 1;

        return;
    }

    column--;

    while (column > 0 && !IsWordCharacter(text[line][column])) {
        column--;
    }

    while (column > 0 && IsWordCharacter(text[line][column - 1])) {
        column--;
    }

    caret.column = column;
}

// o and O: an empty line with the indent of this one, and straight into it
void CodeEditor::OpenLine(bool below) {
    std::vector<std::string> &text = Lines();

    std::size_t at = std::min(caret.line, text.size() - 1);

    std::string indent;

    for (char letter: text[at]) {
        if (letter != ' ') {
            break;
        }

        indent += letter;
    }

    std::size_t index = below ? at + 1 : at;

    text.insert(text.begin() + static_cast<long>(index), indent);

    caret.line = index;
    caret.column = indent.size();

    SetMode(VimMode::Insert);
    Changed();
}

float CodeEditor::RowHeight(const FontRenderer &font) const {
    return static_cast<float>(font.LetterHeight() * scale) + EDITOR_LINE_GAP * static_cast<float>(scale);
}

float CodeEditor::Advance(const FontRenderer &font) const {
    return static_cast<float>(font.Advance(TextSpacing::Narrow) * scale);
}

// Not every character is as wide as the next, e.g. a comma takes less, see
// fontSpacing.json. So a place in a line is measured, not counted.
float CodeEditor::TextWidth(const std::string &line,
                            std::size_t from,
                            std::size_t to,
                            const FontRenderer &font) const {
    if (from >= to || from >= line.size()) {
        return 0.0f;
    }

    to = std::min(to, line.size());

    return static_cast<float>(font.Measure(line.substr(from, to - from), TextSpacing::Narrow, scale));
}

float CodeEditor::ChromeRow(const FontRenderer &font) const {
    return Chrome(static_cast<float>(font.LetterHeight()) + EDITOR_LINE_GAP);
}

float CodeEditor::Chrome(float pixels) const {
    return pixels * static_cast<float>(scale);
}

std::size_t CodeEditor::VisibleRows(const FontRenderer &font) const {
    return static_cast<std::size_t>(std::max(TextArea(font).height / RowHeight(font), 1.0f));
}

std::size_t CodeEditor::VisibleColumns(const FontRenderer &font) const {
    return static_cast<std::size_t>(std::max(TextArea(font).width / Advance(font), 1.0f));
}

std::size_t CodeEditor::LongestLine() const {
    std::size_t longest = 0;

    for (const std::string &line: Lines()) {
        longest = std::max(longest, line.size());
    }

    return longest;
}

void CodeEditor::Scroll(int rows) {
    const auto count = static_cast<int>(Rows().size());

    firstRow = static_cast<std::size_t>(std::clamp(static_cast<int>(firstRow) + rows, 0, std::max(count - 1, 0)));
}

void CodeEditor::ScrollSideways(int columns) {
    const auto longest = static_cast<int>(LongestLine());

    firstColumn = static_cast<std::size_t>(std::clamp(static_cast<int>(firstColumn) + columns, 0, std::max(longest, 0)));
}

void CodeEditor::Zoom(int steps) {
    SetScale(scale + steps);
}

Rectangle CodeEditor::Bounds() const {
    return area;
}

Rectangle CodeEditor::TextArea(const FontRenderer &font) const {
    Rectangle bounds = Bounds();

    float chrome = ChromeRow(font);
    float padding = Chrome(EDITOR_PADDING);
    float bar = Chrome(EDITOR_SCROLLBAR_WIDTH + EDITOR_SCROLLBAR_GAP);
    float numbers = static_cast<float>(EDITOR_NUMBER_WIDTH + 1) * Advance(font);

    return {
        bounds.x + padding + numbers,
        bounds.y + padding + chrome,
        bounds.width - 2.0f * padding - numbers - bar,
        bounds.height - 2.0f * padding - 2.0f * chrome - bar
    };
}

Rectangle CodeEditor::TabBounds(std::size_t file, const FontRenderer &font) const {
    Rectangle bounds = Bounds();

    float padding = Chrome(EDITOR_PADDING);
    float left = bounds.x + padding;

    // The star of a changed file belongs to its tab as well
    auto label = [this](std::size_t index) {
        return files[index].label + (files[index].changed ? "*" : "");
    };

    for (std::size_t i = 0; i < file && i < files.size(); i++) {
        left += Chrome(static_cast<float>(font.Measure(label(i), TextSpacing::Narrow)) + 2.0f * EDITOR_PADDING +
                       EDITOR_TAB_GAP);
    }

    float width = file < files.size()
                      ? Chrome(static_cast<float>(font.Measure(label(file), TextSpacing::Narrow)) +
                               2.0f * EDITOR_PADDING)
                      : 0.0f;

    return {left, bounds.y + padding, width, ChromeRow(font)};
}

Rectangle CodeEditor::ScrollbarBounds(const FontRenderer &font) const {
    Rectangle text = TextArea(font);

    return {text.x + text.width + Chrome(EDITOR_SCROLLBAR_GAP), text.y, Chrome(EDITOR_SCROLLBAR_WIDTH), text.height};
}

Rectangle CodeEditor::ScrollThumbBounds(const FontRenderer &font) const {
    Rectangle track = ScrollbarBounds(font);

    float total = static_cast<float>(Rows().size());
    float shown = static_cast<float>(VisibleRows(font));

    float height = std::max(track.height * std::min(shown / std::max(total, 1.0f), 1.0f), Chrome(4.0f));
    float steps = std::max(total - shown, 0.0f);
    float free = track.height - height;

    return {
        track.x,
        track.y + (steps > 0.0f ? free * static_cast<float>(firstRow) / steps : 0.0f),
        track.width,
        height
    };
}

// The sideways bar lies under the text, as wide as it
Rectangle CodeEditor::SideScrollbarBounds(const FontRenderer &font) const {
    Rectangle text = TextArea(font);

    return {text.x, text.y + text.height + Chrome(EDITOR_SCROLLBAR_GAP), text.width, Chrome(EDITOR_SCROLLBAR_WIDTH)};
}

Rectangle CodeEditor::SideScrollThumbBounds(const FontRenderer &font) const {
    Rectangle track = SideScrollbarBounds(font);

    float total = static_cast<float>(LongestLine());
    float shown = static_cast<float>(VisibleColumns(font));

    float width = std::max(track.width * std::min(shown / std::max(total, 1.0f), 1.0f), Chrome(4.0f));
    float steps = std::max(total - shown, 0.0f);
    float free = track.width - width;

    return {
        track.x + (steps > 0.0f ? free * static_cast<float>(firstColumn) / steps : 0.0f),
        track.y,
        width,
        track.height
    };
}

CodeEditor::Caret CodeEditor::CaretAt(Vector2 mouse, const FontRenderer &font) const {
    Rectangle text = TextArea(font);

    const std::vector<std::string> &lines = Lines();
    std::vector<std::size_t> rows = Rows();

    auto row = static_cast<std::size_t>(std::max((mouse.y - text.y) / RowHeight(font), 0.0f)) + firstRow;

    row = std::min(row, rows.size() - 1);

    std::size_t line = std::min(rows[row], lines.size() - 1);

    // The caret lands on the gap between two characters that is nearest to
    // the mouse, so a wide letter is easier to hit than a comma
    float wanted = std::max(mouse.x - text.x, 0.0f);
    std::size_t column = firstColumn;
    float left = 0.0f;

    while (column < lines[line].size()) {
        float width = TextWidth(lines[line], column, column + 1, font);

        if (left + width / 2.0f >= wanted) {
            break;
        }

        left += width;
        column++;
    }

    return {line, column};
}

void CodeEditor::UpdateScrollbar(Vector2 mouse, bool clicked, const FontRenderer &font) {
    Rectangle track = ScrollbarBounds(font);

    if (clicked && CheckCollisionPointRec(mouse, track)) {
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
    float steps = static_cast<float>(Rows().size()) - static_cast<float>(VisibleRows(font));

    if (free <= 0.0f || steps <= 0.0f) {
        return;
    }

    float share = (mouse.y - track.y - thumb / 2.0f) / free;

    firstRow = static_cast<std::size_t>(std::clamp(std::round(share * steps), 0.0f, steps));
}

void CodeEditor::UpdateSideScrollbar(Vector2 mouse, bool clicked, const FontRenderer &font) {
    Rectangle track = SideScrollbarBounds(font);

    if (clicked && CheckCollisionPointRec(mouse, track)) {
        draggingSideScroll = true;
    }

    if (draggingSideScroll && !IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        draggingSideScroll = false;
    }

    if (!draggingSideScroll) {
        return;
    }

    float thumb = SideScrollThumbBounds(font).width;
    float free = track.width - thumb;
    float steps = static_cast<float>(LongestLine()) - static_cast<float>(VisibleColumns(font));

    if (free <= 0.0f || steps <= 0.0f) {
        return;
    }

    float share = (mouse.x - track.x - thumb / 2.0f) / free;

    firstColumn = static_cast<std::size_t>(std::clamp(std::round(share * steps), 0.0f, steps));
}

// The movements of the normal mode, the visual modes and of an operator
// waiting for its motion. "gg" is the only one of two characters: the caller
// keeps the first "g" in pendingG and asks for the jump with the second.
bool CodeEditor::Move(int character, int count) {
    const std::vector<std::string> &text = Lines();

    const Caret before = caret;

    switch (character) {
        case 'h':
            MoveCaret(0, -1);
            break;

        case 'l':
            MoveCaret(0, 1);
            break;

        case 'j':
            MoveCaret(1, 0);
            break;

        case 'k':
            MoveCaret(-1, 0);
            break;

        case 'w':
            WordForward();
            break;

        case 'b':
            WordBackward();
            break;

        case 'e': {
            std::size_t line = std::min(caret.line, text.size() - 1);
            auto from = static_cast<long>(caret.column);

            // The end of the word, on this line or on one of the next
            for (;;) {
                std::size_t end = WordEnd(text[line], from);

                if (end != std::string::npos) {
                    caret.line = line;
                    caret.column = end;
                    break;
                }

                std::size_t next = Visible(line + 1, 1);

                if (next <= line) {
                    break;
                }

                line = next;
                from = -1;
            }

            followCaret = true;
            break;
        }

        case '0':
            MoveToLineStart();
            break;

        case '^': {
            std::size_t first = text[std::min(caret.line, text.size() - 1)].find_first_not_of(' ');

            caret.column = first == std::string::npos ? 0 : first;
            followCaret = true;
            break;
        }

        case '$':
            MoveToLineEnd();

            // In the normal mode the caret stands on the last character
            if (caret.column > 0) {
                caret.column--;
            }

            break;

        case 'g':
            caret.line = Visible(0, 1);
            caret.column = 0;
            followCaret = true;
            break;

        case 'G': {
            // A number in front goes to that row, without one to the last line
            std::vector<std::size_t> rows = Rows();

            caret.line = count > 0 ? rows[std::min(static_cast<std::size_t>(count), rows.size()) - 1]
                                   : Visible(text.size() - 1, -1);
            caret.column = 0;
            followCaret = true;
            break;
        }

        default:
            return false;
    }

    return caret.line != before.line || caret.column != before.column;
}

// The movements that a number in front repeats, the others ignore it
static bool Repeats(int character) {
    return character == 'h' || character == 'l' || character == 'j' || character == 'k' || character == 'w' ||
           character == 'b' || character == 'e';
}

// The commands of the normal mode. They are read as typed characters, not as
// keys: that way every keyboard layout works, and a quick tap is never lost,
// the same reason the engine reads its hotkeys from the key queue.
void CodeEditor::NormalMode(const FontRenderer &font) {
    // Control has the keys of the zoom and of the fast scrolling, and of redo
    if (ZoomModifier()) {
        if (Pressed(KEY_R)) {
            Redo();
        } else {
            FastScroll(font);
        }

        return;
    }

    // A command may have left the normal mode: the characters that are left
    // stay in the queue for the mode it went to
    while (mode == VimMode::Normal && !command.IsOpen()) {
        int character = GetCharPressed();

        if (character == 0) {
            break;
        }

        State before = Capture();

        NormalCharacter(character);
        ClampCaret();
        EndStep(before);
    }
}

void CodeEditor::NormalCharacter(int character) {
    const std::vector<std::string> &text = Lines();

    if (pendingG && character != 'g') {
        pendingG = false;
    }

    // A number in front of a command, "0" only continues one
    if ((character >= '1' && character <= '9') || (character == '0' && count > 0)) {
        count = std::min(count * 10 + (character - '0'), 9999);

        return;
    }

    const int typed = count;
    const int times = std::max(typed, 1);

    count = 0;

    // "iw" or "aw" after an operator
    if (pendingOperator != 0 && pendingObject != 0) {
        char op = pendingOperator;
        bool around = pendingObject == 'a';
        Range range;

        pendingOperator = 0;
        pendingObject = 0;

        if (character == 'w' && WordObject(around, range)) {
            Operate(op, range);
        }

        return;
    }

    // "d", "c" or "y" wait for what they work on
    if (pendingOperator != 0) {
        char op = pendingOperator;

        if (character == 'i' || character == 'a') {
            pendingObject = static_cast<char>(character);
            count = typed;

            return;
        }

        if (character == 'g' && !pendingG) {
            pendingG = true;
            count = typed;

            return;
        }

        pendingG = false;
        pendingOperator = 0;

        Range range;

        if (character == op) {
            // "dd", "cc" and "yy" take whole lines, this one and the next ones
            std::size_t last = std::min(caret.line, text.size() - 1);

            for (int i = 1; i < times; i++) {
                std::size_t next = Visible(last + 1, 1);

                if (next <= last) {
                    break;
                }

                last = next;
            }

            range.linewise = true;
            range.from = {caret.line, caret.column};
            range.to = {last, caret.column};

            Operate(op, range);
        } else if (MotionRange(character, typed, op == 'c', range)) {
            Operate(op, range);
        }

        return;
    }

    const std::size_t at = std::min(caret.line, text.size() - 1);
    const std::string &line = text[at];

    // Where the movement ends is where "x" and "X" work
    auto deleteHere = [&](std::size_t from, std::size_t to, bool change) {
        Range range;

        range.from = {at, from};
        range.to = {at, to};

        Operate(change ? 'c' : 'd', range);
    };

    switch (character) {
        case 'g':
            if (pendingG) {
                pendingG = false;
                Move('g', typed);
            } else {
                pendingG = true;
            }

            break;

        case 'i':
            SetMode(VimMode::Insert);
            break;

        case 'a':
            SetMode(VimMode::Insert);
            caret.column = std::min(caret.column + 1, line.size());
            break;

        case 'I': {
            std::size_t first = line.find_first_not_of(' ');

            SetMode(VimMode::Insert);
            caret.column = first == std::string::npos ? 0 : first;
            break;
        }

        case 'A':
            SetMode(VimMode::Insert);
            MoveToLineEnd();
            break;

        case 'o':
            OpenLine(true);
            break;

        case 'O':
            OpenLine(false);
            break;

        case 'v':
            anchor = caret;
            SetMode(VimMode::Visual);
            break;

        case 'V':
            anchor = caret;
            SetMode(VimMode::VisualLine);
            break;

        case 'd':
        case 'c':
        case 'y':
            pendingOperator = static_cast<char>(character);
            count = typed;
            break;

        case 'D':
        case 'C': {
            // To the end of the line
            deleteHere(std::min(caret.column, line.size()), line.size(), character == 'C');
            break;
        }

        case 'Y': {
            Range range;

            range.linewise = true;
            range.from = {at, caret.column};
            range.to = {at, caret.column};

            Operate('y', range);
            break;
        }

        case 'x':
            deleteHere(std::min(caret.column, line.size()), std::min(caret.column + times, line.size()), false);
            break;

        case 'X':
            deleteHere(caret.column - std::min(static_cast<std::size_t>(times), caret.column), caret.column, false);
            break;

        case 's':
            deleteHere(std::min(caret.column, line.size()), std::min(caret.column + times, line.size()), true);
            break;

        case 'S': {
            Range range;

            range.linewise = true;
            range.from = {at, caret.column};
            range.to = {at, caret.column};

            Operate('c', range);
            break;
        }

        case 'p':
        case 'P': {
            std::vector<std::string> pieces;
            bool linewise = false;

            if (PasteSource(pieces, linewise)) {
                PutText(character == 'p', pieces, linewise);
            }

            break;
        }

        case 'u':
            for (int i = 0; i < times; i++) {
                Undo();
            }

            break;

        case Console::OPEN_CHARACTER:
            answer.clear();
            command.Open();
            break;

        default:
            for (int i = 0; i < (Repeats(character) ? times : 1); i++) {
                if (!Move(character, typed)) {
                    break;
                }
            }

            break;
    }
}

// The commands of the visual modes: the same movements as in the normal
// mode, and d, c and y for what is selected
void CodeEditor::VisualMode(const FontRenderer &font) {
    if (ZoomModifier()) {
        FastScroll(font);

        return;
    }

    while (IsVisual() && !command.IsOpen()) {
        int character = GetCharPressed();

        if (character == 0) {
            break;
        }

        State before = Capture();

        VisualCharacter(character);
        ClampCaret();
        EndStep(before);
    }
}

void CodeEditor::VisualCharacter(int character) {
    if (pendingG && character != 'g') {
        pendingG = false;
    }

    if ((character >= '1' && character <= '9') || (character == '0' && count > 0)) {
        count = std::min(count * 10 + (character - '0'), 9999);

        return;
    }

    const int typed = count;
    const int times = std::max(typed, 1);

    count = 0;

    Range selection = Selection();
    Range lines = selection;
    bool finished = false;

    lines.linewise = true;

    switch (character) {
        case 'v':
            // Again leaves it, from the other visual mode it switches
            if (mode == VimMode::Visual) {
                SetMode(VimMode::Normal);
            } else {
                mode = VimMode::Visual;
            }

            break;

        case 'V':
            if (mode == VimMode::VisualLine) {
                SetMode(VimMode::Normal);
            } else {
                mode = VimMode::VisualLine;
            }

            break;

        case 'o': {
            // The other end of the selection is the one that moves now
            std::swap(anchor, caret);
            followCaret = true;
            break;
        }

        case 'd':
        case 'x':
            Operate('d', selection);
            finished = true;
            break;

        case 'D':
        case 'X':
            Operate('d', lines);
            finished = true;
            break;

        case 'y':
            Operate('y', selection);
            finished = true;
            break;

        case 'Y':
            Operate('y', lines);
            finished = true;
            break;

        case 'c':
        case 's':
            Operate('c', selection);
            finished = true;
            break;

        case 'C':
        case 'S':
        case 'R':
            Operate('c', lines);
            finished = true;
            break;

        case 'p':
        case 'P': {
            // Replaces the selection. What was in the register is what
            // lands in the text, the selection takes its place.
            std::vector<std::string> pieces;
            bool linewise = false;

            if (!PasteSource(pieces, linewise)) {
                break;
            }

            const std::vector<std::string> &text = Lines();
            std::size_t firstLine = selection.from.line;

            // Behind the caret when the selection reached the end of the line
            bool after = !selection.linewise &&
                         selection.to.column >= text[std::min(selection.to.line, text.size() - 1)].size();

            Operate('d', selection);

            if (selection.linewise) {
                after = caret.line < firstLine;
            }

            PutText(after, pieces, linewise);
            finished = true;
            break;
        }

        case Console::OPEN_CHARACTER:
            answer.clear();
            command.Open();
            break;

        case 'g':
            if (pendingG) {
                pendingG = false;
                Move('g', typed);
            } else {
                pendingG = true;
            }

            break;

        default:
            for (int i = 0; i < (Repeats(character) ? times : 1); i++) {
                if (!Move(character, typed)) {
                    break;
                }
            }

            break;
    }

    // What was done with the selection ends the visual mode, unless the
    // command already went to the insert mode
    if (finished && mode != VimMode::Insert) {
        SetMode(VimMode::Normal);
    }
}

CodeEditor::State CodeEditor::Capture() const {
    return State{current, Lines(), caret};
}

void CodeEditor::EndStep(const State &before) {
    if (historyMove) {
        historyMove = false;

        return;
    }

    if (mode == VimMode::Insert) {
        // Typing goes on, see SetMode. A command that only switched to the
        // insert mode, e.g. "i", may not change anything at all.
        if (!sessionStart) {
            sessionStart = before;
        }

        return;
    }

    if (before.file < lines.size() && lines[before.file] != before.lines) {
        Record(before);
    }
}

void CodeEditor::Record(State before) {
    if (before.file >= history.size()) {
        return;
    }

    History &entry = history[before.file];

    entry.redo.clear();
    entry.undo.push_back(std::move(before));

    if (entry.undo.size() > MAX_HISTORY) {
        entry.undo.erase(entry.undo.begin());
    }
}

void CodeEditor::Restore(const State &state) {
    lines[state.file] = state.lines;
    files[state.file].changed = true;

    caret = state.caret;

    SettleCaret(1);
    ClampCaret();

    followCaret = true;
    historyMove = true;
}

void CodeEditor::Undo() {
    History &entry = history[std::min(current, history.size() - 1)];

    if (entry.undo.empty()) {
        answer = "Already at the oldest change";

        return;
    }

    State now = Capture();
    State state = std::move(entry.undo.back());

    entry.undo.pop_back();
    entry.redo.push_back(std::move(now));

    Restore(state);
}

void CodeEditor::Redo() {
    History &entry = history[std::min(current, history.size() - 1)];

    if (entry.redo.empty()) {
        answer = "Already at the newest change";

        return;
    }

    State now = Capture();
    State state = std::move(entry.redo.back());

    entry.redo.pop_back();
    entry.undo.push_back(std::move(now));

    Restore(state);
}

// What one of d, c and y does with a range. The deleted text lands in the
// register as well, so d and c cut and y copies.
void CodeEditor::Operate(char op, Range range) {
    std::vector<std::string> &text = Lines();

    if (op == 'y') {
        Yank(range);

        // The caret goes to where the copied text starts
        caret.line = Visible(range.from.line, 1);
        caret.column = range.linewise ? std::min(caret.column, text[caret.line].size()) : range.from.column;
        followCaret = true;

        return;
    }

    Yank(range);

    if (range.linewise && op == 'c') {
        // The lines are replaced by one empty line with the indent of the first
        std::size_t first = std::string::npos;

        for (std::size_t line = range.from.line; line <= range.to.line && line < text.size(); line++) {
            if (!IsHidden(line)) {
                first = line;
                break;
            }
        }

        if (first == std::string::npos) {
            return;
        }

        std::string indent = text[first].substr(0, std::min(text[first].find_first_not_of(' '), text[first].size()));

        for (std::size_t line = std::min(range.to.line, text.size() - 1); line > first; line--) {
            if (!IsHidden(line)) {
                text.erase(text.begin() + static_cast<long>(line));
            }
        }

        text[first] = indent;
        caret.line = first;
        caret.column = indent.size();

        SetMode(VimMode::Insert);
        Changed();

        return;
    }

    if (!Erase(range)) {
        answer = "Lines of the engine stay as they are";

        return;
    }

    if (op == 'c') {
        SetMode(VimMode::Insert);
    } else {
        ClampCaret();
    }
}

std::vector<std::string> CodeEditor::RangeText(const Range &range) const {
    const std::vector<std::string> &text = Lines();

    std::vector<std::string> result;

    if (text.empty()) {
        return result;
    }

    std::size_t last = std::min(range.to.line, text.size() - 1);

    if (range.linewise) {
        for (std::size_t line = range.from.line; line <= last; line++) {
            if (!IsHidden(line)) {
                result.push_back(text[line]);
            }
        }

        return result;
    }

    std::size_t first = std::min(range.from.line, last);

    for (std::size_t line = first; line <= last; line++) {
        std::size_t from = line == first ? std::min(range.from.column, text[line].size()) : 0;
        std::size_t to = line == last ? std::min(range.to.column, text[line].size()) : text[line].size();

        result.push_back(from < to ? text[line].substr(from, to - from) : std::string());
    }

    return result;
}

void CodeEditor::Yank(const Range &range) {
    registerText = RangeText(range);
    registerLinewise = range.linewise;

    std::string joined = JoinLines(registerText);

    if (range.linewise) {
        joined += "\n";
    }

    registerClipboard = joined;

    SetClipboardText(joined.c_str());
}

bool CodeEditor::Erase(const Range &range) {
    std::vector<std::string> &text = Lines();

    std::size_t last = std::min(range.to.line, text.size() - 1);
    std::size_t first = std::min(range.from.line, last);

    if (range.linewise) {
        bool any = false;

        for (std::size_t line = last + 1; line-- > first;) {
            if (IsHidden(line)) {
                continue;
            }

            text.erase(text.begin() + static_cast<long>(line));
            any = true;
        }

        if (!any) {
            return false;
        }

        if (text.empty()) {
            text.emplace_back();
        }

        caret.line = Visible(std::min(first, text.size() - 1), 1);

        std::size_t indent = text[caret.line].find_first_not_of(' ');

        caret.column = indent == std::string::npos ? 0 : indent;

        Changed();

        return true;
    }

    // A piece across lines would eat into a line of the engine
    for (std::size_t line = first + 1; line <= last; line++) {
        if (IsHidden(line)) {
            return false;
        }
    }

    std::size_t from = std::min(range.from.column, text[first].size());
    std::size_t to = std::min(range.to.column, text[last].size());

    if (first == last) {
        if (from >= to) {
            caret = {first, from};

            return true;
        }

        text[first].erase(from, to - from);
    } else {
        text[first] = text[first].substr(0, from) + text[last].substr(to);
        text.erase(text.begin() + static_cast<long>(first) + 1, text.begin() + static_cast<long>(last) + 1);
    }

    caret = {first, from};

    Changed();

    return true;
}

bool CodeEditor::PasteSource(std::vector<std::string> &pieces, bool &linewise) const {
    const char *clipboard = GetClipboardText();
    std::string outside = clipboard != nullptr ? clipboard : "";

    // Something copied in another program wins over the register
    if (!outside.empty() && outside != registerClipboard) {
        linewise = outside.back() == '\n';

        if (linewise) {
            outside.pop_back();
        }

        pieces = SplitLines(outside);

        return true;
    }

    pieces = registerText;
    linewise = registerLinewise;

    return !pieces.empty();
}

void CodeEditor::PutText(bool after, const std::vector<std::string> &pieces, bool linewise) {
    std::vector<std::string> &text = Lines();

    if (pieces.empty()) {
        return;
    }

    std::size_t at = std::min(caret.line, text.size() - 1);

    if (linewise) {
        std::size_t index = after ? at + 1 : at;

        text.insert(text.begin() + static_cast<long>(index), pieces.begin(), pieces.end());

        caret.line = index;

        std::size_t indent = text[index].find_first_not_of(' ');

        caret.column = indent == std::string::npos ? 0 : indent;

        Changed();

        return;
    }

    std::string &line = text[at];

    std::size_t column = std::min(caret.column, line.size());

    // Behind the character the caret stands on
    if (after && !line.empty()) {
        column = std::min(column + 1, line.size());
    }

    if (pieces.size() == 1) {
        line.insert(column, pieces[0]);

        // On the last character that was pasted, like in Vim
        caret.column = pieces[0].empty() ? column : column + pieces[0].size() - 1;
    } else {
        std::string tail = line.substr(column);

        line = line.substr(0, column) + pieces.front();

        std::vector<std::string> inserted(pieces.begin() + 1, pieces.end());

        inserted.back() += tail;

        text.insert(text.begin() + static_cast<long>(at) + 1, inserted.begin(), inserted.end());

        caret.column = column;
    }

    Changed();
}

void CodeEditor::ClampCaret() {
    // The caret of the insert mode may stand behind the last character
    if (vim && mode == VimMode::Insert) {
        return;
    }

    const std::vector<std::string> &text = Lines();

    caret.line = std::min(caret.line, text.size() - 1);

    std::size_t size = text[caret.line].size();

    if (caret.column >= size) {
        caret.column = size > 0 ? size - 1 : 0;
    }
}

// "iw" is the word under the caret, "aw" takes the spaces behind it too, or
// in front of it when there are none behind
bool CodeEditor::WordObject(bool around, Range &range) const {
    const std::vector<std::string> &text = Lines();

    std::size_t at = std::min(caret.line, text.size() - 1);
    const std::string &line = text[at];

    if (line.empty()) {
        return false;
    }

    std::size_t from = std::min(caret.column, line.size() - 1);
    std::size_t to = from + 1;
    int kind = CharacterClass(line[from]);

    while (from > 0 && CharacterClass(line[from - 1]) == kind) {
        from--;
    }

    while (to < line.size() && CharacterClass(line[to]) == kind) {
        to++;
    }

    if (around && kind != 0) {
        std::size_t spaces = to;

        while (spaces < line.size() && CharacterClass(line[spaces]) == 0) {
            spaces++;
        }

        if (spaces > to) {
            to = spaces;
        } else {
            while (from > 0 && CharacterClass(line[from - 1]) == 0) {
                from--;
            }
        }
    }

    range.linewise = false;
    range.from = {at, from};
    range.to = {at, to};

    return true;
}

// Where an operator like d reaches with a motion. The caret does not move:
// only the range comes back.
bool CodeEditor::MotionRange(int character, int count, bool change, Range &range) {
    const std::vector<std::string> &text = Lines();

    const Caret start = caret;
    const std::size_t at = std::min(start.line, text.size() - 1);
    const std::string &line = text[at];
    const int times = std::max(count, 1);

    range.linewise = false;
    range.from = {at, std::min(start.column, line.size())};
    range.to = range.from;

    const std::size_t column = range.from.column;

    switch (character) {
        case 'j':
        case 'k':
        case 'g':
        case 'G': {
            // Whole lines, from here to where the movement ends
            for (int i = 0; i < (character == 'j' || character == 'k' ? times : 1); i++) {
                if (!Move(character, count)) {
                    break;
                }
            }

            range.linewise = true;
            range.from = {std::min(start.line, caret.line), column};
            range.to = {std::max(start.line, caret.line), column};

            caret = start;

            return true;
        }

        case 'w': {
            std::size_t end = column;

            if (change && column < line.size() && CharacterClass(line[column]) != 0) {
                // "cw" changes up to the end of the word, not up to the next
                for (int i = 0; i < times; i++) {
                    std::size_t last = WordEnd(line, i == 0 ? static_cast<long>(column) - 1 : static_cast<long>(end) - 1);

                    if (last == std::string::npos) {
                        end = line.size();
                        break;
                    }

                    // Only the word the caret stands in ends where it ends
                    end = last + 1;
                }
            } else {
                for (int i = 0; i < times; i++) {
                    end = NextWordStart(line, end);
                }
            }

            range.to.column = end;

            return true;
        }

        case 'e': {
            std::size_t end = column;

            for (int i = 0; i < times; i++) {
                std::size_t last = WordEnd(line, static_cast<long>(end == column && i == 0 ? column : end - 1));

                if (last == std::string::npos) {
                    end = line.size();
                    break;
                }

                end = last + 1;
            }

            range.to.column = end;

            return true;
        }

        case 'b': {
            for (int i = 0; i < times; i++) {
                if (!Move('b', count)) {
                    break;
                }
            }

            range.from = caret;
            range.to = start;
            range.to.column = column;

            caret = start;

            return true;
        }

        case 'h':
            range.from.column = column - std::min(static_cast<std::size_t>(times), column);
            range.to.column = column;

            return true;

        case 'l':
            range.to.column = std::min(column + static_cast<std::size_t>(times), line.size());

            return true;

        case '0':
            range.from.column = 0;
            range.to.column = column;

            return true;

        case '^': {
            std::size_t first = line.find_first_not_of(' ');

            first = first == std::string::npos ? line.size() : first;

            range.from.column = std::min(first, column);
            range.to.column = std::max(first, column);

            return true;
        }

        case '$':
            range.to.column = line.size();

            return true;

        default:
            return false;
    }
}

CodeEditor::Range CodeEditor::Selection() const {
    const std::vector<std::string> &text = Lines();

    Caret first = anchor;
    Caret last = caret;

    first.line = std::min(first.line, text.size() - 1);
    last.line = std::min(last.line, text.size() - 1);

    if (last.line < first.line || (last.line == first.line && last.column < first.column)) {
        std::swap(first, last);
    }

    Range range;

    range.linewise = mode == VimMode::VisualLine;
    range.from = first;
    range.to = last;

    // The character under the caret belongs to the selection
    if (!range.linewise) {
        range.to.column = std::min(last.column + 1, text[last.line].size());
    }

    return range;
}

bool CodeEditor::IsVisual() const {
    return mode == VimMode::Visual || mode == VimMode::VisualLine;
}

bool CodeEditor::SelectedColumns(std::size_t line, std::size_t &from, std::size_t &to, bool &lineBreak) const {
    if (!vim || !IsVisual()) {
        return false;
    }

    const std::vector<std::string> &text = Lines();

    Range range = Selection();

    if (line < range.from.line || line > range.to.line || line >= text.size()) {
        return false;
    }

    if (range.linewise) {
        from = 0;
        to = text[line].size();
        lineBreak = true;

        return true;
    }

    from = line == range.from.line ? range.from.column : 0;
    to = line == range.to.line ? range.to.column : text[line].size();

    // The line break is part of it when the selection goes on in the next
    // line, and an empty line would show nothing without it
    lineBreak = line < range.to.line || text[line].empty();

    return true;
}

// Control and d or u jump half a screen, f and b a whole one. The caret goes
// along, so the view follows it, exactly like in Vim.
bool CodeEditor::FastScroll(const FontRenderer &font) {
    const auto screen = static_cast<int>(VisibleRows(font));
    const int half = std::max(screen / 2, 1);

    if (Pressed(KEY_D)) {
        MoveCaret(half, 0);
        return true;
    }

    if (Pressed(KEY_U)) {
        MoveCaret(-half, 0);
        return true;
    }

    if (Pressed(KEY_F)) {
        MoveCaret(screen, 0);
        return true;
    }

    if (Pressed(KEY_B)) {
        MoveCaret(-screen, 0);
        return true;
    }

    return false;
}

// Everything that types. Without the Vim bindings this is the whole editor.
void CodeEditor::InsertMode() {
    const bool zooming = ZoomModifier();

    // Characters instead of keys, so every layout works. While zooming the
    // keys belong to the zoom, nothing of them lands in the code.
    for (int character = GetCharPressed(); character != 0; character = GetCharPressed()) {
        if (!zooming && character >= ' ' && character < 127) {
            Insert(std::string(1, static_cast<char>(character)));
        }
    }

    if (zooming) {
        return;
    }

    if (Pressed(KEY_ENTER) || Pressed(KEY_KP_ENTER)) {
        NewLine();
    }

    if (Pressed(KEY_TAB)) {
        Insert(std::string(INDENT, ' '));
    }

    if (Pressed(KEY_BACKSPACE)) {
        Backspace();
    }

    if (Pressed(KEY_DELETE)) {
        DeleteAhead();
    }
}

CodeEditor::Request CodeEditor::Update(Vector2 mouse, bool clicked, const FontRenderer &font) {
    if (!open) {
        return Request::None;
    }

    request = Request::None;
    hovering = CheckCollisionPointRec(mouse, Bounds());

    // The tabs switch between the files of the script
    for (std::size_t i = 0; i < files.size(); i++) {
        if (clicked && CheckCollisionPointRec(mouse, TabBounds(i, font))) {
            current = std::min(i, lines.size() - 1);
            caret = Caret{};
            firstRow = 0;
            firstColumn = 0;

            SettleCaret(1);
        }
    }

    const bool zooming = ZoomModifier();

    // Control and the wheel zoom, like the piano roll of the composer
    float wheel = GetMouseWheelMove();

    if (wheel != 0.0f && hovering) {
        if (zooming) {
            Zoom(wheel > 0.0f ? 1 : -1);
        } else {
            Scroll(wheel > 0.0f ? -3 : 3);
        }
    }

    // Control and + or -, on the number pad as well. On a German keyboard the
    // + sits where the US layout has ], and the - where it has /.
    if (zooming) {
        if (Pressed(KEY_EQUAL) || Pressed(KEY_KP_ADD) || Pressed(KEY_RIGHT_BRACKET)) {
            Zoom(1);
        }

        if (Pressed(KEY_MINUS) || Pressed(KEY_KP_SUBTRACT) || Pressed(KEY_SLASH)) {
            Zoom(-1);
        }

        // Back to the size of the game's own font
        if (Pressed(KEY_ZERO) || Pressed(KEY_KP_0)) {
            SetScale(pixelScale);
        }
    }

    UpdateScrollbar(mouse, clicked, font);
    UpdateSideScrollbar(mouse, clicked, font);

    if (clicked && CheckCollisionPointRec(mouse, TextArea(font))) {
        caret = CaretAt(mouse, font);
        followCaret = true;
    }

    // Control and c leave the mode like ESC does. Only with the Vim bindings:
    // without them ESC saves and closes, which Control and c must not do.
    if (vim && (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_C)) {
        OnEscape();

        return request;
    }

    // The command line has the keyboard to itself while it is open
    if (command.IsOpen()) {
        std::string line = command.Update();

        if (!line.empty()) {
            RunCommand(line);
        }

        return request;
    }

    if (vim && mode == VimMode::Normal) {
        NormalMode(font);
    } else if (vim && IsVisual()) {
        VisualMode(font);
    } else {
        InsertMode();
    }

    // The arrow keys work in both modes, like in Vim
    if (Pressed(KEY_LEFT)) {
        MoveCaret(0, -1);
    }

    if (Pressed(KEY_RIGHT)) {
        MoveCaret(0, 1);
    }

    if (Pressed(KEY_UP)) {
        MoveCaret(-1, 0);
    }

    if (Pressed(KEY_DOWN)) {
        MoveCaret(1, 0);
    }

    if (IsKeyPressed(KEY_HOME)) {
        MoveToLineStart();
    }

    if (IsKeyPressed(KEY_END)) {
        MoveToLineEnd();
    }

    if (Pressed(KEY_PAGE_UP)) {
        MoveCaret(-static_cast<int>(VisibleRows(font)), 0);
    }

    if (Pressed(KEY_PAGE_DOWN)) {
        MoveCaret(static_cast<int>(VisibleRows(font)), 0);
    }

    // A caret that just moved pulls the view along. Scrolling on its own
    // leaves it where it is, otherwise the view would jump back immediately.
    if (followCaret) {
        std::size_t rows = VisibleRows(font);
        std::size_t columns = VisibleColumns(font);
        std::size_t row = RowOf(caret.line);

        if (row < firstRow) {
            firstRow = row;
        } else if (row >= firstRow + rows) {
            firstRow = row - rows + 1;
        }

        if (caret.column < firstColumn) {
            firstColumn = caret.column;
        } else if (caret.column >= firstColumn + columns) {
            firstColumn = caret.column - columns + 1;
        }

        followCaret = false;
    }

    return request;
}

// Colors the line the way C++ reads: comments, texts, numbers and keywords
void CodeEditor::DrawLine(const std::string &line, Vector2 position, const FontRenderer &font) const {
    // Where the next character goes, from the first one that is in view
    float cursor = 0.0f;

    std::size_t first = line.find_first_not_of(' ');
    bool preprocessor = first != std::string::npos && line[first] == '#';

    std::size_t at = 0;
    std::size_t column = 0;

    while (at < line.size()) {
        std::size_t length = 1;
        int variant = EDITOR_VARIANT_TEXT;

        if (preprocessor) {
            length = line.size() - at;
            variant = EDITOR_VARIANT_PREPROCESSOR;
        } else if (line.compare(at, 2, "//") == 0) {
            length = line.size() - at;
            variant = EDITOR_VARIANT_COMMENT;
        } else if (line[at] == '"' || line[at] == '\'') {
            char quote = line[at];
            std::size_t end = at + 1;

            while (end < line.size() && line[end] != quote) {
                end += line[end] == '\\' ? 2 : 1;
            }

            length = std::min(end + 1, line.size()) - at;
            variant = EDITOR_VARIANT_STRING;
        } else if (std::isdigit(static_cast<unsigned char>(line[at]))) {
            std::size_t end = at;

            while (end < line.size() && (std::isalnum(static_cast<unsigned char>(line[end])) || line[end] == '.')) {
                end++;
            }

            length = end - at;
            variant = EDITOR_VARIANT_NUMBER_LITERAL;
        } else if (IsWordCharacter(line[at])) {
            std::size_t end = at;

            while (end < line.size() && IsWordCharacter(line[end])) {
                end++;
            }

            length = end - at;
            variant = IsKeyword(line.substr(at, length)) ? EDITOR_VARIANT_KEYWORD : EDITOR_VARIANT_TEXT;
        }

        // Only what is inside the view is drawn, character by character: each
        // one takes the room its own width asks for
        for (std::size_t i = 0; i < length; i++) {
            std::size_t index = column + i;

            if (index < firstColumn) {
                continue;
            }

            char letter = line[at + i];
            float width = TextWidth(line, index, index + 1, font);
            float x = position.x + cursor;

            // A letter that no longer fits whole is left out: nothing sticks
            // out into the scrollbar
            if (x + width > TextArea(font).x + TextArea(font).width) {
                break;
            }

            if (letter != ' ') {
                font.Draw(std::string(1, letter), {x, position.y}, variant, TextSpacing::Narrow, scale);
            }

            cursor += width;
        }

        at += length;
        column += length;
    }
}

void CodeEditor::Draw(const FontRenderer &font) const {
    if (!open) {
        return;
    }

    DrawRectangle(0, 0, viewWidth, viewHeight, EDITOR_SHADE);

    Rectangle bounds = Bounds();

    DrawRectangleRec(bounds, EDITOR_BACKGROUND);
    DrawRectangleLinesEx(bounds, Chrome(1.0f), EDITOR_BORDER);

    // The files of the script as tabs
    for (std::size_t i = 0; i < files.size(); i++) {
        Rectangle tab = TabBounds(i, font);

        if (i == current) {
            DrawRectangleRec(tab, EDITOR_TAB);
        }

        font.Draw(
            files[i].label + (files[i].changed ? "*" : ""),
            {tab.x + Chrome(EDITOR_PADDING), tab.y + Chrome(1.0f)},
            i == current ? EDITOR_VARIANT_TAB_OPEN : EDITOR_VARIANT_TAB,
            TextSpacing::Narrow,
            scale
        );
    }

    Rectangle text = TextArea(font);

    float row = RowHeight(font);
    float advance = Advance(font);

    const std::vector<std::string> &lines = Lines();
    std::vector<std::size_t> rows = Rows();
    std::size_t shown = VisibleRows(font);

    for (std::size_t i = 0; i < shown && firstRow + i < rows.size(); i++) {
        std::size_t line = rows[firstRow + i];
        float y = text.y + static_cast<float>(i) * row;

        if (line == caret.line) {
            DrawRectangle(
                static_cast<int>(text.x - advance),
                static_cast<int>(y - 1.0f),
                static_cast<int>(text.width + advance),
                static_cast<int>(row),
                EDITOR_CARET_LINE
            );
        }

        std::size_t selectedFrom = 0;
        std::size_t selectedTo = 0;
        bool selectedBreak = false;

        if (SelectedColumns(line, selectedFrom, selectedTo, selectedBreak)) {
            float startX = TextWidth(lines[line], firstColumn, selectedFrom, font);
            float endX = TextWidth(lines[line], firstColumn, selectedTo, font) + (selectedBreak ? advance : 0.0f);

            endX = std::min(endX, text.width);

            if (endX > startX) {
                DrawRectangle(
                    static_cast<int>(text.x + startX),
                    static_cast<int>(y - 1.0f),
                    static_cast<int>(endX - startX),
                    static_cast<int>(row),
                    EDITOR_SELECTION
                );
            }
        }

        // The numbers count the rows, not the lines of the file: what is
        // hidden does not exist for whoever writes the script
        std::string number = std::to_string(firstRow + i + 1);
        float numberX = text.x - Chrome(EDITOR_PADDING) -
                        static_cast<float>(font.Measure(number, TextSpacing::Narrow, scale));

        font.Draw(number, {numberX, y}, EDITOR_VARIANT_NUMBER, TextSpacing::Narrow, scale);

        DrawLine(lines[line], {text.x, y}, font);
    }

    // The caret sits where the next character lands. In the normal mode it
    // stands on a character instead, as a block, like in Vim.
    std::size_t caretRow = RowOf(caret.line);
    bool block = vim && mode != VimMode::Insert && !command.IsOpen();

    if (caretRow >= firstRow && caretRow < firstRow + shown && caret.column >= firstColumn) {
        const std::string &line = lines[std::min(caret.line, lines.size() - 1)];

        float caretX = text.x + TextWidth(line, firstColumn, caret.column, font);
        float caretY = text.y + static_cast<float>(caretRow - firstRow) * row;

        // The bar keeps a gap to the character in front of it, the block
        // covers a character and stays where it is
        if (!block && caret.column > firstColumn) {
            caretX += static_cast<float>(font.CaretSpace() * scale);
        }

        // The block of the normal mode is as wide as the character it stands
        // on, the bar between two characters stays thin
        float width = TextWidth(line, caret.column, caret.column + 1, font);

        if (width <= 0.0f) {
            width = advance;
        }

        // One pixel wider than the character, so the block reaches over its
        // right edge
        width += static_cast<float>(scale);

        if (caretX <= text.x + text.width) {
            DrawRectangle(
                static_cast<int>(caretX),
                static_cast<int>(caretY),
                block ? static_cast<int>(width) : scale,
                font.LetterHeight() * scale,
                block ? EDITOR_CARET_BLOCK : EDITOR_CARET
            );
        }
    }

    // Both scrollbars only show up when there is more than fits
    if (rows.size() > shown) {
        DrawRectangleRec(ScrollbarBounds(font), EDITOR_SCROLL_TRACK);
        DrawRectangleRec(ScrollThumbBounds(font), draggingScroll ? EDITOR_SCROLL_THUMB_HELD : EDITOR_SCROLL_THUMB);
    }

    if (LongestLine() > VisibleColumns(font)) {
        DrawRectangleRec(SideScrollbarBounds(font), EDITOR_SCROLL_TRACK);
        DrawRectangleRec(
            SideScrollThumbBounds(font),
            draggingSideScroll ? EDITOR_SCROLL_THUMB_HELD : EDITOR_SCROLL_THUMB
        );
    }

    // The bottom row: the command line while it is open, otherwise the mode,
    // where the caret is and how far the code is zoomed
    Vector2 status{
        bounds.x + Chrome(EDITOR_PADDING),
        bounds.y + bounds.height - Chrome(EDITOR_PADDING + static_cast<float>(font.LetterHeight()))
    };

    if (command.IsOpen()) {
        std::string line = std::string(1, static_cast<char>(Console::OPEN_CHARACTER)) + command.Input();

        font.Draw(line, status, EDITOR_VARIANT_COMMAND, TextSpacing::Narrow, scale);

        // Behind the character the next one would land on
        float caretX = status.x + static_cast<float>(font.Measure(
            line.substr(0, command.CaretPosition() + 1),
            TextSpacing::Narrow,
            scale
        ));

        caretX += static_cast<float>(font.CaretSpace() * scale);

        DrawRectangle(
            static_cast<int>(caretX),
            static_cast<int>(status.y),
            scale,
            font.LetterHeight() * scale,
            EDITOR_CARET
        );

        return;
    }

    std::string hint;

    if (!answer.empty()) {
        hint = answer + "   ";
    } else if (vim) {
        switch (mode) {
            case VimMode::Insert:
                hint = "-- Insert --   ";
                break;

            case VimMode::Visual:
                hint = "-- Visual --   ";
                break;

            case VimMode::VisualLine:
                hint = "-- Visual Line --   ";
                break;

            default:
                hint = "-- Normal --   ";
                break;
        }
    }

    hint += "Line " + std::to_string(RowOf(caret.line) + 1) + ":" + std::to_string(caret.column + 1);

    if (scale != pixelScale) {
        hint += "   " + std::string(FontRenderer::CROSS) + std::to_string(scale);
    }

    font.Draw(hint, status, EDITOR_VARIANT_HINT, TextSpacing::Narrow, scale);
}
