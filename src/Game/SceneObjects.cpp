#include "SceneObjects.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <system_error>

#include <nlohmann/json.hpp>

#include "ObjectEffects.h"
#include "Engine/AssetFile.h"
#include "Engine/ColorHex.h"

// ordered_json keeps the order of the keys, so the saved file stays as
// readable as it was written
using json = nlohmann::ordered_json;

// A square with "size" and "color", or a sprite sheet with "texture" and its
// frames, like in a character file
static CharacterLook LookFromJson(const json &item) {
    CharacterLook look;

    if (item.contains("texture")) {
        look.texture = item.at("texture").get<std::string>();
        look.frameWidth = item.value("frameWidth", look.frameWidth);
        look.frameHeight = item.value("frameHeight", look.frameHeight);
        look.frameCount = item.value("frameCount", look.frameCount);

        // The rows this character animates during an effect
        if (item.contains("effects")) {
            for (const ObjectEffect &effect: ObjectEffects::All()) {
                if (item.at("effects").contains(effect.id)) {
                    look.effectRows[effect.id] = item.at("effects").at(effect.id).get<int>();
                }
            }
        }
    } else {
        look.frameWidth = item.value("size", look.frameWidth);
        look.frameHeight = look.frameWidth;
        look.color = ColorFromHex(item.value("color", std::string()));
    }

    return look;
}

// A shadow or hitbox with width, height and offsets in whole pixels
static CharacterBox BoxFromJson(const json &item) {
    CharacterBox box;

    box.width = item.value("width", box.width);
    box.height = item.value("height", box.height);
    box.offsetX = item.value("offsetX", box.offsetX);
    box.offsetY = item.value("offsetY", box.offsetY);

    return box;
}

static json BoxToJson(const CharacterBox &box) {
    json item;

    item["width"] = box.width;
    item["height"] = box.height;
    item["offsetX"] = box.offsetX;
    item["offsetY"] = box.offsetY;

    return item;
}

static json LookToJson(const CharacterLook &look) {
    json item;

    if (look.IsSquare()) {
        item["size"] = look.frameWidth;
        item["color"] = ColorToHex(look.color);
    } else {
        item["texture"] = look.texture;
        item["frameWidth"] = look.frameWidth;
        item["frameHeight"] = look.frameHeight;
        item["frameCount"] = look.frameCount;

        if (!look.effectRows.empty()) {
            json effects;

            for (const ObjectEffect &effect: ObjectEffects::All()) {
                auto row = look.effectRows.find(effect.id);

                if (row != look.effectRows.end()) {
                    effects[effect.id] = row->second;
                }
            }

            item["effects"] = effects;
        }
    }

    return item;
}

// The group of an entry. Older files have a team instead: heroes become the
// allies, everyone else the enemies.
static std::string GroupOf(const json &item) {
    if (item.contains("group") && item["group"].is_string()) {
        return item["group"].get<std::string>();
    }

    if (item.contains("team") && item["team"] == "heroes") {
        return ObjectGroups::ALLY;
    }

    return ObjectGroups::ENEMY;
}

bool ObjectPlacement::operator==(const ObjectPlacement &other) const {
    return character == other.character &&
           group == other.group &&
           position.x == other.position.x &&
           position.y == other.position.y &&
           health == other.health &&
           overrides == other.overrides;
}

bool SceneObjects::Load(const std::string &filename, std::vector<ObjectPlacement> &placements) {
    placements.clear();

    std::ifstream file("assets/" + filename);

    if (!file) {
        TraceLog(LOG_WARNING, "SCENE: [%s] Datei nicht gefunden, Szene startet leer", filename.c_str());
        return true;
    }

    ObjectGroups groups = ObjectGroups::Load();

    try {
        json j = json::parse(file);

        for (const json &item: j.at("objects")) {
            ObjectPlacement placement;

            placement.character = item.at("character").get<std::string>();
            placement.group = GroupOf(item);
            placement.position = {item.value("x", 0.0f), item.value("y", 0.0f)};
            placement.health = item.value("health", placement.health);
            placement.overrides.name = item.value("name", std::string());

            // Health and energy this character can have at most, where they
            // differ from its type
            if (item.contains("maxHealth")) {
                placement.overrides.maxHealth = item.at("maxHealth").get<int>();
            }

            if (item.contains("maxEnergy")) {
                placement.overrides.maxEnergy = item.at("maxEnergy").get<int>();
            }

            if (item.contains("look")) {
                CharacterLook look = LookFromJson(item.at("look"));

                if (!look.IsSquare() && !std::filesystem::exists("assets/" + look.texture)) {
                    TraceLog(LOG_WARNING, "SCENE: [%s] %s nicht gefunden, Figur sieht aus wie ihr Typ",
                             filename.c_str(), look.texture.c_str());
                } else {
                    placement.overrides.look = look;
                }
            }

            // Shadow and hitbox are always stored together, as the advanced
            // settings of this character
            if (item.contains("shadow") && item.contains("hitbox")) {
                CharacterBody body;

                body.shadow = BoxFromJson(item.at("shadow"));
                body.hitbox = BoxFromJson(item.at("hitbox"));
                body.shadowBounce = item.value("shadowBounce", body.shadowBounce);

                placement.overrides.body = body;
            }

            if (groups.IndexOf(placement.group) < 0) {
                TraceLog(LOG_WARNING, "SCENE: [%s] Gruppe \"%s\" unbekannt, nutze enemy",
                         filename.c_str(), placement.group.c_str());
                placement.group = ObjectGroups::ENEMY;
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

        if (!placement.overrides.name.empty()) {
            item["name"] = placement.overrides.name;
        }

        item["group"] = placement.group;

        // Pixel art stands on whole pixels
        item["x"] = std::lround(placement.position.x);
        item["y"] = std::lround(placement.position.y);

        if (placement.health >= 0) {
            item["health"] = placement.health;
        }

        if (placement.overrides.maxHealth) {
            item["maxHealth"] = *placement.overrides.maxHealth;
        }

        if (placement.overrides.maxEnergy) {
            item["maxEnergy"] = *placement.overrides.maxEnergy;
        }

        if (placement.overrides.look) {
            item["look"] = LookToJson(*placement.overrides.look);
        }

        if (placement.overrides.body) {
            item["shadow"] = BoxToJson(placement.overrides.body->shadow);
            item["hitbox"] = BoxToJson(placement.overrides.body->hitbox);

            if (placement.overrides.body->shadowBounce != 0) {
                item["shadowBounce"] = placement.overrides.body->shadowBounce;
            }
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

std::set<std::string> SceneObjects::UsedGroups() {
    std::set<std::string> used;

    std::error_code error;

    for (const auto &entry: std::filesystem::directory_iterator("assets/scenes", error)) {
        if (entry.path().extension() != ".json") {
            continue;
        }

        std::ifstream file(entry.path());
        json j = json::parse(file, nullptr, false);

        // Other files in the folder, like scenes.json, have no characters
        if (!j.is_object() || !j.contains("objects") || !j["objects"].is_array()) {
            continue;
        }

        for (const json &item: j["objects"]) {
            if (item.is_object()) {
                used.insert(GroupOf(item));
            }
        }
    }

    return used;
}
