#pragma once

// What the mouse cursor looks like, see Engine::RequestCursor. Stronger states
// are listed further down: if a frame reports several, the strongest wins.
enum class CursorState {
    // The pointing hand in white
    Idle,

    // Over something clickable the hand lights up yellow
    Hover,

    // While dragging the hand grabs, also yellow
    Grab
};
