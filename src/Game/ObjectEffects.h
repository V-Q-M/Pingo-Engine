#pragma once

#include <string>
#include <vector>

// A state an object can be in besides normal, e.g. burning. A character shows
// an effect by animating a different row of its sprite sheet, see
// CharacterLook::effectRows.
struct ObjectEffect {
    // Id in the files and in the console, e.g. "burning"
    std::string id;

    // Name in the editor, e.g. "Burning"
    std::string label;
};

class ObjectEffects {
public:
    // No effect: the character animates its normal row
    static constexpr const char *NONE = "";

    // The effects in the order they appear in the editor
    static const std::vector<ObjectEffect> &All();

    // nullptr if there is no effect with this id
    static const ObjectEffect *Find(const std::string &id);

    // The effect whose id starts with the word, so "burn" also finds burning.
    // Case does not matter. nullptr if no effect or more than one matches.
    static const ObjectEffect *Match(const std::string &word);

    // All ids in one line, e.g. for the console
    static std::string Names();
};
