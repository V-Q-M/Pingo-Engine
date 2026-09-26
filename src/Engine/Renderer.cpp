#include "Renderer.h"

#include <algorithm>
#include <cmath>
#include <map>

// Health and energy bars: heights and distance above the body, plus colors
// from the game's palette. The energy bar is a pixel flatter than the health
// bar, so the two can be told apart at a glance.
constexpr int HEALTH_BAR_HEIGHT = 3;
constexpr int ENERGY_BAR_HEIGHT = 2;
constexpr int BAR_GAP = 3;

// The outline around a bar, on every side
constexpr int BAR_OUTLINE_WIDTH = 1;

// How wide a bar gets: four points of health or energy per pixel, so a
// character with more of it also shows more bar
constexpr float BAR_POINTS_PER_PIXEL = 4.0f;
constexpr int BAR_MIN_WIDTH = 6;
constexpr int BAR_MAX_WIDTH = 64;

constexpr Color BAR_OUTLINE{24, 20, 37, 255};

constexpr Color HEALTH_BAR_EMPTY{120, 28, 28, 255};
constexpr Color HEALTH_BAR_FULL{61, 255, 20, 255};

constexpr Color ENERGY_BAR_EMPTY{28, 40, 96, 255};
constexpr Color ENERGY_BAR_FULL{64, 164, 255, 255};

// However small the bounce makes a shadow, this much of it stays
constexpr float SHADOW_MIN_SCALE = 0.2f;

// Ring around the shadow of the selected character
constexpr float SELECTOR_GAP = 2.0f;
constexpr float SELECTOR_MIN_RADIUS = 2.0f;

constexpr Color SELECTOR_RING{255, 229, 26, 220};
constexpr Color SELECTOR_SHADE{24, 20, 37, 160};

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
int Renderer::GetScale() const {
    return std::max(
        1,
        static_cast<int>(std::floor(std::min(
            (float) GetScreenWidth() / virtualWidth,
            (float) GetScreenHeight() / virtualHeight
        )))
    );
}

Vector2 Renderer::GetLetterbox() const {
    int scale = GetScale();

    return {
        (GetScreenWidth() - virtualWidth * scale) / 2.0f,
        (GetScreenHeight() - virtualHeight * scale) / 2.0f
    };
}

Vector2 Renderer::ScreenToViewport(Vector2 screenPosition) const {
    int scale = GetScale();
    Vector2 letterbox = GetLetterbox();

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
// A ring around the shadow, so it fits every character. Objects without a
// shadow, e.g. dummies, get one from the width of their hitbox.
void Renderer::DrawSelector(const SpriteInstance &character) {
    if (!character.IsSelected()) {
        return;
    }

    const AnimatedSprite &sprite = character.Sprite();

    Vector2 feet = SnapToPixel(character.Position());

    float radiusX = sprite.ShadowWidth();
    float radiusY = sprite.ShadowHeight();
    float offsetX = sprite.ShadowOffsetX();
    float offsetY = sprite.ShadowOffsetY();

    if (radiusX <= 0.0f || radiusY <= 0.0f) {
        Rectangle box = character.Bounds();

        radiusX = box.width / 2.0f;
        radiusY = std::max(box.width / 4.0f, SELECTOR_MIN_RADIUS);
        offsetX = box.x + box.width / 2.0f - feet.x;
        offsetY = box.y + box.height - feet.y;
    }

    int x = static_cast<int>(feet.x + offsetX);
    int y = static_cast<int>(feet.y + offsetY);

    // A dark ring below the bright one, so it stands out on light ground too
    DrawEllipseLines(x, y, radiusX + SELECTOR_GAP + 1.0f, radiusY + SELECTOR_GAP + 1.0f, SELECTOR_SHADE);
    DrawEllipseLines(x, y, radiusX + SELECTOR_GAP, radiusY + SELECTOR_GAP, SELECTOR_RING);
}

void Renderer::DrawShadow(const SpriteInstance &character) {
    const auto &sprite = character.Sprite();

    // Without a shadow size, e.g. for dummy objects, there is no shadow
    if (sprite.ShadowWidth() <= 0.0f || sprite.ShadowHeight() <= 0.0f) {
        return;
    }

    Vector2 feet = SnapToPixel(character.Position());

    float width = sprite.ShadowWidth();
    float height = sprite.ShadowHeight();

    // While the character is up in its animation the shadow shrinks, as if it
    // moved away from the ground. It keeps its shape, so both sides shrink.
    // The bounce counts the pixels the whole shadow loses, and width and
    // height are radii: half of it comes off each.
    if (sprite.ShadowBounce() > 0.0f) {
        float phase = (1.0f - std::cos(2.0f * PI * sprite.AnimationPhase())) / 2.0f;
        float scale = std::max(1.0f - sprite.ShadowBounce() * phase / (2.0f * width), SHADOW_MIN_SCALE);

        width *= scale;
        height *= scale;
    }

    DrawEllipse(
        static_cast<int>(feet.x + sprite.ShadowOffsetX()),
        static_cast<int>(feet.y + sprite.ShadowOffsetY()),
        width,
        height,
        Color{0, 0, 0, 80}
    );
}

void Renderer::Draw(const SpriteInstance &character) {
    // The ring lies above the shadow and below the character
    DrawShadow(character);
    DrawSelector(character);
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

// The bars sit above the hitbox, not above the sprite frame: the frames
// have different sizes per character, otherwise a bar would hang directly
// above the head for one and far up in the air for another.
// The bars of one character are centered over it, each in its own width
void Renderer::DrawBar(const BarEntry &entry, int offset) {
    const float fraction = std::clamp(entry.fraction, 0.0f, 1.0f);

    Rectangle box = entry.instance->Bounds();

    Vector2 topLeft = SnapToPixel({
        box.x + (box.width - static_cast<float>(entry.width)) / 2.0f,
        box.y - BAR_GAP - static_cast<float>(entry.height + offset)
    });

    int x = static_cast<int>(topLeft.x);
    int y = static_cast<int>(topLeft.y);

    DrawRectangle(
        x - BAR_OUTLINE_WIDTH,
        y - BAR_OUTLINE_WIDTH,
        entry.width + 2 * BAR_OUTLINE_WIDTH,
        entry.height + 2 * BAR_OUTLINE_WIDTH,
        BAR_OUTLINE
    );

    DrawRectangle(x, y, entry.width, entry.height, entry.empty);

    int filled = static_cast<int>(std::round(entry.width * fraction));

    if (filled > 0) {
        DrawRectangle(x, y, filled, entry.height, entry.full);
    }
}

// A bar shows how much a character can have at most: more points, more pixels
static int BarWidth(int maximum) {
    int width = static_cast<int>(std::round(static_cast<float>(maximum) / BAR_POINTS_PER_PIXEL));

    return std::clamp(width, BAR_MIN_WIDTH, BAR_MAX_WIDTH);
}

void Renderer::SubmitHealthBar(const SpriteInstance &instance, float fraction, int maximum) {
    bars.push_back({&instance, fraction, BarWidth(maximum), HEALTH_BAR_HEIGHT, HEALTH_BAR_EMPTY, HEALTH_BAR_FULL});
}

void Renderer::SubmitEnergyBar(const SpriteInstance &instance, float fraction, int maximum) {
    bars.push_back({&instance, fraction, BarWidth(maximum), ENERGY_BAR_HEIGHT, ENERGY_BAR_EMPTY, ENERGY_BAR_FULL});
}

void Renderer::DrawBars() {
    // The same sorting as for the characters, otherwise the bar of the
    // character in the back lies above the one in front. stable_sort keeps the
    // order in which the bars of one character were submitted.
    std::stable_sort(
        bars.begin(),
        bars.end(),
        [](const BarEntry &a, const BarEntry &b) {
            return a.instance->Z() < b.instance->Z();
        }
    );

    // Every further bar of the same character lands above the ones before it
    std::map<const SpriteInstance *, int> stacked;

    for (const BarEntry &entry: bars) {
        int &offset = stacked[entry.instance];

        DrawBar(entry, offset);

        offset += entry.height + 2 * BAR_OUTLINE_WIDTH;
    }

    bars.clear();
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
    DrawBars();

    phase = Phase::Overlay;
}

void Renderer::BeginUI() {
    // Catches up on the overlay layer if it was skipped
    BeginOverlay();

    if (phase == Phase::UI) {
        return;
    }

    // Bars that were only submitted during the overlay layer
    DrawBars();

    EndMode2D();

    phase = Phase::UI;
}

void Renderer::EndDraw(const std::function<void()> &screenLayer) {
    // Catches up on all layers the caller left out
    BeginUI();

    EndTextureMode();

    BeginDrawing();

    ClearBackground(BLACK);

    int scale = GetScale();

    float renderWidth = virtualWidth * scale;
    float renderHeight = virtualHeight * scale;

    Vector2 letterbox = GetLetterbox();

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

    // Above the finished picture, in the pixels of the window
    if (screenLayer) {
        screenLayer();
    }

    EndDrawing();
}
