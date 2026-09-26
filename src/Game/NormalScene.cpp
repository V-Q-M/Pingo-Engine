#include "NormalScene.h"

#include "Themes.h"
#include "Engine/Engine.h"

static std::string SceneFile(const std::string &id) {
    return "scenes/" + id + ".json";
}

static SceneOptions NormalOptions(Engine &engine, const std::string &id) {
    SceneOptions options;

    options.id = id;
    options.pauseMenu = true;
    options.pauseLeaveLabel = "Return to Main Menu";
    options.movement = false;
    options.developmentZoom = true;

    // The characters here can be moved by scripts, see ScriptHost
    options.scripts = true;

    // Like every scene, a combat chooses its music in the scene bar
    options.music = engine.GetScenes().Choice(id, SceneCatalog::MUSIC_CHOICE);
    options.theme = DefaultTheme();

    return options;
}

NormalScene::NormalScene(Engine &engine, const std::string &id)
    : FieldScene(engine, NormalOptions(engine, id), "Normal") {
    // In the Development mode the characters here can be moved, added and
    // deleted, see FieldScene
    LoadObjects(SceneFile(id));
}

void NormalScene::LeaveFromPauseMenu() {
    engine.EnterEntryPoint();
}
