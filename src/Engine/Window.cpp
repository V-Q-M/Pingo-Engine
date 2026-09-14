#include "Window.h"

#include "raylib.h"

Window::Window(int width, int height, const std::string &title) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);

    InitWindow(width, height, title.c_str());
    InitAudioDevice();

    SetTargetFPS(60);

    // Otherwise raylib quits the game on ESC. But the key belongs to the pause
    // menu.
    SetExitKey(KEY_NULL);
}

Window::~Window() {
    CloseAudioDevice();
    CloseWindow();
}
