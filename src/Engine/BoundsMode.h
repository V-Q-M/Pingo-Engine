#pragma once

// What happens when a character reaches the edge of the world
enum class BoundsMode {
    // The character is stopped at the edge
    OutOfBounds,

    // The character reappears on the opposite side
    WarpAround,

    // The character is stopped, but the view follows it
    Follow
};
