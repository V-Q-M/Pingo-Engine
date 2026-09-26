#include "SceneKeys.h"

#include <algorithm>

#include "Engine/Engine.h"

// F1 belongs to the first scene, F9 to the ninth one
constexpr int SCENE_KEY_COUNT = 9;

static bool IsAltDown() {
    return IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT);
}

// The scenes in the order of the scene bar, so a new scene gets its key
// without anything else to do
void HandleDebugSceneKeys(Engine &engine) {
    if (!engine.GetConfig().HasDebugTools()) {
        return;
    }

    const SceneCatalog &scenes = engine.GetScenes();
    const std::size_t count = std::min<std::size_t>(scenes.Count(), SCENE_KEY_COUNT);

    for (std::size_t index = 0; index < count; ++index) {
        const int offset = static_cast<int>(index);

        // Alt and a number work as well, digits alone belong to the tile editor
        const bool pressed = engine.WasKeyPressed(KEY_F1 + offset) ||
                             (IsAltDown() && engine.WasKeyPressed(KEY_ONE + offset));

        if (pressed) {
            scenes.Enter(engine, index);
            return;
        }
    }
}
