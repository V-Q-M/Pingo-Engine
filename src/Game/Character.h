#pragma once
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

    SpriteInstance &GetCharacter();

    const SpriteInstance &GetCharacter() const;

private:
    std::string name;
    std::string definitionFile;

    float moveSpeed;

    int health;
    int maxHealth;

    SpriteInstance character;
};


