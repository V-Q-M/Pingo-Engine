#include "ClickMonitor.h"

// The raylib target hands the SDK frameworks to the compiler as normal include
// paths, so -Wall and -Wextra would also warn about Apple's headers. The
// warnings stay on for everything below.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#import <AppKit/AppKit.h>
#pragma clang diagnostic pop

#include "raylib.h"

struct ClickMonitor::State {
    Clicks clicks;

    bool leftHeldAsRight = false;

    // The handle of the local event monitor, for removing it again
    id monitor = nil;
};

ClickMonitor::ClickMonitor() : state(new State()) {
    // On macOS raylib hands out the NSWindow of the game
    NSWindow *window = (__bridge NSWindow *)GetWindowHandle();

    if (window == nil) {
        return;
    }

    // The block only keeps the pointer, the state lives as long as the monitor
    State *target = state;

    NSEventMask mask = NSEventMaskLeftMouseDown | NSEventMaskLeftMouseUp | NSEventMaskRightMouseDown;

    // A local monitor sees every event before the window handles it, also when
    // press and release arrive together within one poll
    state->monitor = [NSEvent addLocalMonitorForEventsMatchingMask:mask handler:^NSEvent *(NSEvent *event) {
        if (event.window != window) {
            return event;
        }

        // The button going up always ends a Ctrl click, wherever the mouse is now
        if (event.type == NSEventTypeLeftMouseUp) {
            target->leftHeldAsRight = false;
            return event;
        }

        // Clicks on the title bar move the window and are not meant for the game
        NSView *content = window.contentView;
        NSPoint point = [content convertPoint:event.locationInWindow fromView:nil];

        if (!NSPointInRect(point, content.bounds)) {
            return event;
        }

        bool control = (event.modifierFlags & NSEventModifierFlagControl) != 0;

        if (event.type == NSEventTypeRightMouseDown) {
            target->clicks.right++;
        } else if (control) {
            target->clicks.right++;
            target->leftHeldAsRight = true;
        } else {
            target->clicks.left++;
        }

        return event;
    }];
}

ClickMonitor::~ClickMonitor() {
    if (state->monitor != nil) {
        [NSEvent removeMonitor:state->monitor];
    }

    delete state;
}

ClickMonitor::Clicks ClickMonitor::Take() {
    Clicks clicks = state->clicks;
    state->clicks = Clicks{};

    return clicks;
}

bool ClickMonitor::IsLeftHeldAsRight() const {
    return state->leftHeldAsRight;
}
