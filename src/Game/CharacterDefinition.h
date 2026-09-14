#pragma  once

#include <chrono>
#include <string>

#include "raylib.h"

struct CharacterDefinition {
    std::string name;

    // File the definition was loaded from
    std::string file;

    std::string texture;

    int frameWidth;
    int frameHeight;
    int frameCount;

    std::chrono::milliseconds animationSpeed{150};

    float shadowWidth = 10.0f;
    float shadowHeight = 4.0f;
    float shadowOffsetY = -1.0f;

    float moveSpeed = 60.0f;

    int maxHealth = 100;

    // Solid characters cannot walk into each other
    bool isSolid = false;

    // Without a value in the JSON the hitbox covers the whole sprite frame
    float hitboxWidth = 0.0f;
    float hitboxHeight = 0.0f;
    float hitboxOffsetY = 0.0f;

    // Dummy objects have no texture but a square in this color
    Color color{255, 255, 255, 255};

    bool IsDummy() const;

    // If the file is missing or broken, there is a garish placeholder "Missing"
    // instead of a crash
    static CharacterDefinition Load(const std::string &filename);

    // A dummy object: square with side length size, the hitbox just as large,
    // without a shadow. maxHealth 0 means indestructible and without a health bar.
    static CharacterDefinition Dummy(const std::string &name, int size, Color color, int maxHealth);

    // Saves a dummy object, path relative to the asset folder
    bool SaveDummy(const std::string &filename) const;

    // Copies a definition file under a new name. Everything else stays, including
    // texture and hitbox.
    static bool SaveCopy(const std::string &source, const std::string &target, const std::string &name);
};


