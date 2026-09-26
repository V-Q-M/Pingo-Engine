#include "ScriptHost.h"

#include "raylib.h"

#include "Api/ScriptRegistry.h"
#include "ScriptFiles.h"

// The registry only knows scripts in general. A script of the wrong kind,
// e.g. a Script assigned to a tile, would not find what it expects, so it is
// not created at all.
template<typename T>
static std::unique_ptr<T> CreateAs(const std::string &scene, const std::string &name) {
    std::unique_ptr<ScriptBase> created = ScriptRegistry::Create(scene, name);

    if (dynamic_cast<T *>(created.get()) == nullptr) {
        return nullptr;
    }

    return std::unique_ptr<T>(static_cast<T *>(created.release()));
}

void ScriptHost::Load(const std::string &scene, ScriptWorld &world) {
    main = CreateAs<Script>(scene, ScriptFiles::MAIN);

    if (main == nullptr) {
        TraceLog(LOG_INFO, "SCRIPTS: [%s] hat kein gebautes %s", scene.c_str(), ScriptFiles::MAIN);
        return;
    }

    main->scene.Attach(&world);
}

bool ScriptHost::AddTileScript(const std::string &scene,
                               const std::string &name,
                               TileWorld &world,
                               int column,
                               int row) {
    std::unique_ptr<TileScript> script = CreateAs<TileScript>(scene, name);

    if (script == nullptr) {
        TraceLog(LOG_INFO, "SCRIPTS: [%s/%s] ist kein gebautes Tile-Script", scene.c_str(), name.c_str());
        return false;
    }

    script->tile = Tile(&world, column, row);
    script->map.Attach(&world);

    tileScripts.push_back(std::move(script));

    return true;
}

void ScriptHost::Clear() {
    main.reset();
    tileScripts.clear();
}

bool ScriptHost::HasMain() const {
    return main != nullptr;
}

void ScriptHost::Update(float dt) {
    if (main != nullptr) {
        main->Update(dt);
    }

    for (const std::unique_ptr<TileScript> &script: tileScripts) {
        script->Update(dt);
    }
}
