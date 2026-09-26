#include "Soundcard.h"

#include "AssetFile.h"

const std::vector<Soundcard> &Soundcard::All() {
    static const std::vector<Soundcard> cards = {
        {"none", "None", ""},
        {"mt32", "Roland MT-32", "mt32"}
    };

    return cards;
}

const Soundcard &Soundcard::Find(const std::string &id) {
    for (const Soundcard &card: All()) {
        if (card.id == id) {
            return card;
        }
    }

    return All().front();
}

const Soundcard &Soundcard::Step(const std::string &id, int direction) {
    const std::vector<Soundcard> &cards = All();
    const int count = static_cast<int>(cards.size());

    int index = 0;

    for (int i = 0; i < count; i++) {
        if (cards[static_cast<std::size_t>(i)].id == id) {
            index = i;
        }
    }

    index = ((index + direction) % count + count) % count;

    return cards[static_cast<std::size_t>(index)];
}

std::string Soundcard::Track(const std::string &music) const {
    if (folder.empty() || music.empty()) {
        return music;
    }

    std::size_t slash = music.rfind('/');
    std::string directory = slash == std::string::npos ? "" : music.substr(0, slash + 1);
    std::string name = slash == std::string::npos ? music : music.substr(slash + 1);

    std::string version = directory + folder + "/" + name;

    return AssetExists(version) ? version : music;
}
