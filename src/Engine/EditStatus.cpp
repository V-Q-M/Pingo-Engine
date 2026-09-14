#include "EditStatus.h"

#include <string>

#include "Engine.h"

void DrawEditStatus(Engine &engine, bool modified, bool canUndo, bool canRedo) {
    const FontRenderer &font = engine.GetFont();
    const Theme &theme = engine.GetTheme();

    float right = static_cast<float>(engine.GetRenderer().GetWidth() - 8);
    float top = engine.UiTop();

    // Below the engine's FPS display, if it is on
    if (engine.GetConfig().HasDebugTools() && engine.GetSettings().showFps) {
        top += font.LetterHeight() + 4.0f;
    }

    auto drawRight = [&](const std::string &text, int variant) {
        float width = static_cast<float>(font.Measure(text, TextSpacing::Narrow));

        font.Draw(text, {right - width, top}, variant, TextSpacing::Narrow);

        top += font.LetterHeight() + 4.0f;
    };

    if (modified) {
        drawRight("Changes detected", theme.hoverVariant);
    }

    std::string keys;

    if (canUndo) {
        keys = "Z Undo";
    }

    if (canRedo) {
        keys += keys.empty() ? "Y Redo" : "  Y Redo";
    }

    if (!keys.empty()) {
        drawRight(keys, theme.textVariant);
    }
}
