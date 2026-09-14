#pragma once

#include "raylib.h"

// Free flying camera for the Development mode. It follows no character
// but is controlled directly. Its center stays inside the world:
// you can move up to the edge and still see some empty space beyond it.
//
// On top of that there is a zoom in fixed steps. It only enlarges the view: the
// world keeps its pixels, characters still stand on whole pixels of the world.
class DevCamera {
public:
    // Also resets the zoom
    void Reset(Vector2 focus);

    // direction gets normalized, speed in viewport pixels per second.
    // With zoom the camera therefore moves more slowly through the world.
    void Update(Vector2 direction, float speed, float dt, Rectangle area);

    // Movement of the mouse wheel, positive zooms in. Small movements, like from a
    // trackpad, add up until they make a step. anchor is the point in the world
    // that stays in its place on screen, e.g. the one under the mouse.
    void Scroll(float wheel, Vector2 anchor);

    // Changes by steps levels, limited to the smallest and the largest
    void ChangeZoom(int steps, Vector2 anchor);

    void ResetZoom();

    Vector2 Focus() const;

    float Zoom() const;

    // Short form for displays, e.g. "x2" or "x1/2"
    const char *ZoomLabel() const;

private:
    // Index of the level without zoom, see ZOOM_LEVELS in DevCamera.cpp
    static constexpr int DEFAULT_ZOOM_LEVEL = 2;

    Vector2 focus{0.0f, 0.0f};

    int zoomLevel = DEFAULT_ZOOM_LEVEL;

    // Mouse wheel movement that has not yet added up to a whole step
    float scrolled = 0.0f;
};
