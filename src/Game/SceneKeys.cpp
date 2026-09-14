#include "SceneKeys.h"

#include "Engine/Engine.h"

void HandleDebugSceneKeys(Engine &engine) {
    if (!engine.GetConfig().HasDebugTools()) {
        return;
    }

    if (IsKeyPressed(KEY_F1)) {
        engine.EnterEntryPoint();
    } else if (IsKeyPressed(KEY_F2)) {
        engine.EnterScene("hub");
    } else if (IsKeyPressed(KEY_F3)) {
        engine.EnterScene("combat");
    }
}
