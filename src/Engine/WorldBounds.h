#pragma once

#include "raylib.h"

#include "BoundsMode.h"
#include "SpriteInstance.h"

class WorldBounds {
public:
    WorldBounds(Rectangle area, BoundsMode mode);

    const Rectangle &Area() const;

    BoundsMode Mode() const;

    void SetMode(BoundsMode mode);

    // Keeps the character in the world, depending on the mode
    void Apply(SpriteInstance &instance) const;

    // Point the camera should center on: follows the character, but does not
    // move past the edge of the world.
    Vector2 CameraFocus(Vector2 focus, int viewWidth, int viewHeight) const;

private:
    void Clamp(SpriteInstance &instance) const;

    void Warp(SpriteInstance &instance) const;

    Rectangle area;

    BoundsMode mode;
};
