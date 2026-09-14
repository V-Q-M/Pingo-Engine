#include "Team.h"

Character &Team::Add(Assets &assets, const std::string &characterFile, Vector2 position) {
    CharacterDefinition definition = CharacterDefinition::Load(characterFile);

    return monsters.emplace_back(assets, definition, position);
}

Character &Team::Add(Character character) {
    return monsters.emplace_back(std::move(character));
}

Character *Team::Find(const SpriteInstance *instance) {
    for (Character &monster: monsters) {
        if (&monster.GetCharacter() == instance) {
            return &monster;
        }
    }

    return nullptr;
}

bool Team::Remove(const SpriteInstance *instance) {
    if (instance == nullptr || Find(instance) == nullptr) {
        return false;
    }

    // vector::erase needs assignable elements. Character is not, because of the
    // texture reference in the sprite, so the list is rebuilt.
    std::vector<Character> rest;
    rest.reserve(monsters.size() - 1);

    for (Character &monster: monsters) {
        if (&monster.GetCharacter() != instance) {
            rest.push_back(std::move(monster));
        }
    }

    monsters.swap(rest);

    return true;
}

void Team::Clear() {
    monsters.clear();
}

std::vector<Character> &Team::Monsters() {
    return monsters;
}

const std::vector<Character> &Team::Monsters() const {
    return monsters;
}
