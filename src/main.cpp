#include <string>

#include "Engine/Config.h"
#include "Engine/Engine.h"
#include "Game/GameScenes.h"
#include "Game/SplashScene.h"
#include "Game/SceneKeys.h"

#ifdef PINGO_SCREENSHOT_HARNESS
#include "../tools/screenshots/ScreenshotHarness.h"
#endif

int main(int argc, char **argv) {
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

#ifdef PINGO_SCREENSHOT_HARNESS
    // --harness <commands> <output folder>: the harness runs the frames instead
    // of the normal loop, see tools/screenshots
    if (argc >= 4 && std::string(argv[1]) == "--harness") {
        return ScreenshotHarness::Run(engine, argv[2], argv[3]);
    }
#else
    (void) argc;
    (void) argv;
#endif

    engine.Run();

    return 0;
}
