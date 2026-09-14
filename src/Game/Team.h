#pragma once

#include <vector>

#include "Character.h"

class Team {
public:
    // void Add(Monster monster);
    // Returns the new character. Pointers to other characters of the team may
    // become invalid, because the list can move.
    Character &Add(Assets &assets, const std::string &characterFile, Vector2 position);

    Character &Add(Character character);

    // The character the SpriteInstance belongs to, otherwise nullptr
    Character *Find(const SpriteInstance *instance);

    // false if the character does not belong to the team. Pointers to characters
    // of the team become invalid.
    bool Remove(const SpriteInstance *instance);

    void Clear();

    std::vector<Character> &Monsters();

    const std::vector<Character> &Monsters() const;

private:
    std::vector<Character> monsters;
};
