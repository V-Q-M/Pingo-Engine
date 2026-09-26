#include "CharacterList.h"

#include <utility>

Character &CharacterList::Add(Assets &assets,
                              const std::string &characterFile,
                              Vector2 position,
                              const std::string &group,
                              const CharacterOverrides &overrides) {
    CharacterDefinition definition = CharacterDefinition::Load(characterFile).WithOverrides(overrides);

    Character &added = characters.emplace_back(assets, definition, position);

    added.SetGroup(group);
    added.SetOverrides(overrides);

    return added;
}

Character &CharacterList::Add(Character character) {
    return characters.emplace_back(std::move(character));
}

Character *CharacterList::Find(const SpriteInstance *instance) {
    int index = IndexOf(instance);

    return index >= 0 ? &characters[static_cast<std::size_t>(index)] : nullptr;
}

const Character *CharacterList::Find(const SpriteInstance *instance) const {
    int index = IndexOf(instance);

    return index >= 0 ? &characters[static_cast<std::size_t>(index)] : nullptr;
}

int CharacterList::IndexOf(const SpriteInstance *instance) const {
    if (instance == nullptr) {
        return -1;
    }

    for (std::size_t i = 0; i < characters.size(); i++) {
        if (&characters[i].GetCharacter() == instance) {
            return static_cast<int>(i);
        }
    }

    return -1;
}

bool CharacterList::Remove(const SpriteInstance *instance) {
    if (IndexOf(instance) < 0) {
        return false;
    }

    // vector::erase needs assignable elements. Character is not, because of the
    // texture reference in the sprite, so the list is rebuilt.
    std::vector<Character> rest;
    rest.reserve(characters.size() - 1);

    for (Character &character: characters) {
        if (&character.GetCharacter() != instance) {
            rest.push_back(std::move(character));
        }
    }

    characters.swap(rest);

    return true;
}

void CharacterList::Clear() {
    characters.clear();
}

std::vector<Character> &CharacterList::Characters() {
    return characters;
}

const std::vector<Character> &CharacterList::Characters() const {
    return characters;
}
