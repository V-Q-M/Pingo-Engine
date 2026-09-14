#pragma once

// Rectangle for hit and collision checks, relative to the character's
// position: horizontally centered on it, the bottom edge lies at
// position.y + offsetY. A negative offsetY raises the box, which is needed
// for floating characters like the witches.
struct Hitbox {
    float width = 0.0f;
    float height = 0.0f;
    float offsetY = 0.0f;
};
