#include "SoundManager.h"

SoundManager::~SoundManager() {
    for (auto &[_, sound]: sounds) {
        UnloadSound(sound);
    }
}

Sound &SoundManager::Get(const std::string &filename) {
    auto it = sounds.find(filename);

    if (it != sounds.end())
        return it->second;

    return Load(filename);
}

Sound &SoundManager::Load(const std::string &filename) {
    Sound sound = LoadSound(("assets/" + filename).c_str());

    auto result = sounds.emplace(filename, sound);

    return result.first->second;
}
