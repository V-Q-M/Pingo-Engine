#pragma once

#include <set>
#include <string>
#include <vector>

#include "raylib.h"

#include "CharacterDefinition.h"
#include "ObjectGroups.h"

// Where a character stands in a scene
struct ObjectPlacement {
    // Character file, e.g. "characters/pingo.json"
    std::string character;

    // Id of an ObjectGroup, e.g. "ally"
    std::string group = ObjectGroups::ENEMY;

    Vector2 position{0.0f, 0.0f};

    // Health at the start, -1 for full
    int health = -1;

    // Name and look of this one character, where they differ from the type
    CharacterOverrides overrides;

    // For the undo history
    bool operator==(const ObjectPlacement &other) const;
};

// What a scene defines in its file besides its characters
struct SceneSettings {
    // Music of the scene, empty for silence
    std::string music;
};

// The characters of a scene as JSON under assets/scenes. In the Development
// mode they are moved, added and deleted in the game.
class SceneObjects {
public:
    // false if the file exists but cannot be read. It must not be overwritten
    // then, otherwise the lineup would be gone. If the file is missing, there are
    // simply no characters. Entries with a missing character file are skipped.
    static bool Load(const std::string &filename, std::vector<ObjectPlacement> &placements);

    // The music from the same file. If the file is missing, it is empty.
    static SceneSettings LoadSettings(const std::string &filename);

    // Saves like SaveAsset into both asset folders. Anything else in the file,
    // e.g. the music, is kept.
    static bool Save(const std::string &filename, const std::vector<ObjectPlacement> &placements);

    // The ids of all groups a character belongs to, in every scene file
    static std::set<std::string> UsedGroups();
};
