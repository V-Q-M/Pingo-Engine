#include "Renderer.h"

#include <algorithm>
#include <cmath>

// Health bar: height and distance above the body, plus colors from the
// game's palette
constexpr int HEALTH_BAR_HEIGHT = 3;
constexpr int HEALTH_BAR_GAP = 3;

constexpr Color HEALTH_BAR_OUTLINE{24, 20, 37, 255};
constexpr Color HEALTH_BAR_EMPTY{120, 28, 28, 255};
constexpr Color HEALTH_BAR_FULL{61, 255, 20, 255};

#include "SpriteInstance.h"

Renderer::Renderer(int virtualWidth, int virtualHeight)
    : virtualWidth(virtualWidth),
      virtualHeight(virtualHeight) {
    target = LoadRenderTexture(virtualWidth, virtualHeight);

    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);

    camera.zoom = 1.0f;

    ClearCamera();
}

Renderer::~Renderer() {
    UnloadRenderTexture(target);
}

int Renderer::GetWidth() const {
    return virtualWidth;
}

int Renderer::GetHeight() const {
    return virtualHeight;
}


// Pixel art must land on whole pixels. A position like 114.37 would be
// drawn between two screen pixels and distort the edges.
int Renderer::Scale() const {
    return std::max(
        1,
        static_cast<int>(std::floor(std::min(
            (float) GetScreenWidth() / virtualWidth,
            (float) GetScreenHeight() / virtualHeight
        )))
    );
}

Vector2 Renderer::Letterbox() const {
    int scale = Scale();

    return {
        (GetScreenWidth() - virtualWidth * scale) / 2.0f,
        (GetScreenHeight() - virtualHeight * scale) / 2.0f
    };
}

Vector2 Renderer::ScreenToViewport(Vector2 screenPosition) const {
    int scale = Scale();
    Vector2 letterbox = Letterbox();

    return {
        (screenPosition.x - letterbox.x) / scale,
        (screenPosition.y - letterbox.y) / scale
    };
}

Vector2 Renderer::ScreenToWorld(Vector2 screenPosition) const {
    return GetScreenToWorld2D(ScreenToViewport(screenPosition), camera);
}

Vector2 Renderer::MouseViewportPosition() const {
    return ScreenToViewport(GetMousePosition());
}

Vector2 Renderer::MouseWorldPosition() const {
    return ScreenToWorld(GetMousePosition());
}

Vector2 Renderer::SnapToPixel(Vector2 position) {
    return {
        std::round(position.x),
        std::round(position.y)
    };
}

Vector2 Renderer::GetScreenPosition(const AnimatedSprite &sprite,
                                    Vector2 worldPosition,
                                    Anchor anchor) {
    Vector2 offset = sprite.GetAnchor(anchor);
    return SnapToPixel({
        worldPosition.x - offset.x,
        worldPosition.y - offset.y
    });
}

// The ring lies flat on the ground, so below shadow and character. Its
// visible part sits in the bottom third of the texture, which is why the
// texture is shifted over the feet position instead of being centered.
void Renderer::DrawSelector(const SpriteInstance &character) {
    const Texture2D *selector = character.Selector();

    if (selector == nullptr) {
        return;
    }

    Vector2 feet = character.Position();

    Vector2 topLeft = SnapToPixel({
        feet.x - selector->width / 2.0f,
        feet.y - selector->height * 0.75f
    });

    DrawTexture(
        *selector,
        static_cast<int>(topLeft.x),
        static_cast<int>(topLeft.y),
        WHITE
    );
}

void Renderer::DrawShadow(const SpriteInstance &character) {
    const auto &sprite = character.Sprite();

    // Without a shadow size, e.g. for dummy objects, there is no shadow
    if (sprite.ShadowWidth() <= 0.0f || sprite.ShadowHeight() <= 0.0f) {
        return;
    }

    Vector2 feet = SnapToPixel(character.Position());

    DrawEllipse(
        static_cast<int>(feet.x),
        static_cast<int>(feet.y + sprite.ShadowOffsetY()),
        sprite.ShadowWidth(),
        sprite.ShadowHeight(),
        Color{0, 0, 0, 80}
    );
}

void Renderer::Draw(const SpriteInstance &character) {
    DrawSelector(character);
    DrawShadow(character);
    Draw(character.Sprite(), character.Position());
}

void Renderer::Draw(const AnimatedSprite &sprite, Vector2 position) {
    Vector2 drawPosition = GetScreenPosition(sprite, position, Anchor::Feet);

    DrawTextureRec(
        sprite.GetTexture(),
        sprite.GetFrame(),
        drawPosition,
        WHITE
    );
}

void Renderer::Submit(const SpriteInstance &instance) {
    submitted.push_back(&instance);
}

void Renderer::SetShowHitboxes(bool show) {
    showHitboxes = show;
}

bool Renderer::ShowHitboxes() const {
    return showHitboxes;
}

void Renderer::DrawHitbox(const SpriteInstance &instance) {
    Rectangle bounds = instance.Bounds();

    Vector2 topLeft = SnapToPixel({bounds.x, bounds.y});

    DrawRectangleLines(
        static_cast<int>(topLeft.x),
        static_cast<int>(topLeft.y),
        static_cast<int>(bounds.width),
        static_cast<int>(bounds.height),
        instance.IsColliding() ? RED : WHITE
    );
}

void Renderer::DrawSubmitted() {
    // stable_sort keeps characters with the same Z in the order in which they
    // were submitted. That way the image does not flicker from frame to frame
    // when two characters stand at the same height.
    std::stable_sort(
        submitted.begin(),
        submitted.end(),
        [](const SpriteInstance *a, const SpriteInstance *b) {
            return a->Z() < b->Z();
        }
    );

    for (const SpriteInstance *instance: submitted) {
        Draw(*instance);
    }

    // Second pass, so no sprite covers a frame
    if (showHitboxes) {
        for (const SpriteInstance *instance: submitted) {
            DrawHitbox(*instance);
        }
    }

    // clear keeps the memory, so it only allocates in the first frame
    submitted.clear();
}

void Renderer::SetCameraFocus(Vector2 focus) {
    camera.offset = {
        virtualWidth / 2.0f,
        virtualHeight / 2.0f
    };

    // The camera must sit on whole pixels too, otherwise the whole image
    // shimmers while scrolling instead of just one character
    camera.target = SnapToPixel(focus);
}

void Renderer::SetCameraZoom(float zoom) {
    camera.zoom = zoom > 0.0f ? zoom : 1.0f;
}

float Renderer::CameraZoom() const {
    return camera.zoom;
}

void Renderer::ClearCamera() {
    camera.offset = {0.0f, 0.0f};
    camera.target = {0.0f, 0.0f};
    camera.zoom = 1.0f;
}

// The bar sits above the hitbox, not above the sprite frame: the frames
// have different sizes per character, otherwise the bar would hang directly
// above the head for one and far up in the air for another.
void Renderer::DrawHealthBar(const SpriteInstance &instance, float fraction) {
    fraction = std::clamp(fraction, 0.0f, 1.0f);

    Rectangle box = instance.Bounds();

    Vector2 topLeft = SnapToPixel({
        box.x,
        box.y - HEALTH_BAR_GAP - HEALTH_BAR_HEIGHT
    });

    int x = static_cast<int>(topLeft.x);
    int y = static_cast<int>(topLeft.y);
    int width = static_cast<int>(box.width);

    DrawRectangle(x - 1, y - 1, width + 2, HEALTH_BAR_HEIGHT + 2, HEALTH_BAR_OUTLINE);

    DrawRectangle(x, y, width, HEALTH_BAR_HEIGHT, HEALTH_BAR_EMPTY);

    int filled = static_cast<int>(std::round(width * fraction));

    if (filled > 0) {
        DrawRectangle(x, y, filled, HEALTH_BAR_HEIGHT, HEALTH_BAR_FULL);
    }
}

void Renderer::SubmitHealthBar(const SpriteInstance &instance, float fraction) {
    healthBars.push_back({&instance, fraction});
}

void Renderer::DrawHealthBars() {
    // The same sorting as for the characters, otherwise the bar of the
    // character in the back lies above the one in front
    std::stable_sort(
        healthBars.begin(),
        healthBars.end(),
        [](const HealthBarEntry &a, const HealthBarEntry &b) {
            return a.instance->Z() < b.instance->Z();
        }
    );

    for (const HealthBarEntry &entry: healthBars) {
        DrawHealthBar(*entry.instance, entry.fraction);
    }

    healthBars.clear();
}

void Renderer::BeginDraw() {
    BeginTextureMode(target);

    ClearBackground(BLACK);

    BeginMode2D(camera);

    phase = Phase::World;
}

void Renderer::BeginOverlay() {
    if (phase != Phase::World) {
        return;
    }

    DrawSubmitted();
    DrawHealthBars();

    phase = Phase::Overlay;
}

void Renderer::BeginUI() {
    // Catches up on the overlay layer if it was skipped
    BeginOverlay();

    if (phase == Phase::UI) {
        return;
    }

    // Bars that were only submitted during the overlay layer
    DrawHealthBars();

    EndMode2D();

    phase = Phase::UI;
}

void Renderer::EndDraw() {
    // Catches up on all layers the caller left out
    BeginUI();

    EndTextureMode();

    BeginDrawing();

    ClearBackground(BLACK);

    int scale = Scale();

    float renderWidth = virtualWidth * scale;
    float renderHeight = virtualHeight * scale;

    Vector2 letterbox = Letterbox();

    DrawTexturePro(
        target.texture,
        {
            0,
            0,
            (float) virtualWidth,
            -(float) virtualHeight
        },
        {
            letterbox.x,
            letterbox.y,
            renderWidth,
            renderHeight
        },
        {0, 0},
        0,
        WHITE
    );

    EndDrawing();
}
