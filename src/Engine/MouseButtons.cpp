#include "MouseButtons.h"

#include "raylib.h"

void MouseButtons::Update() {
    ClickMonitor::Clicks clicks = monitor.Take();

    bool leftPressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    bool control = ClickMonitor::CONTROL_CLICK_IS_RIGHT &&
                   (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL));

    // raylib reports a Ctrl click as a left click. It belongs to the right
    // button until the left one goes up again.
    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        leftHeldAsRight = false;
    } else if (leftPressed && (control || monitor.IsLeftHeldAsRight())) {
        leftHeldAsRight = true;
    }

    bool controlClick = leftPressed && leftHeldAsRight;

    // The monitor and raylib usually see the same click in the same frame, so
    // they are combined, not added. raylib alone still covers platforms without
    // a monitor and input that does not come from the system.
    leftClicked = clicks.left > 0 || (leftPressed && !controlClick);
    rightClicked = clicks.right > 0 || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) || controlClick;
}

bool MouseButtons::IsLeftClicked() const {
    return leftClicked;
}

bool MouseButtons::IsRightClicked() const {
    return rightClicked;
}

bool MouseButtons::IsLeftDown() const {
    return IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !leftHeldAsRight;
}
