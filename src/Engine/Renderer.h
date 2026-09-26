#pragma once

#include <functional>
#include <vector>

#include "raylib.h"
#include "SpriteInstance.h"

class Renderer {
public:
    Renderer(int virtualWidth, int virtualHeight);

    ~Renderer();

    static Vector2 SnapToPixel(Vector2 position);

    static Vector2 GetScreenPosition(const AnimatedSprite &sprite, Vector2 worldPosition, Anchor anchor);

    void Draw(const AnimatedSprite &sprite, Vector2 position);

    // Queues the character for the Z sorted layer. It is only drawn in EndDraw,
    // but then in the right order.
    void Submit(const SpriteInstance &instance);

    // Converts a window position (e.g. the mouse) into coordinates of the
    // virtual viewport, without the camera. Suitable for the UI layer.
    Vector2 ScreenToViewport(Vector2 screenPosition) const;

    // Like ScreenToViewport, but also takes out the camera.
    // Suitable for everything that lives in the world.
    Vector2 ScreenToWorld(Vector2 screenPosition) const;

    Vector2 MouseViewportPosition() const;

    Vector2 MouseWorldPosition() const;

    // Centers the view on a point in the world
    void SetCameraFocus(Vector2 focus);

    // Enlarges the world around the center of the view, 1 for normal. Positions
    // stay world pixels, ScreenToWorld takes the zoom out as well.
    void SetCameraZoom(float zoom);

    float CameraZoom() const;

    // Shows the world from (0,0) again, without offset and without zoom
    void ClearCamera();

    // Drawing runs in three layers:
    //   BeginDraw    world, camera active: background and Submit
    //   BeginOverlay world, camera active: above the characters, e.g. health bars
    //   BeginUI      screen, without camera: HUD, menus
    // Layers may be skipped, EndDraw catches up on them.
    void BeginDraw();

    void BeginOverlay();

    void BeginUI();

    // screenLayer draws in window pixels, after the viewport was blown up to
    // the window: for things that want the real pixels of the screen instead
    // of the coarse grid of the viewport, e.g. the code editor.
    void EndDraw(const std::function<void()> &screenLayer = {});

    // Queues a health bar above the character, fraction from 0.0 to 1.0 and
    // maximum as the health it can have at most: the bar gets wider the more
    // that is. It is drawn in the overlay layer, with the same depth sorting
    // as the characters.
    void SubmitHealthBar(const SpriteInstance &instance, float fraction, int maximum);

    // The same for energy, in blue and a little flatter. Bars of one character
    // stack upwards in the order they were submitted, so the energy bar belongs
    // in front of the health bar.
    void SubmitEnergyBar(const SpriteInstance &instance, float fraction, int maximum);

    // Draws a frame around every sprite in the Development mode
    void SetShowHitboxes(bool show);

    bool ShowHitboxes() const;

    int GetWidth() const;

    int GetHeight() const;

    // Whole number scale factor of the viewport in the window: one pixel of
    // the viewport is this many pixels on the screen
    int GetScale() const;

    // Black border left and top when the viewport does not fit exactly
    Vector2 GetLetterbox() const;

private:

    void DrawSelector(const SpriteInstance &character);

    void DrawShadow(const SpriteInstance &character);

    void Draw(const SpriteInstance &character);

    void DrawHitbox(const SpriteInstance &instance);

    void DrawSubmitted();

    struct BarEntry {
        const SpriteInstance *instance;
        float fraction;

        // Width in pixels, from the maximum value of the bar
        int width;
        int height;

        Color empty;
        Color full;
    };

    // offset is the height of the bars of this character below this one
    void DrawBar(const BarEntry &entry, int offset);

    void DrawBars();

    std::vector<BarEntry> bars;

    bool showHitboxes = false;

    Camera2D camera{};

    enum class Phase {
        World,
        Overlay,
        UI
    };

    Phase phase = Phase::World;

    std::vector<const SpriteInstance *> submitted;

    int virtualWidth;
    int virtualHeight;

    RenderTexture2D target;
};
