#include "ScreenshotHarness.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>
#include <unistd.h>

#include "raylib.h"

#include "Engine/Engine.h"

// raylib links GLFW into itself but ships no GLFW header. Only the calls that
// return the callbacks raylib registered are needed, so they are declared here.
// The Homebrew dylib hides these functions, the screenshot build therefore
// links the static library, see CMakeLists.txt.
extern "C" {
struct GLFWwindow;

typedef void (*GLFWkeyfun)(GLFWwindow *, int, int, int, int);
typedef void (*GLFWcharfun)(GLFWwindow *, unsigned int);
typedef void (*GLFWmousebuttonfun)(GLFWwindow *, int, int, int);
typedef void (*GLFWcursorposfun)(GLFWwindow *, double, double);
typedef void (*GLFWscrollfun)(GLFWwindow *, double, double);

GLFWwindow *glfwGetCurrentContext(void);
GLFWkeyfun glfwSetKeyCallback(GLFWwindow *, GLFWkeyfun);
GLFWcharfun glfwSetCharCallback(GLFWwindow *, GLFWcharfun);
GLFWmousebuttonfun glfwSetMouseButtonCallback(GLFWwindow *, GLFWmousebuttonfun);
GLFWcursorposfun glfwSetCursorPosCallback(GLFWwindow *, GLFWcursorposfun);
GLFWscrollfun glfwSetScrollCallback(GLFWwindow *, GLFWscrollfun);
}

namespace {
    // The values of GLFW's action and modifier constants
    constexpr int ACTION_RELEASE = 0;
    constexpr int ACTION_PRESS = 1;

    constexpr int MOD_SHIFT = 1;
    constexpr int MOD_CONTROL = 2;
    constexpr int MOD_ALT = 4;
    constexpr int MOD_SUPER = 8;

    // The window is 1280x720 and the game draws 320x180 scaled up by four
    constexpr int GRID_STEP = 40;

    std::string Lower(std::string text) {
        for (char &c: text) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }

        return text;
    }

    std::vector<std::string> Split(const std::string &text, char separator) {
        std::vector<std::string> parts;
        std::string part;
        std::stringstream stream(text);

        while (std::getline(stream, part, separator)) {
            parts.push_back(part);
        }

        return parts;
    }

    // A key by its name. Letters, digits and the keys the game reacts to.
    int KeyByName(const std::string &rawName) {
        std::string name = Lower(rawName);

        if (name.size() == 1) {
            char c = name[0];

            if (c >= 'a' && c <= 'z') {
                return KEY_A + (c - 'a');
            }

            if (c >= '0' && c <= '9') {
                return KEY_ZERO + (c - '0');
            }
        }

        static const std::map<std::string, int> names = {
            {"enter", KEY_ENTER}, {"return", KEY_ENTER}, {"escape", KEY_ESCAPE}, {"esc", KEY_ESCAPE},
            {"space", KEY_SPACE}, {"tab", KEY_TAB}, {"backspace", KEY_BACKSPACE}, {"delete", KEY_DELETE},
            {"up", KEY_UP}, {"down", KEY_DOWN}, {"left", KEY_LEFT}, {"right", KEY_RIGHT},
            {"home", KEY_HOME}, {"end", KEY_END}, {"pageup", KEY_PAGE_UP}, {"pagedown", KEY_PAGE_DOWN},
            {"shift", KEY_LEFT_SHIFT}, {"ctrl", KEY_LEFT_CONTROL}, {"control", KEY_LEFT_CONTROL},
            {"alt", KEY_LEFT_ALT}, {"cmd", KEY_LEFT_SUPER}, {"super", KEY_LEFT_SUPER},
            {"comma", KEY_COMMA}, {"period", KEY_PERIOD}, {"minus", KEY_MINUS}, {"slash", KEY_SLASH},
            {"semicolon", KEY_SEMICOLON}, {"colon", KEY_SEMICOLON}, {"equal", KEY_EQUAL},
            {"f1", KEY_F1}, {"f2", KEY_F2}, {"f3", KEY_F3}, {"f4", KEY_F4}, {"f5", KEY_F5}, {"f6", KEY_F6},
            {"f7", KEY_F7}, {"f8", KEY_F8}, {"f9", KEY_F9}, {"f10", KEY_F10}, {"f11", KEY_F11},
            {"f12", KEY_F12},
        };

        auto found = names.find(name);

        return found == names.end() ? KEY_NULL : found->second;
    }

    int ModifierOf(int key) {
        switch (key) {
            case KEY_LEFT_SHIFT:
            case KEY_RIGHT_SHIFT:
                return MOD_SHIFT;
            case KEY_LEFT_CONTROL:
            case KEY_RIGHT_CONTROL:
                return MOD_CONTROL;
            case KEY_LEFT_ALT:
            case KEY_RIGHT_ALT:
                return MOD_ALT;
            case KEY_LEFT_SUPER:
            case KEY_RIGHT_SUPER:
                return MOD_SUPER;
            default:
                return 0;
        }
    }

    // What a key types on its own, 0 if nothing. Only what a US keyboard does
    // for letters, digits and space is needed.
    unsigned int CharacterOf(int key, int mods) {
        if (mods & (MOD_CONTROL | MOD_ALT | MOD_SUPER)) {
            return 0;
        }

        if (key >= KEY_A && key <= KEY_Z) {
            return static_cast<unsigned int>((mods & MOD_SHIFT ? 'A' : 'a') + (key - KEY_A));
        }

        if (key >= KEY_ZERO && key <= KEY_NINE) {
            return static_cast<unsigned int>('0' + (key - KEY_ZERO));
        }

        if (key == KEY_SPACE) {
            return ' ';
        }

        return 0;
    }

    // The key and modifiers that produce a character when typing text
    struct Typed {
        int key = KEY_NULL;
        int mods = 0;
    };

    Typed KeyOfCharacter(char c) {
        Typed typed;

        if (c >= 'a' && c <= 'z') {
            typed.key = KEY_A + (c - 'a');
        } else if (c >= 'A' && c <= 'Z') {
            typed.key = KEY_A + (c - 'A');
            typed.mods = MOD_SHIFT;
        } else if (c >= '0' && c <= '9') {
            typed.key = KEY_ZERO + (c - '0');
        } else if (c == ' ') {
            typed.key = KEY_SPACE;
        } else if (c == '_') {
            typed.key = KEY_MINUS;
            typed.mods = MOD_SHIFT;
        } else if (c == '-') {
            typed.key = KEY_MINUS;
        } else if (c == '.') {
            typed.key = KEY_PERIOD;
        } else if (c == ',') {
            typed.key = KEY_COMMA;
        }

        return typed;
    }

    // Where the commands come from: a file that ends, or a FIFO that waits for
    // the next line. Between lines of a FIFO the game keeps running.
    class CommandSource {
    public:
        explicit CommandSource(const std::string &path) {
            struct stat info {};
            bool isFifo = stat(path.c_str(), &info) == 0 && S_ISFIFO(info.st_mode);

            if (isFifo) {
                // Read and write, so the FIFO never reports its end when the
                // writer closes it between two commands
                descriptor = open(path.c_str(), O_RDWR | O_NONBLOCK);
                fifo = true;
            } else {
                descriptor = open(path.c_str(), O_RDONLY | O_NONBLOCK);
            }
        }

        ~CommandSource() {
            if (descriptor >= 0) {
                close(descriptor);
            }
        }

        bool IsOpen() const {
            return descriptor >= 0;
        }

        bool IsFifo() const {
            return fifo;
        }

        // The next line without waiting. false if there is none right now.
        // ended is set when a file has no more lines.
        bool Poll(std::string &line, bool &ended) {
            ended = false;

            while (true) {
                std::size_t newline = buffer.find('\n');

                if (newline != std::string::npos) {
                    line = buffer.substr(0, newline);
                    buffer.erase(0, newline + 1);
                    return true;
                }

                char chunk[4096];
                ssize_t count = read(descriptor, chunk, sizeof(chunk));

                if (count > 0) {
                    buffer.append(chunk, static_cast<std::size_t>(count));
                    continue;
                }

                // A file at its end: a last line without newline still counts
                if (count == 0 && !fifo) {
                    if (!buffer.empty()) {
                        line = buffer;
                        buffer.clear();
                        return true;
                    }

                    ended = true;
                }

                return false;
            }
        }

    private:
        int descriptor = -1;
        bool fifo = false;
        std::string buffer;
    };

    class Harness {
    public:
        Harness(Engine &engine, std::string outputDir) : engine(engine), outputDir(std::move(outputDir)) {
            GLFWwindow *window = glfwGetCurrentContext();

            // Setting a callback returns the one that was there before. That
            // is how raylib's own handlers are found, they are static.
            keyCallback = glfwSetKeyCallback(window, nullptr);
            glfwSetKeyCallback(window, keyCallback);

            charCallback = glfwSetCharCallback(window, nullptr);
            glfwSetCharCallback(window, charCallback);

            buttonCallback = glfwSetMouseButtonCallback(window, nullptr);
            glfwSetMouseButtonCallback(window, buttonCallback);

            cursorCallback = glfwSetCursorPosCallback(window, nullptr);
            glfwSetCursorPosCallback(window, cursorCallback);

            scrollCallback = glfwSetScrollCallback(window, nullptr);
            glfwSetScrollCallback(window, scrollCallback);

            this->window = window;

            std::filesystem::create_directories(this->outputDir + "/probe");
            log.open(this->outputDir + "/harness.log", std::ios::trunc);
        }

        bool IsReady() const {
            return window && keyCallback && charCallback && buttonCallback && cursorCallback && scrollCallback;
        }

        int Run(const std::string &commands) {
            CommandSource source(commands);

            if (!source.IsOpen()) {
                Report("error cannot open " + commands);
                return 1;
            }

            // The pointer waits in the lower right corner instead of on top of
            // the first tab
            Move(1250, 700);
            Frames(5);

            std::string line;
            bool ended = false;

            while (!quit) {
                if (source.Poll(line, ended)) {
                    Handle(line);
                    continue;
                }

                if (ended) {
                    break;
                }

                // Nothing to do: keep the window alive until the next command
                Frames(1);
            }

            Report("done");
            return failures == 0 ? 0 : 2;
        }

    private:
        void Report(const std::string &text) {
            log << text << std::endl;
            std::printf("[harness] %s\n", text.c_str());
            std::fflush(stdout);
        }

        // The engine's frames. The volume stays at 0, the run should be silent.
        void Frames(int count) {
            for (int i = 0; i < count && !quit; i++) {
                if (WindowShouldClose()) {
                    quit = true;
                    break;
                }

                engine.RunFrame();
                SetMasterVolume(0.0f);
            }
        }

        void Key(int key, int action) {
            int scancode = 0;
            keyCallback(window, key, scancode, action, mods);
        }

        void Press(int key) {
            mods |= ModifierOf(key);
            Key(key, ACTION_PRESS);
        }

        void Release(int key) {
            Key(key, ACTION_RELEASE);
            mods &= ~ModifierOf(key);
        }

        void Move(double x, double y) {
            mouseX = x;
            mouseY = y;
            cursorCallback(window, x, y);
        }

        void Button(int button, int action) {
            buttonCallback(window, button, action, mods);
        }

        void Click(int button, double x, double y) {
            Move(x, y);
            Frames(2);
            Button(button, ACTION_PRESS);
            Frames(2);
            Button(button, ACTION_RELEASE);
            Frames(2);
        }

        void Drag(double x1, double y1, double x2, double y2, int steps) {
            Move(x1, y1);
            Frames(2);
            Button(0, ACTION_PRESS);
            Frames(2);

            for (int i = 1; i <= steps; i++) {
                double t = static_cast<double>(i) / steps;
                Move(x1 + (x2 - x1) * t, y1 + (y2 - y1) * t);
                Frames(1);
            }

            Frames(2);
            Button(0, ACTION_RELEASE);
            Frames(2);
        }

        // One key or a combination like ctrl+shift+z: the modifiers go down
        // first and come up last, and the key types its character
        bool Tap(const std::string &combo) {
            std::vector<std::string> parts = Split(combo, '+');

            if (parts.empty()) {
                return false;
            }

            std::vector<int> keys;

            for (const std::string &part: parts) {
                int key = KeyByName(part);

                if (key == KEY_NULL) {
                    return false;
                }

                keys.push_back(key);
            }

            for (std::size_t i = 0; i + 1 < keys.size(); i++) {
                Press(keys[i]);
            }

            int last = keys.back();
            Press(last);

            unsigned int character = CharacterOf(last, mods);

            if (character != 0) {
                charCallback(window, character);
            }

            Frames(2);
            Release(last);

            for (std::size_t i = keys.size() - 1; i-- > 0;) {
                Release(keys[i]);
            }

            Frames(2);
            return true;
        }

        void Type(const std::string &text) {
            for (char c: text) {
                Typed typed = KeyOfCharacter(c);

                if (typed.mods & MOD_SHIFT) {
                    Press(KEY_LEFT_SHIFT);
                }

                if (typed.key != KEY_NULL) {
                    Press(typed.key);
                }

                charCallback(window, static_cast<unsigned int>(static_cast<unsigned char>(c)));

                if (typed.key != KEY_NULL) {
                    Release(typed.key);
                }

                if (typed.mods & MOD_SHIFT) {
                    Release(KEY_LEFT_SHIFT);
                }

                Frames(1);
            }

            Frames(2);
        }

        bool Save(const std::string &path, bool grid) {
            Image image = LoadImageFromScreen();

            if (image.data == nullptr) {
                return false;
            }

            // A high resolution display renders more pixels than the window has
            // points, the screenshots are always the size of the window
            if (image.width != GetScreenWidth() || image.height != GetScreenHeight()) {
                ImageResize(&image, GetScreenWidth(), GetScreenHeight());
            }

            if (grid) {
                DrawGrid(image);
            }

            bool saved = ExportImage(image, path.c_str());
            UnloadImage(image);

            return saved;
        }

        static void DrawGrid(Image &image) {
            Color line = {255, 0, 255, 90};
            Color strong = {255, 0, 255, 170};

            for (int x = 0; x < image.width; x += GRID_STEP) {
                ImageDrawLine(&image, x, 0, x, image.height, x % 160 == 0 ? strong : line);
            }

            for (int y = 0; y < image.height; y += GRID_STEP) {
                ImageDrawLine(&image, 0, y, image.width, y, y % 160 == 0 ? strong : line);
            }

            for (int x = 0; x < image.width; x += 80) {
                ImageDrawText(&image, TextFormat("%d", x), x + 2, 2, 10, YELLOW);
            }

            for (int y = 80; y < image.height; y += 80) {
                ImageDrawText(&image, TextFormat("%d", y), 2, y + 2, 10, YELLOW);
            }
        }

        void Handle(const std::string &rawLine) {
            std::string line = rawLine;

            while (!line.empty() && (line.back() == '\r' || std::isspace(static_cast<unsigned char>(line.back())))) {
                line.pop_back();
            }

            std::size_t first = line.find_first_not_of(" \t");

            if (first == std::string::npos || line[first] == '#') {
                return;
            }

            line = line.substr(first);

            std::istringstream words(line);
            std::string command;
            words >> command;
            command = Lower(command);

            // What comes after the command word, for the ones that take text
            std::string rest;
            std::getline(words >> std::ws, rest);

            bool valid = true;
            std::istringstream args(rest);

            if (command == "wait") {
                int count = 0;
                valid = static_cast<bool>(args >> count);
                Frames(count);
            } else if (command == "move") {
                double x, y;
                valid = static_cast<bool>(args >> x >> y);

                if (valid) {
                    Move(x, y);
                    Frames(2);
                }
            } else if (command == "click" || command == "rclick") {
                double x, y;
                valid = static_cast<bool>(args >> x >> y);

                if (valid) {
                    Click(command == "click" ? 0 : 1, x, y);
                }
            } else if (command == "drag") {
                double x1, y1, x2, y2;
                int steps = 20;
                valid = static_cast<bool>(args >> x1 >> y1 >> x2 >> y2);
                args >> steps;

                if (valid) {
                    Drag(x1, y1, x2, y2, steps);
                }
            } else if (command == "mousedown" || command == "mouseup") {
                std::string which = "left";
                args >> which;

                Button(Lower(which) == "right" ? 1 : 0, command == "mousedown" ? ACTION_PRESS : ACTION_RELEASE);
                Frames(2);
            } else if (command == "scroll") {
                double amount = 0;
                valid = static_cast<bool>(args >> amount);

                if (valid) {
                    scrollCallback(window, 0.0, amount);
                    Frames(2);
                }
            } else if (command == "key") {
                std::string combo;

                while (valid && args >> combo) {
                    valid = Tap(combo);
                }
            } else if (command == "keydown" || command == "keyup") {
                int key = KeyByName(rest);
                valid = key != KEY_NULL;

                if (valid) {
                    command == "keydown" ? Press(key) : Release(key);
                    Frames(2);
                }
            } else if (command == "hold") {
                std::string name;
                int count = 0;
                valid = static_cast<bool>(args >> name >> count) && KeyByName(name) != KEY_NULL;

                if (valid) {
                    Press(KeyByName(name));
                    Frames(count);
                    Release(KeyByName(name));
                    Frames(2);
                }
            } else if (command == "type") {
                Type(rest);
            } else if (command == "scene") {
                valid = engine.EnterScene(rest);
                Frames(5);
            } else if (command == "fps") {
                int fps = 60;
                valid = static_cast<bool>(args >> fps);
                SetTargetFPS(fps);
            } else if (command == "shot" || command == "probe") {
                std::string name;
                valid = static_cast<bool>(args >> name);

                if (valid) {
                    bool probe = command == "probe";
                    std::string path = outputDir + (probe ? "/probe/" : "/") + name + ".png";

                    valid = Save(path, probe);

                    if (valid) {
                        Report("saved " + path);
                    }
                }
            } else if (command == "log") {
                Report("log " + rest);
            } else if (command == "quit") {
                quit = true;
            } else {
                Report("error unknown command: " + line);
                failures++;
                return;
            }

            if (!valid) {
                Report("error bad arguments: " + line);
                failures++;
                return;
            }

            Report("ok " + line);
        }

        Engine &engine;
        std::string outputDir;
        std::ofstream log;

        GLFWwindow *window = nullptr;
        GLFWkeyfun keyCallback = nullptr;
        GLFWcharfun charCallback = nullptr;
        GLFWmousebuttonfun buttonCallback = nullptr;
        GLFWcursorposfun cursorCallback = nullptr;
        GLFWscrollfun scrollCallback = nullptr;

        double mouseX = 0;
        double mouseY = 0;

        // The modifiers that are held right now
        int mods = 0;

        bool quit = false;
        int failures = 0;
    };
}

int ScreenshotHarness::Run(Engine &engine, const std::string &commands, const std::string &outputDir) {
    Harness harness(engine, outputDir);

    if (!harness.IsReady()) {
        std::fprintf(stderr, "[harness] the GLFW callbacks of raylib were not found\n");
        return 1;
    }

    return harness.Run(commands);
}
