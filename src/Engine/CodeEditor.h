#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "raylib.h"

#include "Console.h"
#include "ConsoleCommands.h"
#include "FontRenderer.h"

// A code editor that fills almost the whole screen, e.g. for the scripts of a
// scene.
//
// It draws in the pixels of the window, not in the coarse grid of the
// viewport: only that way the code can be made smaller than the font of the
// game, and a whole file fits on the screen. Its area and the mouse are
// therefore window coordinates, see Renderer::EndDraw.
//
// It opens with one or more files, shown as tabs at the top, and is written
// in like a small Vim: h, j, k and l move, i and a start typing, ESC goes
// back to the normal mode, v and V select, d, c and y work on what a motion
// covers (e.g. dw, cw, yy) or on the selection, x, s and p delete, replace and
// paste, and ":" opens a command line for w, wq and q. The
// mouse, the two scrollbars and Control with + or - work in every mode.
//
// The editor itself never writes a file: it says with a Request what it
// wants, and whoever opened it saves, see Files().
//
// Lines that only the engine cares about are not shown at all, see
// SetHiddenLines: they stay in the file and are saved with it, but they do
// not even take a line number.
//
// While it is open it owns mouse and keyboard: the scene around it pauses its
// hotkeys, exactly like the console does.
class CodeEditor {
public:
    // One open file. text is what the editor works on, path is only carried
    // along so the caller knows where to save it.
    struct File {
        // Name in the tab, e.g. "Main.cpp"
        std::string label;

        std::string path;

        std::string text;

        // Was it changed since it was opened?
        bool changed = false;
    };

    // What the editor wants after a frame. Whoever opened it does the work:
    // the editor does not know where its files belong.
    enum class Request {
        // Nothing happened, e.g. only typing
        None,

        // ":w", the editor stays open
        Save,

        // ":wq", or ESC while the Vim bindings are off
        SaveAndClose,

        // ":q", nothing is written
        Close
    };

    // What the keyboard does right now, like in Vim
    enum class VimMode {
        // Letters are commands, see the class comment
        Normal,

        // Letters land in the text
        Insert,

        // Movement grows a selection of characters, d, c and y work on it
        Visual,

        // The same, but whole lines are selected
        VisualLine
    };

    // How many spaces one indent is
    static constexpr int INDENT = 4;

    // How far the code can be zoomed, in window pixels per pixel of the font.
    // Whole numbers only: a letter stays a block of whole pixels that way,
    // like everywhere else in the engine. The scale of the viewport, e.g. 4
    // in a big window, is the normal size, everything below it shows more
    // code than the game's own font ever could.
    static constexpr int MIN_SCALE = 1;
    static constexpr int MAX_SCALE = 8;

    // area is where the window goes in the window's pixels, viewWidth and
    // viewHeight are the size of the window, for the shade behind it.
    // pixelScale is what one pixel of the viewport is worth on the screen,
    // see Renderer::GetScale: the size the editor opens with and the one
    // Control and 0 go back to.
    void Open(std::vector<File> files, Rectangle area, int viewWidth, int viewHeight, int pixelScale);

    // The same again while it is open, e.g. after the window was resized.
    // The zoom stays where the developer put it.
    void SetLayout(Rectangle area, int viewWidth, int viewHeight, int pixelScale);

    void Close();

    // Decides per line whether it belongs to the engine instead of to the
    // developer, e.g. the registration of a script. Such lines are left out
    // completely, together with the comment and the empty line above them:
    // they are not drawn, they cannot be edited, and they do not count when
    // the lines are numbered. Without a filter everything is shown.
    void SetHiddenLines(std::function<bool(const std::string &line)> filter);

    // Zoom of the code, between MIN_SCALE and MAX_SCALE
    int GetScale() const;

    void SetScale(int scale);

    bool IsOpen() const;

    // Was the mouse over the editor during the last Update?
    bool IsHovering() const;

    // The files with everything that was typed into them, e.g. to save them
    std::vector<File> Files() const;

    // Reads mouse and keyboard, the mouse in the pixels of the window
    Request Update(Vector2 mouse, bool clicked, const FontRenderer &font);

    // ESC was pressed. In the insert mode it goes back to the normal one, an
    // open command line is thrown away, and without the Vim bindings it asks
    // to be saved and closed. The caller does what the Request says and
    // keeps the key to itself either way.
    Request OnEscape();

    // The files were written, so the stars in the tabs go away
    void MarkSaved();

    // The Vim bindings, on by default. Off, the editor types straight away
    // and ESC saves and closes it, as it did before.
    void SetVimEnabled(bool enabled);

    bool IsVimEnabled() const;

    VimMode GetMode() const;

    // Screen layer, above everything of the scene
    void Draw(const FontRenderer &font) const;

    Rectangle Bounds() const;

private:
    // Where the caret sits: in which line and in front of which character
    struct Caret {
        std::size_t line = 0;
        std::size_t column = 0;
    };

    // The lines of the open file
    std::vector<std::string> &Lines();

    const std::vector<std::string> &Lines() const;

    // Splits the text of a file into lines and back
    static std::vector<std::string> SplitLines(const std::string &text);

    static std::string JoinLines(const std::vector<std::string> &lines);

    // Everything that changes text goes through here: it marks the file as
    // changed
    void Changed();

    void Insert(const std::string &text);

    void NewLine();

    void Backspace();

    void DeleteAhead();

    void MoveCaret(int lines, int columns);

    void MoveToLineStart();

    void MoveToLineEnd();

    // The Vim commands of the normal mode, read as typed characters so every
    // keyboard layout works and no quick tap is lost
    void NormalMode(const FontRenderer &font);

    // Control with d, u, f or b: half a screen or a whole one, like in Vim.
    // true when one of them was pressed.
    bool FastScroll(const FontRenderer &font);

    // Everything "u" has to bring back: the lines of a file and the caret
    struct State {
        std::size_t file = 0;
        std::vector<std::string> lines;
        Caret caret;
    };

    // The steps of one file that can be undone, and those that were undone
    // and can be done again
    struct History {
        std::vector<State> undo;
        std::vector<State> redo;
    };

    // How many steps are kept per file
    static constexpr std::size_t MAX_HISTORY = 200;

    State Capture() const;

    // Called after a command with the state from before it: when the text
    // changed, that is one step to undo. A command that went to the insert
    // mode is not finished yet: the step ends when the mode is left.
    void EndStep(const State &before);

    // Puts a step on the undo list, and forgets what could be done again
    void Record(State before);

    // u and Control with r
    void Undo();

    void Redo();

    // Brings a state back, e.g. from the undo list
    void Restore(const State &state);

    // The commands of the two visual modes: movement, and what to do with
    // the selection
    void VisualMode(const FontRenderer &font);

    // One typed character of the normal mode, see NormalMode
    void NormalCharacter(int character);

    // The same for the visual modes
    void VisualCharacter(int character);

    // Everything that types, the mode of the editor without Vim
    void InsertMode();

    // A piece of text, either from one place to another (the end is not
    // part of it) or a number of whole lines (both ends are part of it)
    struct Range {
        Caret from;
        Caret to;
        bool linewise = false;
    };

    // The movements that both the normal and the visual mode know, and that
    // an operator like d can be followed by. count is the number typed in
    // front of it, 0 for none. true when the caret moved.
    bool Move(int character, int count);

    // Works out what an operator ("d", "c" or "y") followed by the motion
    // covers, count is the number typed in front of it, 0 for none. change
    // is set for "c", where "w" stops at the end of the word. false when the
    // motion is unknown.
    bool MotionRange(int character, int count, bool change, Range &range);

    // The word under the caret for "iw" and "aw"
    bool WordObject(bool around, Range &range) const;

    // Runs d, c or y on a range and lands in the mode that belongs to it
    void Operate(char op, Range range);

    // The text of a range, line by line
    std::vector<std::string> RangeText(const Range &range) const;

    // Copies a range into the register and the clipboard
    void Yank(const Range &range);

    // Takes a range out of the text, false when it would touch a line of the
    // engine. The caret ends where the range started.
    bool Erase(const Range &range);

    // What p pastes: the clipboard when something was copied outside the
    // editor, otherwise the register. false when both are empty.
    bool PasteSource(std::vector<std::string> &pieces, bool &linewise) const;

    // p and P: puts the text behind the caret or in front of it, whole lines
    // go under or over the current line
    void PutText(bool after, const std::vector<std::string> &pieces, bool linewise);

    // In every mode but the insert one the caret stands on a character, this
    // brings it back there
    void ClampCaret();

    // The range the selection covers
    Range Selection() const;

    // The columns of a line that are selected, false when none are.
    // lineBreak says that the end of the line is selected as well, so the
    // selection reaches a little past the last character.
    bool SelectedColumns(std::size_t line, std::size_t &from, std::size_t &to, bool &lineBreak) const;

    bool IsVisual() const;

    void SetMode(VimMode mode);

    // To the start of the next word, or of the one before
    void WordForward();

    void WordBackward();

    // Opens a line under or over the current one and types in it
    void OpenLine(bool below);

    // Runs what was entered in the command line, e.g. "wq"
    void RunCommand(const std::string &line);

    // Puts w, wq and q into the command line, see RunCommand
    void AddCommands();

    void Scroll(int rows);

    void ScrollSideways(int columns);

    // Zooms by whole steps, e.g. +1 for Control and +
    void Zoom(int steps);

    // Which lines of the open file the editor keeps to itself. A hidden line
    // takes the comment and the empty line above it with it, so no gap is
    // left behind.
    std::vector<bool> HiddenLines() const;

    bool IsHidden(std::size_t line) const;

    // The lines that are really shown, in their order. The row of a line is
    // its place in here, and that is also the number in front of it: the
    // first row is line 1, whatever stands above it in the file.
    std::vector<std::size_t> Rows() const;

    // The row the line is shown in
    std::size_t RowOf(std::size_t line) const;

    // The next line that is really shown, from line in this direction. Runs
    // back the other way when there is none.
    std::size_t Visible(std::size_t line, int direction) const;

    // Makes sure the caret never sits inside a folded block
    void SettleCaret(int direction);

    // How many rows and columns fit into the text area
    std::size_t VisibleRows(const FontRenderer &font) const;

    std::size_t VisibleColumns(const FontRenderer &font) const;

    // Height of one row of code and width of one character, both zoomed
    float RowHeight(const FontRenderer &font) const;

    float Advance(const FontRenderer &font) const;

    // How wide a piece of a line is, with the width every single character
    // brings along, see fontSpacing.json
    float TextWidth(const std::string &line, std::size_t from, std::size_t to, const FontRenderer &font) const;

    // Height of a row of the frame, e.g. the tabs. The frame is zoomed with
    // the code, so the whole window grows and shrinks in one piece.
    float ChromeRow(const FontRenderer &font) const;

    // A length of the frame in window pixels, e.g. the padding, zoomed
    float Chrome(float pixels) const;

    // The longest line that can be scrolled to, in characters
    std::size_t LongestLine() const;

    // Areas of the window, all in viewport coordinates
    Rectangle TextArea(const FontRenderer &font) const;

    Rectangle TabBounds(std::size_t file, const FontRenderer &font) const;

    Rectangle ScrollbarBounds(const FontRenderer &font) const;

    Rectangle ScrollThumbBounds(const FontRenderer &font) const;

    Rectangle SideScrollbarBounds(const FontRenderer &font) const;

    Rectangle SideScrollThumbBounds(const FontRenderer &font) const;

    // The caret of a click in the text area
    Caret CaretAt(Vector2 mouse, const FontRenderer &font) const;

    void UpdateScrollbar(Vector2 mouse, bool clicked, const FontRenderer &font);

    void UpdateSideScrollbar(Vector2 mouse, bool clicked, const FontRenderer &font);

    // Draws one line with its colors, see the C++ tokens in the source
    void DrawLine(const std::string &line, Vector2 position, const FontRenderer &font) const;

    bool open = false;

    std::vector<File> files;

    // The text of every file as its lines, that is what the editor works on
    std::vector<std::vector<std::string>> lines;

    std::size_t current = 0;

    Caret caret;

    // First shown row and column, for scrolling
    std::size_t firstRow = 0;
    std::size_t firstColumn = 0;

    // Zoom of the code, see MIN_SCALE
    int scale = 1;

    // What one pixel of the viewport is worth on the screen: the size the
    // editor opens with, see SetLayout
    int pixelScale = 1;

    // Says which lines belong to the engine, see SetHiddenLines
    std::function<bool(const std::string &line)> hiddenLine;

    bool vim = true;

    VimMode mode = VimMode::Normal;

    // The "g" of "gg" was typed and waits for the second one
    bool pendingG = false;

    // The "d", "c" or "y" that waits for its motion, or 0
    char pendingOperator = 0;

    // The "i" or "a" of "iw" that waits for the "w", or 0
    char pendingObject = 0;

    // The number typed in front of a command, 0 when there is none
    int count = 0;

    std::vector<History> history;

    // The state from before a command that went to the insert mode: what is
    // typed there is one step together with it
    std::optional<State> sessionStart;

    // "u" or Control and r just ran, so the text that changed is no new step
    bool historyMove = false;

    // Where the selection of the visual modes started
    Caret anchor;

    // What was deleted or copied last, for p. Whole lines or a piece of one.
    std::vector<std::string> registerText;
    bool registerLinewise = false;

    // What the clipboard held when the register put its text there. If it
    // holds something else later, that was copied outside the editor.
    std::string registerClipboard;

    // The ":" line at the bottom, the typing of the game's console
    Console command;

    // What w, wq and q do, see RunCommand
    ConsoleCommands commands;

    // What a command answered, e.g. for an unknown one
    std::string answer;

    // What the editor wants after this frame, see Request
    Request request = Request::None;

    // The caret moved in this frame, so the view has to follow it. Without
    // that, scrolling away from the caret would snap back right away.
    bool followCaret = false;

    bool draggingScroll = false;
    bool draggingSideScroll = false;

    bool hovering = false;

    Rectangle area{0.0f, 0.0f, 0.0f, 0.0f};

    int viewWidth = 0;
    int viewHeight = 0;
};
