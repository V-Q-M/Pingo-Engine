#pragma once

#include <map>
#include <string>
#include <vector>

#include "Engine/Assets.h"
#include "Engine/SpriteInstance.h"
#include "CharacterDefinition.h"


class Character {
public:
    Character(Assets &assets,
              const CharacterDefinition &definition,
              Vector2 position);

    void Move(Vector2 direction,
              float dt,
              const std::vector<SpriteInstance *> &obstacles);

    const std::string &Name() const;

    // The character file the character was created from
    const std::string &DefinitionFile() const;

    // What this character changes about its type, e.g. its own name. The
    // definition it was created from already contains these changes.
    const CharacterOverrides &Overrides() const;

    void SetOverrides(CharacterOverrides overrides);

    // Id of the ObjectGroup the character belongs to, e.g. "ally"
    const std::string &Group() const;

    void SetGroup(std::string group);

    // Pixels per second, from the JSON
    float MoveSpeed() const;

    int Health() const;

    int MaxHealth() const;

    void SetHealth(int value);

    // Share of the remaining health, 0.0 to 1.0
    float HealthFraction() const;

    // Characters without health (maxHealth 0) take no damage and have no health
    // bar
    bool IsIndestructible() const;

    int Energy() const;

    int MaxEnergy() const;

    void SetEnergy(int value);

    // Share of the remaining energy, 0.0 to 1.0
    float EnergyFraction() const;

    // Only characters with energy (maxEnergy above 0) have an energy bar
    bool HasEnergy() const;

    // The effect the character is in, empty for none. While it lasts, the
    // character animates the row of its sheet that belongs to the effect.
    const std::string &Effect() const;

    void SetEffect(const std::string &effect);

    // Does the character show this effect, does it have a row for it?
    bool ShowsEffect(const std::string &effect) const;

    SpriteInstance &GetCharacter();

    const SpriteInstance &GetCharacter() const;

private:
    std::string name;
    std::string definitionFile;

    CharacterOverrides overrides;

    std::string group;

    float moveSpeed;

    int health;
    int maxHealth;

    int energy;
    int maxEnergy;

    // Rows of the sheet for the effects, see CharacterLook::effectRows
    std::map<std::string, int> effectRows;

    std::string effect;

    SpriteInstance character;
};


