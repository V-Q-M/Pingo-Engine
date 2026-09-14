#include "SceneObjects.h"

#include <cmath>
#include <filesystem>
#include <fstream>

#include <nlohmann/json.hpp>

#include "Engine/AssetFile.h"

// ordered_json keeps the order of the keys, so the saved file stays as
// readable as it was written
using json = nlohmann::ordered_json;

bool ObjectPlacement::operator==(const ObjectPlacement &other) const {
    return character == other.character &&
           team == other.team &&
           position.x == other.position.x &&
           position.y == other.position.y &&
           health == other.health;
}

bool SceneObjects::Load(const std::string &filename, std::vector<ObjectPlacement> &placements) {
    placements.clear();

    std::ifstream file("assets/" + filename);

    if (!file) {
        TraceLog(LOG_WARNING, "SCENE: [%s] Datei nicht gefunden, Szene startet leer", filename.c_str());
        return true;
    }

    try {
        json j = json::parse(file);

        for (const json &item: j.at("objects")) {
            ObjectPlacement placement;

            placement.character = item.at("character").get<std::string>();
            placement.team = item.value("team", placement.team);
            placement.position = {item.value("x", 0.0f), item.value("y", 0.0f)};
            placement.health = item.value("health", placement.health);

            if (placement.team != "heroes" && placement.team != "enemies") {
                TraceLog(LOG_WARNING, "SCENE: [%s] Team \"%s\" unbekannt, nutze enemies",
                         filename.c_str(), placement.team.c_str());
                placement.team = "enemies";
            }

            if (!std::filesystem::exists("assets/" + placement.character)) {
                TraceLog(LOG_WARNING, "SCENE: [%s] %s nicht gefunden, Figur uebersprungen",
                         filename.c_str(), placement.character.c_str());
                continue;
            }

            placements.push_back(placement);
        }
    } catch (const json::exception &error) {
        TraceLog(LOG_WARNING, "SCENE: [%s] %s", filename.c_str(), error.what());
        placements.clear();
        return false;
    }

    return true;
}

SceneSettings SceneObjects::LoadSettings(const std::string &filename) {
    SceneSettings settings;

    std::ifstream file("assets/" + filename);

    if (!file) {
        return settings;
    }

    try {
        json j = json::parse(file);

        settings.music = j.value("music", std::string());
    } catch (const json::exception &error) {
        TraceLog(LOG_WARNING, "SCENE: [%s] %s", filename.c_str(), error.what());
    }

    return settings;
}

bool SceneObjects::Save(const std::string &filename, const std::vector<ObjectPlacement> &placements) {
    json objects = json::array();

    for (const ObjectPlacement &placement: placements) {
        json item;

        item["character"] = placement.character;
        item["team"] = placement.team;

        // Pixel art stands on whole pixels
        item["x"] = std::lround(placement.position.x);
        item["y"] = std::lround(placement.position.y);

        if (placement.health >= 0) {
            item["health"] = placement.health;
        }

        objects.push_back(item);
    }

    json j = json::object();

    // Take over the remaining entries of the file, only the characters are new
    std::ifstream existing("assets/" + filename);

    if (existing) {
        try {
            json previous = json::parse(existing);

            if (previous.is_object()) {
                j = previous;
            }
        } catch (const json::exception &) {
            // SceneObjects::Load does not even read a broken file
        }
    }

    j["objects"] = objects;

    bool saved = SaveAsset(filename, j.dump(2) + "\n");

    if (saved) {
        TraceLog(LOG_INFO, "SCENE: [%s] gespeichert", filename.c_str());
    } else {
        TraceLog(LOG_WARNING, "SCENE: [%s] konnte nicht gespeichert werden", filename.c_str());
    }

    return saved;
}
