#pragma once

#include <deque>
#include <string>
#include <vector>

#include "raylib.h"

#include "FontRenderer.h"

// The command line at the bottom of the screen, only in the Debugging and the
// Development build.
//
// ":" opens it, typing writes into it, Enter runs the line and ESC closes it.
// Left and right move the caret through the line, up and down go through the
// lines entered before, like in a terminal. While it is open it has the
// keyboard to itself: the game keeps running, but no hotkey reacts to what is
// typed here.
//
// The console only handles typing and showing. What a line does is up to the
// ConsoleCommands of the engine, see there for adding commands.
class Console {
public:
    // How many lines of answers stay above the input
    static constexpr std::size_t HISTORY = 6;

    // Longest command line
    static constexpr std::size_t MAX_LENGTH = 48;

    // The character that opens the console, like in a terminal
    static constexpr int OPEN_CHARACTER = ':';

    void Open();

    void Close();

    bool IsOpen() const;

    // Reads the keyboard while the console is closed and opens it when ":" was
    // typed. Only looks at the typed characters in a frame where ":" can come
    // up at all, so other tools still get digits and letters.
    void CheckOpen();

    // Reads the keyboard. Returns the line when Enter was pressed, otherwise
    // it stays empty.
    std::string Update();

    // The line as it stands right now, and where its caret is. For whoever
    // draws the input somewhere else, e.g. the command line of the code
    // editor: it uses the typing of the console but its own place on screen.
    const std::string &Input() const;

    std::size_t CaretPosition() const;

    // Adds lines above the input, e.g. the answer of a command. A line break
    // starts a new line.
    void Print(const std::string &text);

    // Removes all lines above the input, the entered lines stay for Up
    void ClearHistory();

    // Screen layer, above everything of the scene
    void Draw(int viewWidth, int viewHeight, const FontRenderer &font) const;

private:
    // Up and down: brings back an entered line, step is -1 or 1
    void Browse(int step);

    // Writes the line into the input, with the caret at its end
    void SetInput(std::string line);

    // The caret never stands outside the line
    void MoveCaret(int step);

    bool open = false;

    std::string input;

    // Where the next typed character lands, 0 is in front of the line
    std::size_t caret = 0;

    // What stands above the input, oldest first
    std::deque<std::string> history;

    // The lines entered so far, oldest first
    std::vector<std::string> entered;

    // Which entered line Up and Down show, entered.size() for a new line
    std::size_t browsed = 0;
};
