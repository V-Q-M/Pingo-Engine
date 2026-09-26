#include "ObjectEffects.h"

#include "Engine/Names.h"

const std::vector<ObjectEffect> &ObjectEffects::All() {
    static const std::vector<ObjectEffect> effects{
        {"burning", "Burning"},
        {"poisoned", "Poisoned"},
        {"stunned", "Stunned"},
        {"angry", "Angry"}
    };

    return effects;
}

const ObjectEffect *ObjectEffects::Match(const std::string &word) {
    const ObjectEffect *found = nullptr;
    const std::string wanted = ToLower(word);

    if (wanted.empty()) {
        return nullptr;
    }

    for (const ObjectEffect &effect: All()) {
        if (effect.id.rfind(wanted, 0) != 0) {
            continue;
        }

        // Two effects with the same start: the word is not clear enough
        if (found != nullptr) {
            return nullptr;
        }

        found = &effect;
    }

    return found;
}

std::string ObjectEffects::Names() {
    std::string names;

    for (const ObjectEffect &effect: All()) {
        names += names.empty() ? effect.id : " " + effect.id;
    }

    return names;
}

const ObjectEffect *ObjectEffects::Find(const std::string &id) {
    for (const ObjectEffect &effect: All()) {
        if (effect.id == id) {
            return &effect;
        }
    }

    return nullptr;
}
