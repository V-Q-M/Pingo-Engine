#include "NormalScene.h"

#include "Themes.h"
#include "Engine/Engine.h"

static std::string SceneFile(const std::string &id) {
    return "scenes/" + id + ".json";
}

static SceneOptions NormalOptions(const std::string &id, const SceneSettings &settings) {
    SceneOptions options;

    options.id = id;
    options.pauseMenu = true;
    options.pauseLeaveLabel = "Return to Main Menu";
    options.movement = false;
    options.developmentZoom = true;
    options.music = settings.music;
    options.theme = DefaultTheme();

    return options;
}

NormalScene::NormalScene(Engine &engine, const std::string &id)
    : NormalScene(engine, id, SceneObjects::LoadSettings(SceneFile(id))) {
}

NormalScene::NormalScene(Engine &engine, const std::string &id, const SceneSettings &settings)
    : FieldScene(engine, NormalOptions(id, settings), "Normal") {
    // In the Development mode the characters here can be moved, added and
    // deleted, see FieldScene
    LoadObjects(SceneFile(id));
}

void NormalScene::LeaveFromPauseMenu() {
    engine.EnterEntryPoint();
}
