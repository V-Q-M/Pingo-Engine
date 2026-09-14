#pragma once

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

    void EndDraw();

    // Queues a health bar above the character, fraction from 0.0 to 1.0. It is
    // drawn in the overlay layer, with the same depth sorting as the characters.
    void SubmitHealthBar(const SpriteInstance &instance, float fraction);

    // Draws a frame around every sprite in the Development mode
    void SetShowHitboxes(bool show);

    bool ShowHitboxes() const;

    int GetWidth() const;

    int GetHeight() const;

private:
    // Whole number scale factor of the viewport in the window
    int Scale() const;

    // Black border left and top when the viewport does not fit exactly
    Vector2 Letterbox() const;

    void DrawSelector(const SpriteInstance &character);

    void DrawShadow(const SpriteInstance &character);

    void Draw(const SpriteInstance &character);

    void DrawHitbox(const SpriteInstance &instance);

    void DrawSubmitted();

    void DrawHealthBar(const SpriteInstance &instance, float fraction);

    void DrawHealthBars();

    struct HealthBarEntry {
        const SpriteInstance *instance;
        float fraction;
    };

    std::vector<HealthBarEntry> healthBars;

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
