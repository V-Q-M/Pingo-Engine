#pragma once

// Rectangle for hit and collision checks, relative to the character's
// position: horizontally centered on position.x + offsetX, the bottom edge lies
// at position.y + offsetY. A negative offsetY raises the box, which is needed
// for floating characters like the witches, a negative offsetX moves it left.
struct Hitbox {
    float width = 0.0f;
    float height = 0.0f;
    float offsetY = 0.0f;
    float offsetX = 0.0f;
};
