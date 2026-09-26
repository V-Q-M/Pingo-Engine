#pragma  once

#include <chrono>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "raylib.h"

// How a character looks: the frames of a sprite sheet or, without a texture,
// a colored square
struct CharacterLook {
    // Sprite sheet relative to the asset folder, empty for a square
    std::string texture;

    // For a square both are its side length
    int frameWidth = 24;
    int frameHeight = 24;

    int frameCount = 1;

    // Only used by squares
    Color color{255, 255, 255, 255};

    // Which row of the sheet an effect animates, by its id, e.g.
    // {"burning", 2} for the second row. Effects that are not in here are not
    // shown by this character. Rows are counted from 1, like in an image
    // editor. See ObjectEffects.
    std::map<std::string, int> effectRows;

    bool IsSquare() const;

    // Compares only what matters for the kind of look, e.g. no color for sprites
    bool operator==(const CharacterLook &other) const;
};

// Size and offset of a shadow or a hitbox in whole pixels, see
// CharacterDefinition for what the values mean
struct CharacterBox {
    int width = 0;
    int height = 0;
    int offsetX = 0;
    int offsetY = 0;

    bool operator==(const CharacterBox &other) const;
};

// Shadow and hitbox of a character, the advanced settings in the editor
struct CharacterBody {
    CharacterBox shadow;
    CharacterBox hitbox;

    // Pixels the shadow gets narrower while the character is up in its
    // animation, 0 for a shadow that stays as it is
    int shadowBounce = 0;

    bool operator==(const CharacterBody &other) const;
};

// What a single placed character changes about its type. Everything else, like
// its speed, still comes from the type.
struct CharacterOverrides {
    // Empty: the name of the type
    std::string name;

    // Health and energy this character can have at most. Without a value they
    // come from the type, 0 means the character has none of it.
    std::optional<int> maxHealth;
    std::optional<int> maxEnergy;

    // Without a value the character looks like its type
    std::optional<CharacterLook> look;

    // Shadow and hitbox, applied after the look. Without a value they come
    // from the type, or are estimated for a different sprite.
    std::optional<CharacterBody> body;

    bool operator==(const CharacterOverrides &other) const;
};

struct CharacterDefinition {
    // Where the editor finds types and sprite sheets, relative to the asset folder
    static constexpr const char *TYPE_FOLDER = "characters";
    static constexpr const char *SPRITE_FOLDER = "sprites/characters";

    std::string name;

    // File the definition was loaded from
    std::string file;

    std::string texture;

    int frameWidth = 16;
    int frameHeight = 16;
    int frameCount = 1;

    std::chrono::milliseconds animationSpeed{150};

    float shadowWidth = 10.0f;
    float shadowHeight = 4.0f;
    float shadowOffsetY = -1.0f;
    float shadowOffsetX = 0.0f;

    // See CharacterBody::shadowBounce
    float shadowBounce = 0.0f;

    float moveSpeed = 60.0f;

    int maxHealth = 100;

    // 0: the character has no energy and therefore no energy bar
    int maxEnergy = 0;

    // Solid characters cannot walk into each other
    bool isSolid = false;

    // Without a value in the JSON the hitbox covers the whole sprite frame
    float hitboxWidth = 0.0f;
    float hitboxHeight = 0.0f;
    float hitboxOffsetY = 0.0f;
    float hitboxOffsetX = 0.0f;

    // Dummy objects have no texture but a square in this color
    Color color{255, 255, 255, 255};

    // Rows of the sheet for the effects, see CharacterLook::effectRows
    std::map<std::string, int> effectRows;

    bool IsDummy() const;

    // Texture and frames, or size and color of a dummy
    CharacterLook Look() const;

    // Takes over a look. A different sprite sheet gets a hitbox and shadow
    // estimated from its frame size, the same one keeps the values of the file.
    void SetLook(const CharacterLook &look);

    // Shadow and hitbox, rounded to whole pixels
    CharacterBody Body() const;

    void SetBody(const CharacterBody &body);

    // This definition with the changes of a single placed character
    CharacterDefinition WithOverrides(const CharacterOverrides &overrides) const;

    // If the file is missing or broken, there is a garish placeholder "Missing"
    // instead of a crash
    static CharacterDefinition Load(const std::string &filename);

    // A dummy object: square with side length size, the hitbox just as large,
    // without a shadow. maxHealth 0 means indestructible and without a health
    // bar.
    static CharacterDefinition Dummy(const std::string &name, int size, Color color, int maxHealth);

    // Saves the definition as a type, path relative to the asset folder. Dummies
    // only store size and color, sprites their frames, hitbox and shadow.
    bool Save(const std::string &filename) const;

    // Copies a definition file under a new name. Everything else stays, including
    // texture and hitbox.
    static bool SaveCopy(const std::string &source, const std::string &target, const std::string &name);

    // All type files, alphabetical, e.g. "characters/pingo.json"
    static std::vector<std::string> AvailableTypes();

    // All sprite sheets for characters, alphabetical, e.g.
    // "sprites/characters/pingo_atlas.png"
    static std::vector<std::string> AvailableSprites();
};
