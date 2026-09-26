#include "CharacterDefinition.h"

#include <algorithm>
#include <cmath>
#include <fstream>

#include <nlohmann/json.hpp>

#include "ObjectEffects.h"
#include "Engine/AssetFile.h"
#include "Engine/ColorHex.h"

using json = nlohmann::json;

// Shares of the frame size an estimated hitbox and shadow cover. Close to the
// hand-tuned values of the existing characters.
constexpr float ESTIMATED_HITBOX_WIDTH = 0.6f;
constexpr float ESTIMATED_HITBOX_HEIGHT = 0.65f;
constexpr float ESTIMATED_SHADOW_WIDTH = 0.28f;
constexpr float ESTIMATED_SHADOW_HEIGHT = 0.3f;
constexpr float ESTIMATED_SHADOW_MIN_HEIGHT = 3.0f;

static void ApplyDummyShape(CharacterDefinition &definition, int size, Color color) {
    definition.texture.clear();
    definition.color = color;

    definition.frameWidth = size;
    definition.frameHeight = size;
    definition.frameCount = 1;

    definition.shadowWidth = 0.0f;
    definition.shadowHeight = 0.0f;
    definition.shadowOffsetY = 0.0f;
    definition.shadowOffsetX = 0.0f;

    definition.hitboxWidth = static_cast<float>(size);
    definition.hitboxHeight = static_cast<float>(size);
    definition.hitboxOffsetY = 0.0f;
    definition.hitboxOffsetX = 0.0f;
}

// Hitbox and shadow for a sprite sheet nobody has tuned by hand yet
static void EstimateBody(CharacterDefinition &definition) {
    float smaller = static_cast<float>(std::min(definition.frameWidth, definition.frameHeight));

    definition.hitboxWidth = std::round(smaller * ESTIMATED_HITBOX_WIDTH);
    definition.hitboxHeight = std::round(static_cast<float>(definition.frameHeight) * ESTIMATED_HITBOX_HEIGHT);
    definition.hitboxOffsetY = 0.0f;
    definition.hitboxOffsetX = 0.0f;

    definition.shadowOffsetX = 0.0f;
    definition.shadowWidth = std::round(smaller * ESTIMATED_SHADOW_WIDTH);
    definition.shadowHeight = std::max(ESTIMATED_SHADOW_MIN_HEIGHT,
                                       std::round(definition.shadowWidth * ESTIMATED_SHADOW_HEIGHT));
    definition.shadowOffsetY = 0.0f;
}

bool CharacterLook::IsSquare() const {
    return texture.empty();
}

bool CharacterLook::operator==(const CharacterLook &other) const {
    if (IsSquare() != other.IsSquare()) {
        return false;
    }

    if (IsSquare()) {
        return frameWidth == other.frameWidth &&
               color.r == other.color.r &&
               color.g == other.color.g &&
               color.b == other.color.b;
    }

    return texture == other.texture &&
           frameWidth == other.frameWidth &&
           frameHeight == other.frameHeight &&
           frameCount == other.frameCount &&
           effectRows == other.effectRows;
}

bool CharacterOverrides::operator==(const CharacterOverrides &other) const {
    return name == other.name &&
           look == other.look &&
           body == other.body &&
           maxHealth == other.maxHealth &&
           maxEnergy == other.maxEnergy;
}

bool CharacterBox::operator==(const CharacterBox &other) const {
    return width == other.width &&
           height == other.height &&
           offsetX == other.offsetX &&
           offsetY == other.offsetY;
}

bool CharacterBody::operator==(const CharacterBody &other) const {
    return shadow == other.shadow && hitbox == other.hitbox && shadowBounce == other.shadowBounce;
}

static int ToPixels(float value) {
    return static_cast<int>(std::lround(value));
}

CharacterBody CharacterDefinition::Body() const {
    CharacterBody body;

    body.shadow = {ToPixels(shadowWidth), ToPixels(shadowHeight), ToPixels(shadowOffsetX), ToPixels(shadowOffsetY)};
    body.hitbox = {ToPixels(hitboxWidth), ToPixels(hitboxHeight), ToPixels(hitboxOffsetX), ToPixels(hitboxOffsetY)};
    body.shadowBounce = ToPixels(shadowBounce);

    return body;
}

void CharacterDefinition::SetBody(const CharacterBody &body) {
    shadowWidth = static_cast<float>(body.shadow.width);
    shadowHeight = static_cast<float>(body.shadow.height);
    shadowOffsetX = static_cast<float>(body.shadow.offsetX);
    shadowOffsetY = static_cast<float>(body.shadow.offsetY);

    hitboxWidth = static_cast<float>(body.hitbox.width);
    hitboxHeight = static_cast<float>(body.hitbox.height);
    hitboxOffsetX = static_cast<float>(body.hitbox.offsetX);
    hitboxOffsetY = static_cast<float>(body.hitbox.offsetY);

    shadowBounce = static_cast<float>(body.shadowBounce);
}

bool CharacterDefinition::IsDummy() const {
    return texture.empty();
}

CharacterLook CharacterDefinition::Look() const {
    CharacterLook look;

    look.texture = texture;
    look.frameWidth = frameWidth;
    look.frameHeight = frameHeight;
    look.frameCount = frameCount;
    look.color = color;
    look.effectRows = effectRows;

    return look;
}

void CharacterDefinition::SetLook(const CharacterLook &look) {
    if (look.IsSquare()) {
        ApplyDummyShape(*this, look.frameWidth, look.color);

        // A square has no sheet and therefore no rows for effects
        effectRows.clear();

        return;
    }

    // The rows of the effects belong to the sheet, they never change the body
    effectRows = look.effectRows;

    bool sameFrames = texture == look.texture &&
                      frameWidth == look.frameWidth &&
                      frameHeight == look.frameHeight &&
                      frameCount == look.frameCount;

    // Same sheet, same frames: hitbox and shadow of the file fit already
    if (sameFrames) {
        return;
    }

    texture = look.texture;
    frameWidth = look.frameWidth;
    frameHeight = look.frameHeight;
    frameCount = look.frameCount;

    EstimateBody(*this);
}

CharacterDefinition CharacterDefinition::WithOverrides(const CharacterOverrides &overrides) const {
    CharacterDefinition result = *this;

    if (!overrides.name.empty()) {
        result.name = overrides.name;
    }

    if (overrides.look) {
        result.SetLook(*overrides.look);
    }

    if (overrides.body) {
        result.SetBody(*overrides.body);
    }

    if (overrides.maxHealth) {
        result.maxHealth = *overrides.maxHealth;
    }

    if (overrides.maxEnergy) {
        result.maxEnergy = *overrides.maxEnergy;
    }

    return result;
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

bool CharacterDefinition::Save(const std::string &filename) const {
    nlohmann::ordered_json j;

    j["name"] = name;

    if (IsDummy()) {
        j["size"] = frameWidth;
        j["color"] = ColorToHex(color);
        j["maxHealth"] = maxHealth;
        j["maxEnergy"] = maxEnergy;
        j["isSolid"] = isSolid;

        // Shadow and hitbox only if they differ from what a square has anyway
        CharacterDefinition square = Dummy(name, frameWidth, color, maxHealth);

        if (!(Body() == square.Body())) {
            j["hitboxWidth"] = hitboxWidth;
            j["hitboxHeight"] = hitboxHeight;
            j["hitboxOffsetY"] = hitboxOffsetY;
            j["hitboxOffsetX"] = hitboxOffsetX;

            j["shadowWidth"] = shadowWidth;
            j["shadowHeight"] = shadowHeight;
            j["shadowOffsetY"] = shadowOffsetY;
            j["shadowOffsetX"] = shadowOffsetX;
            j["shadowBounce"] = shadowBounce;
        }

        return SaveAsset(filename, j.dump(2) + "\n");
    }

    // The same keys a hand-written character file has
    j["texture"] = texture;

    j["frameWidth"] = frameWidth;
    j["frameHeight"] = frameHeight;
    j["frameCount"] = frameCount;

    j["animationSpeedMs"] = static_cast<int>(animationSpeed.count());

    j["moveSpeed"] = moveSpeed;

    j["maxHealth"] = maxHealth;
    j["maxEnergy"] = maxEnergy;
    j["isSolid"] = isSolid;

    // Only effects this character really shows land in the file
    if (!effectRows.empty()) {
        nlohmann::ordered_json effects;

        for (const ObjectEffect &effect: ObjectEffects::All()) {
            auto row = effectRows.find(effect.id);

            if (row != effectRows.end()) {
                effects[effect.id] = row->second;
            }
        }

        j["effects"] = effects;
    }

    j["hitboxWidth"] = hitboxWidth;
    j["hitboxHeight"] = hitboxHeight;
    j["hitboxOffsetY"] = hitboxOffsetY;
    j["hitboxOffsetX"] = hitboxOffsetX;

    j["shadowBounce"] = shadowBounce;

    j["shadowWidth"] = shadowWidth;
    j["shadowHeight"] = shadowHeight;
    j["shadowOffsetY"] = shadowOffsetY;
    j["shadowOffsetX"] = shadowOffsetX;

    return SaveAsset(filename, j.dump(2) + "\n");
}

std::vector<std::string> CharacterDefinition::AvailableTypes() {
    return AssetFilesIn(TYPE_FOLDER, ".json");
}

std::vector<std::string> CharacterDefinition::AvailableSprites() {
    return AssetFilesIn(SPRITE_FOLDER, ".png");
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
        definition.maxEnergy = j.value("maxEnergy", definition.maxEnergy);
        definition.isSolid = j.value("isSolid", false);

        // Optional: a square saved from a character carries its own shadow and
        // hitbox, a hand written one only has the square
        definition.hitboxWidth = j.value("hitboxWidth", definition.hitboxWidth);
        definition.hitboxHeight = j.value("hitboxHeight", definition.hitboxHeight);
        definition.hitboxOffsetY = j.value("hitboxOffsetY", definition.hitboxOffsetY);
        definition.hitboxOffsetX = j.value("hitboxOffsetX", definition.hitboxOffsetX);

        definition.shadowWidth = j.value("shadowWidth", definition.shadowWidth);
        definition.shadowHeight = j.value("shadowHeight", definition.shadowHeight);
        definition.shadowOffsetY = j.value("shadowOffsetY", definition.shadowOffsetY);
        definition.shadowOffsetX = j.value("shadowOffsetX", definition.shadowOffsetX);
        definition.shadowBounce = j.value("shadowBounce", definition.shadowBounce);

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

    // Optional: older files have no sideways offsets
    definition.shadowOffsetX = j.value("shadowOffsetX", 0.0f);

    // Optional: without it the shadow keeps its size
    definition.shadowBounce = j.value("shadowBounce", 0.0f);

    // Optional: falls back to the default in the struct
    definition.moveSpeed = j.value("moveSpeed", definition.moveSpeed);

    definition.maxHealth = j.value("maxHealth", definition.maxHealth);

    // Optional: characters without energy have no energy bar
    definition.maxEnergy = j.value("maxEnergy", definition.maxEnergy);

    // Optional: the rows the effects animate, see ObjectEffects
    if (j.contains("effects")) {
        for (const ObjectEffect &effect: ObjectEffects::All()) {
            if (j.at("effects").contains(effect.id)) {
                definition.effectRows[effect.id] = j.at("effects").at(effect.id).get<int>();
            }
        }
    }

    definition.isSolid = j.value("isSolid", false);

    definition.hitboxWidth = j.value("hitboxWidth",
                                     static_cast<float>(definition.frameWidth));
    definition.hitboxHeight = j.value("hitboxHeight",
                                      static_cast<float>(definition.frameHeight));
    definition.hitboxOffsetY = j.value("hitboxOffsetY", 0.0f);
    definition.hitboxOffsetX = j.value("hitboxOffsetX", 0.0f);

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
