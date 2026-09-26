#include "GameScenes.h"

#include "MenuScene.h"
#include "NormalScene.h"
#include "TilesetScene.h"
#include "Engine/AssetFile.h"
#include "Engine/Engine.h"
#include "Engine/SceneCatalog.h"
#include "Engine/TileMap.h"
#include "SceneObjects.h"

constexpr const char *SCENES_FILE = "scenes/scenes.json";

// Name of the tileset type in the catalog
constexpr const char *TILESET_TYPE = "tileset";

// Size of new maps: this many 32 pixel cells fill the viewport
constexpr int TILESET_COLUMNS = 10;
constexpr int TILESET_ROWS = 6;
constexpr int TILESET_MAX_SIZE = 99;

// Tile atlases and tile data are chosen separately, so they can be combined
constexpr const char *TILE_ATLAS_FOLDER = "sprites/tilesets";
constexpr const char *TILE_DATA_FOLDER = "tilesets";

constexpr const char *TILE_ATLAS_INITIAL = "sprites/tilesets/tiles_atlas.png";
constexpr const char *TILE_DATA_INITIAL = "tilesets/overworld.json";

// What tileset scenes played before the music moved into the SceneCatalog
constexpr const char *LEGACY_TILESET_MUSIC = "music/hub.wav";

// The map from a file, an empty one if it does not exist yet. Reading the
// size does not need a real TileSet.
static TileMap LoadMap(const std::string &file) {
    return AssetExists(file) ? TileMap::Load(file, TileSet()) : TileMap::Empty(file, 0, 0);
}

static SceneType MenuType() {
    SceneType type;

    type.label = "Menu";
    type.enter = [](Engine &engine, const std::string &id) { engine.ChangeScene<MenuScene>(id); };
    type.files = {"scenes/{id}.json"};

    // Default is the main menu as we know it
    type.templates = {
        {"None", {}, ""},
        {"Default", {"templates/menu/default.json"}, "map.png"}
    };

    // There is always a menu, one of them is the entry point
    type.required = true;
    type.entryPoint = true;

    return type;
}

static SceneType TilesetType() {
    SceneType type;

    type.label = "Tileset";
    type.enter = [](Engine &engine, const std::string &id) { engine.ChangeScene<TilesetScene>(id); };
    type.files = {"maps/{id}.txt"};

    // Default is exactly the hub
    type.templates = {
        {"None", {}, ""},
        {"Default", {"templates/tileset/default.txt"}, ""}
    };

    // Instead of a background the map shows its tiles
    type.background = false;

    type.choices = {
        {
            TilesetScene::TILE_ATLAS_CHOICE,
            "Tile atlas",
            [] { return AssetFilesIn(TILE_ATLAS_FOLDER, ".png"); },
            TILE_ATLAS_INITIAL
        },
        {
            TilesetScene::TILE_DATA_CHOICE,
            "Tile data",
            [] { return AssetFilesIn(TILE_DATA_FOLDER, ".json"); },
            TILE_DATA_INITIAL
        }
    };

    // Same order as the values in the functions below
    type.options = {
        {"Columns", 1, TILESET_MAX_SIZE, TILESET_COLUMNS},
        {"Rows", 1, TILESET_MAX_SIZE, TILESET_ROWS}
    };

    type.readOptions = [](const std::vector<std::string> &files) {
        if (files.empty()) {
            return std::vector<int>();
        }

        TileMap map = LoadMap(files[0]);

        return std::vector<int>{map.Columns(), map.Rows()};
    };

    type.optionsWarning = [](const std::vector<std::string> &files, const std::vector<int> &values) {
        if (files.empty()) {
            return std::string();
        }

        TileMap map = LoadMap(files[0]);

        if (values.size() == 2 && (values[0] < map.Columns() || values[1] < map.Rows())) {
            return std::string("Shrinking can erase some tiles.");
        }

        return std::string();
    };

    // Without a template an empty map is created, enlarging adds empty cells
    type.applyOptions = [](const std::vector<std::string> &files, const std::vector<int> &values) {
        if (files.empty() || values.size() != 2) {
            return false;
        }

        TileMap map = LoadMap(files[0]);

        map.Resize(values[0], values[1]);

        return map.SaveAsset();
    };

    return type;
}

static SceneType NormalType() {
    SceneType type;

    type.label = "Normal";
    type.enter = [](Engine &engine, const std::string &id) { engine.ChangeScene<NormalScene>(id); };
    type.files = {"scenes/{id}.json"};

    // Default is the combat with its characters, background and music
    type.templates = {
        {"None", {}, ""},
        {"Default", {"templates/normal/default.json"}, "map.png"}
    };

    return type;
}

// Scenes used to carry their music in their own file, tileset scenes played
// the hub track. Scenes from back then take that over once, everything after
// that is chosen in the scene bar.
static void MigrateSceneMusic(SceneCatalog &catalog) {
    for (std::size_t index = 0; index < catalog.Count(); index++) {
        const SceneEntry &entry = catalog.Entries()[index];

        if (entry.choices.find(SceneCatalog::MUSIC_CHOICE) != entry.choices.end()) {
            continue;
        }

        std::string music = SceneObjects::LoadSettings("scenes/" + entry.id + ".json").music;

        if (music.empty() && entry.type == TILESET_TYPE) {
            music = LEGACY_TILESET_MUSIC;
        }

        catalog.SetChoice(index, SceneCatalog::MUSIC_CHOICE, music);
    }
}

void RegisterGameScenes(SceneCatalog &catalog) {
    // The types appear in this order in the window for new scenes
    catalog.RegisterType("menu", MenuType());
    catalog.RegisterType(TILESET_TYPE, TilesetType());
    catalog.RegisterType("normal", NormalType());

    // Applies if the file is missing
    catalog.Load(SCENES_FILE, {
        {"main_menu", "Main Menu", "menu", "map.png", false, true, {}},
        {"hub", "Hub", "tileset", "", false, false, {}},
        {"combat", "Combat", "normal", "map.png", false, false, {}}
    });

    MigrateSceneMusic(catalog);
}
