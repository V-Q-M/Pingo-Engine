#pragma once

#include <string>
#include <vector>

#include "Character.h"

// The characters of a scene, in the order they were added. Every character
// knows its group itself.
class CharacterList {
public:
    // Returns the new character. Pointers to other characters of the list may
    // become invalid, because the list can move. group is the id of an
    // ObjectGroup, overrides are the changes of this one character, e.g. its
    // own name.
    Character &Add(Assets &assets,
                   const std::string &characterFile,
                   Vector2 position,
                   const std::string &group,
                   const CharacterOverrides &overrides = {});

    Character &Add(Character character);

    // The character the SpriteInstance belongs to, otherwise nullptr
    Character *Find(const SpriteInstance *instance);

    const Character *Find(const SpriteInstance *instance) const;

    // Position in the list, -1 if the character does not belong to it
    int IndexOf(const SpriteInstance *instance) const;

    // false if the character does not belong to the list. Pointers to
    // characters of the list become invalid.
    bool Remove(const SpriteInstance *instance);

    void Clear();

    std::vector<Character> &Characters();

    const std::vector<Character> &Characters() const;

private:
    std::vector<Character> characters;
};
