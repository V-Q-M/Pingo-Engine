#pragma once

#include "ClickMonitor.h"

// The mouse buttons of the current frame, the same for the whole engine.
//
// Scenes and menus ask here instead of calling raylib directly. Besides normal
// clicks this also catches taps on a trackpad, which raylib misses when press
// and release land between two frames. On macOS Ctrl + click and the two
// finger tap are right clicks.
class MouseButtons {
public:
    // Once per frame, before anything asks for the buttons
    void Update();

    // Was the button clicked in this frame?
    bool IsLeftClicked() const;

    bool IsRightClicked() const;

    // Is the left button held down, e.g. to drag something? Not while it
    // belongs to a Ctrl click.
    bool IsLeftDown() const;

private:
    // Created together with the engine, after the window
    ClickMonitor monitor;

    bool leftClicked = false;
    bool rightClicked = false;

    // The left button went down as a Ctrl click and is still held
    bool leftHeldAsRight = false;
};
