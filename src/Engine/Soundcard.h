#pragma once

#include <string>
#include <vector>

// A version of the soundtrack, like the sound card choice of old games: the
// same music, arranged for different hardware. Every version lives in its own
// subfolder next to the normal tracks, e.g. music/mt32/battle.wav.
struct Soundcard {
    // Stored in the settings, e.g. "mt32"
    std::string id;

    // Shown in the settings menu
    std::string label;

    // Subfolder of the tracks, empty for the normal version
    std::string folder;

    // All cards the settings offer, the first one is the default
    static const std::vector<Soundcard> &All();

    // The card with this id, the default one if there is none
    static const Soundcard &Find(const std::string &id);

    // The card after (direction 1) or before (-1) this one, wrapping around
    static const Soundcard &Step(const std::string &id, int direction);

    // This card's version of a track, e.g. music/mt32/battle.wav for
    // music/battle.wav. Without a version of its own the normal track plays.
    std::string Track(const std::string &music) const;
};
