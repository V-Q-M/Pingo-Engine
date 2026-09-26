#pragma once

// Counts the mouse clicks the operating system reports, no matter when raylib
// polls its input.
//
// raylib only compares the button state of two frames. If a button goes down
// and up again in between, like when tapping on a trackpad, raylib never sees
// the click. On macOS the monitor listens to the events of the game window
// directly. Other platforms have no monitor yet, Take then always returns no
// clicks and raylib's own state has to do.
class ClickMonitor {
public:
    struct Clicks {
        int left = 0;
        int right = 0;
    };

#ifdef __APPLE__
    // On macOS Ctrl + click is a right click, like everywhere else on the system
    static constexpr bool CONTROL_CLICK_IS_RIGHT = true;
#else
    static constexpr bool CONTROL_CLICK_IS_RIGHT = false;
#endif

    // Needs the window, so only create it after InitWindow
    ClickMonitor();

    ~ClickMonitor();

    ClickMonitor(const ClickMonitor &) = delete;

    ClickMonitor &operator=(const ClickMonitor &) = delete;

    // The clicks since the last call. Clicks on the title bar do not count.
    Clicks Take();

    // Is the left button held down as part of a Ctrl click? It then belongs to
    // the right click, e.g. it must not start a drag.
    bool IsLeftHeldAsRight() const;

private:
    // Defined by the platform part, see ClickMonitorMac.mm
    struct State;

    State *state = nullptr;
};
