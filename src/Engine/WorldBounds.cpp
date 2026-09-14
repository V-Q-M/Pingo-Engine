#include "WorldBounds.h"

#include <algorithm>

WorldBounds::WorldBounds(Rectangle area, BoundsMode mode)
    : area(area),
      mode(mode) {
}

const Rectangle &WorldBounds::Area() const {
    return area;
}

BoundsMode WorldBounds::Mode() const {
    return mode;
}

void WorldBounds::SetMode(BoundsMode mode) {
    this->mode = mode;
}

void WorldBounds::Apply(SpriteInstance &instance) const {
    switch (mode) {
        // With Follow the character also stays in the world, only the view moves
        // along with it
        case BoundsMode::OutOfBounds:
        case BoundsMode::Follow:
            Clamp(instance);
            break;

        case BoundsMode::WarpAround:
            Warp(instance);
            break;
    }
}

// The hitbox is used, not the sprite: the body should touch the world, not
// the transparent border of the frame.
void WorldBounds::Clamp(SpriteInstance &instance) const {
    Rectangle box = instance.Bounds();
    Vector2 position = instance.Position();

    float right = area.x + area.width;
    float bottom = area.y + area.height;

    if (box.x < area.x) {
        position.x += area.x - box.x;
    } else if (box.x + box.width > right) {
        position.x -= (box.x + box.width) - right;
    }

    if (box.y < area.y) {
        position.y += area.y - box.y;
    } else if (box.y + box.height > bottom) {
        position.y -= (box.y + box.height) - bottom;
    }

    instance.SetPosition(position);
}

// It only wraps once the hitbox has completely left the edge. Otherwise the
// character would look cut in half in the middle of the screen.
void WorldBounds::Warp(SpriteInstance &instance) const {
    Rectangle box = instance.Bounds();
    Vector2 position = instance.Position();

    float right = area.x + area.width;
    float bottom = area.y + area.height;

    if (box.x + box.width < area.x) {
        position.x += right - box.x;
    } else if (box.x > right) {
        position.x -= (box.x + box.width) - area.x;
    }

    if (box.y + box.height < area.y) {
        position.y += bottom - box.y;
    } else if (box.y > bottom) {
        position.y -= (box.y + box.height) - area.y;
    }

    instance.SetPosition(position);
}

Vector2 WorldBounds::CameraFocus(Vector2 focus, int viewWidth, int viewHeight) const {
    float halfWidth = viewWidth / 2.0f;
    float halfHeight = viewHeight / 2.0f;

    float minX = area.x + halfWidth;
    float maxX = area.x + area.width - halfWidth;
    float minY = area.y + halfHeight;
    float maxY = area.y + area.height - halfHeight;

    // If the world is smaller than the viewport, it is shown centered instead of
    // moving past the edge
    float x = minX > maxX
                  ? area.x + area.width / 2.0f
                  : std::clamp(focus.x, minX, maxX);

    float y = minY > maxY
                  ? area.y + area.height / 2.0f
                  : std::clamp(focus.y, minY, maxY);

    return {x, y};
}
