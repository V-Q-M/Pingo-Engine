#include "CharacterDefinition.h"

#include <cstdio>
#include <fstream>

#include <nlohmann/json.hpp>

#include "Engine/AssetFile.h"

using json = nlohmann::json;

static std::string ColorToHex(Color color) {
    char text[8];
    std::snprintf(text, sizeof(text), "#%02X%02X%02X", color.r, color.g, color.b);
    return text;
}

// "#RRGGBB", white for anything else
static Color ColorFromHex(const std::string &text) {
    unsigned int r = 255, g = 255, b = 255;

    if (text.size() != 7 || std::sscanf(text.c_str(), "#%02x%02x%02x", &r, &g, &b) != 3) {
        return WHITE;
    }

    return {static_cast<unsigned char>(r), static_cast<unsigned char>(g), static_cast<unsigned char>(b), 255};
}

static void ApplyDummyShape(CharacterDefinition &definition, int size, Color color) {
    definition.texture.clear();
    definition.color = color;

    definition.frameWidth = size;
    definition.frameHeight = size;
    definition.frameCount = 1;

    definition.shadowWidth = 0.0f;
    definition.shadowHeight = 0.0f;
    definition.shadowOffsetY = 0.0f;

    definition.hitboxWidth = static_cast<float>(size);
    definition.hitboxHeight = static_cast<float>(size);
    definition.hitboxOffsetY = 0.0f;
}

bool CharacterDefinition::IsDummy() const {
    return texture.empty();
}

CharacterDefinition CharacterDefinition::Dummy(const std::string &name, int size, Color color, int maxHealth) {
    CharacterDefinition definition;

    definition.name = name;
    definition.maxHealth = maxHealth;

    ApplyDummyShape(definition, size, color);

    return definition;
}

bool CharacterDefinition::SaveCopy(const std::string &source, const std::string &target, const std::string &name) {
    std::ifstream file("assets/" + source);

    if (!file) {
        return false;
    }

    try {
        nlohmann::ordered_json j = nlohmann::ordered_json::parse(file);

        j["name"] = name;

        return SaveAsset(target, j.dump(2) + "\n");
    } catch (const nlohmann::json::exception &) {
        return false;
    }
}

bool CharacterDefinition::SaveDummy(const std::string &filename) const {
    nlohmann::ordered_json j;

    j["name"] = name;
    j["size"] = frameWidth;
    j["color"] = ColorToHex(color);
    j["maxHealth"] = maxHealth;

    return SaveAsset(filename, j.dump(2) + "\n");
}


static CharacterDefinition LoadUnchecked(const std::string &filename) {
    std::ifstream file("assets/" + filename);

    json j;
    file >> j;

    CharacterDefinition definition;

    definition.file = filename;

    definition.name = j["name"];

    // Dummy objects have a colored square instead of a texture
    if (j.contains("color")) {
        ApplyDummyShape(definition, j.value("size", 16), ColorFromHex(j.value("color", std::string())));

        definition.maxHealth = j.value("maxHealth", definition.maxHealth);
        definition.isSolid = j.value("isSolid", false);

        return definition;
    }

    definition.texture = j["texture"];

    definition.frameWidth = j["frameWidth"];
    definition.frameHeight = j["frameHeight"];
    definition.frameCount = j["frameCount"];

    definition.animationSpeed = std::chrono::milliseconds(j["animationSpeedMs"]);

    definition.shadowWidth = j["shadowWidth"];
    definition.shadowHeight = j["shadowHeight"];
    definition.shadowOffsetY = j["shadowOffsetY"];

    // Optional: falls back to the default in the struct
    definition.moveSpeed = j.value("moveSpeed", definition.moveSpeed);

    definition.maxHealth = j.value("maxHealth", definition.maxHealth);

    definition.isSolid = j.value("isSolid", false);

    definition.hitboxWidth = j.value("hitboxWidth",
                                     static_cast<float>(definition.frameWidth));
    definition.hitboxHeight = j.value("hitboxHeight",
                                      static_cast<float>(definition.frameHeight));
    definition.hitboxOffsetY = j.value("hitboxOffsetY", 0.0f);

    return definition;
}

// Garish, so a missing definition stands out right away
static CharacterDefinition Missing(const std::string &filename) {
    CharacterDefinition definition = CharacterDefinition::Dummy("Missing", 16, {255, 0, 220, 255}, 0);

    definition.file = filename;

    return definition;
}

CharacterDefinition CharacterDefinition::Load(const std::string &filename) {
    if (!std::ifstream("assets/" + filename)) {
        TraceLog(LOG_WARNING, "CHARACTER: [%s] Datei nicht gefunden, nutze Platzhalter", filename.c_str());
        return Missing(filename);
    }

    try {
        return LoadUnchecked(filename);
    } catch (const json::exception &error) {
        TraceLog(LOG_WARNING, "CHARACTER: [%s] %s, nutze Platzhalter", filename.c_str(), error.what());
        return Missing(filename);
    }
}
