#include <string>

#include "Engine/Config.h"
#include "Engine/Engine.h"
#include "Game/GameScenes.h"
#include "Game/SplashScene.h"
#include "Game/SceneKeys.h"

int main() {
    // CMake sets the mode via PINGO_BUILD_MODE. For testing one can also be set
    // here, e.g. Config(BuildMode::Debugging)
    Config config = Config::Default();

    EngineOptions options;
    options.title = "Pingo Legends";

    if (!config.IsRelease()) {
        options.title += std::string(" [") + config.ModeName() + "]";
    }

    Engine engine(config, options);

    // The scenes are stored in assets/scenes/scenes.json, in the Development
    // mode also shown as tabs at the top
    RegisterGameScenes(engine.GetScenes());

    // F1 to F3 change the scene, except in the Release build
    engine.SetGlobalUpdate(HandleDebugSceneKeys);

    if (config.IsDevelopment()) {
        // Development starts right at the entry point. If there is none, it
        // starts with the first scene.
        if (!engine.EnterEntryPoint()) {
            engine.GetScenes().Enter(engine, 0);
        }
    } else {
        // Like Unity: the engine logo first, then the entry point
        engine.ChangeScene<SplashScene>();
    }

    engine.Run();

    return 0;
}
