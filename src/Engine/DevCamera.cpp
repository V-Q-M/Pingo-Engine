#include "DevCamera.h"

#include <algorithm>
#include <cmath>
#include <iterator>

// Zooming in uses whole numbers, so the pixels stay sharp. When zooming out
// single pixel rows get dropped, which is fine for an overview.
constexpr float ZOOM_LEVELS[] = {0.25f, 0.5f, 1.0f, 2.0f, 3.0f, 4.0f};
constexpr const char *ZOOM_LABELS[] = {"x1/4", "x1/2", "x1", "x2", "x3", "x4"};

constexpr int ZOOM_LEVEL_COUNT = static_cast<int>(std::size(ZOOM_LEVELS));

void DevCamera::Reset(Vector2 focus) {
    this->focus = focus;

    ResetZoom();
}

Vector2 DevCamera::Focus() const {
    return focus;
}

float DevCamera::Zoom() const {
    return ZOOM_LEVELS[zoomLevel];
}

const char *DevCamera::ZoomLabel() const {
    return ZOOM_LABELS[zoomLevel];
}

void DevCamera::Update(Vector2 direction, float speed, float dt, Rectangle area) {
    float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);

    if (length > 0.0f) {
        float distance = speed / Zoom() * dt;

        focus.x += direction.x / length * distance;
        focus.y += direction.y / length * distance;
    }

    focus.x = std::clamp(focus.x, area.x, area.x + area.width);
    focus.y = std::clamp(focus.y, area.y, area.y + area.height);
}

void DevCamera::Scroll(float wheel, Vector2 anchor) {
    scrolled += wheel;

    // Every full unit is one step, the rest waits for the next movement
    int steps = static_cast<int>(scrolled);

    if (steps == 0) {
        return;
    }

    scrolled -= static_cast<float>(steps);

    ChangeZoom(steps, anchor);
}

void DevCamera::ChangeZoom(int steps, Vector2 anchor) {
    int next = std::clamp(zoomLevel + steps, 0, ZOOM_LEVEL_COUNT - 1);

    if (next == zoomLevel) {
        return;
    }

    // The distance from the anchor to the center grows or shrinks with the zoom.
    // If the center moves along accordingly, the anchor stays in place on screen.
    float ratio = ZOOM_LEVELS[zoomLevel] / ZOOM_LEVELS[next];

    focus.x = anchor.x + (focus.x - anchor.x) * ratio;
    focus.y = anchor.y + (focus.y - anchor.y) * ratio;

    zoomLevel = next;
}

void DevCamera::ResetZoom() {
    zoomLevel = DEFAULT_ZOOM_LEVEL;
    scrolled = 0.0f;
}
