#include "SplashScene.h"

#include <algorithm>
#include <string>

#include "Themes.h"
#include "Engine/Engine.h"
#include "Engine/Version.h"

// The character in the logo, shown with its idle animation
constexpr const char *SPLASH_CHARACTER = "characters/pingo.json";

// Double size, so Pingo fills the center. Whole numbers keep every pixel sharp.
constexpr float SPLASH_ZOOM = 2.0f;

// Seconds for fading in and out, and in between for the still logo
constexpr float SPLASH_FADE_TIME = 0.6f;
constexpr float SPLASH_HOLD_TIME = 2.2f;
constexpr float SPLASH_TOTAL_TIME = 2.0f * SPLASH_FADE_TIME + SPLASH_HOLD_TIME;

// Longest time step per frame. The first frame after loading often takes
// long, without a limit the fade in would be over before you see it.
constexpr float SPLASH_MAX_STEP = 1.0f / 30.0f;

// Space between Pingo and the lettering, and between the version label and the edge
constexpr float SPLASH_TEXT_GAP = 4.0f;
constexpr float SPLASH_MARGIN = 8.0f;

constexpr const char *SPLASH_MADE_WITH = "Made with ";
constexpr const char *SPLASH_ENGINE_NAME = "Pingo Engine";

static SceneOptions SplashOptions() {
    SceneOptions options;

    // Without an id: the splash screen has no tab and no background
    options.pauseMenu = false;
    options.movement = false;
    options.theme = DefaultTheme();

    return options;
}

SplashScene::SplashScene(Engine &engine)
    : Scene(engine, SplashOptions()),
      pingo(engine.GetAssets(), CharacterDefinition::Load(SPLASH_CHARACTER), {0.0f, 0.0f}) {
}

void SplashScene::Enter() {
    // Pingo stands at the origin of the world, the camera looks at it zoomed in
    Renderer &renderer = engine.GetRenderer();

    renderer.SetCameraFocus({0.0f, 0.0f});
    renderer.SetCameraZoom(SPLASH_ZOOM);
}

void SplashScene::Update(float dt) {
    if (finished) {
        return;
    }

    float step = std::min(dt, SPLASH_MAX_STEP);

    time += step;

    pingo.GetCharacter().Sprite().Update(step);

    bool skipRequested = GetKeyPressed() != 0 ||
                         IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ||
                         IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);

    if (skipRequested) {
        Skip();
    }

    if (time < SPLASH_TOTAL_TIME) {
        return;
    }

    finished = true;

    // If the entry point was deleted, it starts with the first scene
    if (!engine.EnterEntryPoint()) {
        engine.GetScenes().Enter(engine, 0);
    }
}

void SplashScene::Skip() {
    float fadeOutStart = SPLASH_FADE_TIME + SPLASH_HOLD_TIME;

    // During the fade in it jumps to the point of the fade out with the same
    // brightness, afterwards directly to the start of the fade out
    float target = fadeOutStart + SPLASH_FADE_TIME - std::min(time, SPLASH_FADE_TIME);

    time = std::max(time, target);
}

float SplashScene::Shade() const {
    if (time < SPLASH_FADE_TIME) {
        return 1.0f - time / SPLASH_FADE_TIME;
    }

    float fadeOutStart = SPLASH_FADE_TIME + SPLASH_HOLD_TIME;

    if (time > fadeOutStart) {
        return std::min(1.0f, (time - fadeOutStart) / SPLASH_FADE_TIME);
    }

    return 0.0f;
}

bool SplashScene::IsFinished() const {
    return finished;
}

void SplashScene::Draw() {
    const AnimatedSprite &sprite = pingo.GetCharacter().Sprite();

    // Renderer::Draw puts the feet at the position. That way the center of the
    // frame lies at the origin, exactly in the middle of the image.
    engine.GetRenderer().Draw(sprite, {0.0f, sprite.Height() / 2.0f});
}

void SplashScene::DrawUI() {
    Renderer &renderer = engine.GetRenderer();
    const FontRenderer &font = engine.GetFont();

    float width = static_cast<float>(renderer.GetWidth());
    float height = static_cast<float>(renderer.GetHeight());

    // The lettering sits centered below the enlarged frame, "Pingo Engine"
    // stands out in white from the grey rest
    const AnimatedSprite &sprite = pingo.GetCharacter().Sprite();

    std::string madeWith = SPLASH_MADE_WITH;
    std::string engineName = SPLASH_ENGINE_NAME;

    float textWidth = static_cast<float>(font.Measure(madeWith + engineName, TextSpacing::Narrow));

    Vector2 textPosition = Renderer::SnapToPixel({
        (width - textWidth) / 2.0f,
        height / 2.0f + static_cast<float>(sprite.Height()) * SPLASH_ZOOM / 2.0f + SPLASH_TEXT_GAP
    });

    font.Draw(madeWith, textPosition, FontVariant::Grey, TextSpacing::Narrow);

    font.Draw(
        engineName,
        {textPosition.x + static_cast<float>(font.Measure(madeWith, TextSpacing::Narrow)), textPosition.y},
        FontVariant::White,
        TextSpacing::Narrow
    );

    std::string version = PINGO_ENGINE_VERSION;

    font.Draw(
        version,
        {
            width - SPLASH_MARGIN - static_cast<float>(font.Measure(version, TextSpacing::Narrow)),
            height - SPLASH_MARGIN - static_cast<float>(font.LetterHeight())
        },
        FontVariant::Grey,
        TextSpacing::Narrow
    );

    // The black overlay lies above logo and texts
    unsigned char alpha = static_cast<unsigned char>(Shade() * 255.0f);

    if (alpha > 0) {
        DrawRectangle(0, 0, renderer.GetWidth(), renderer.GetHeight(), {0, 0, 0, alpha});
    }
}
