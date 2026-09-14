#pragma  once

#include <string>
#include <unordered_map>

#include "raylib.h"

class SoundManager {
public:
    ~SoundManager();

    Sound &Get(const std::string &filename);

private:
    Sound &Load(const std::string &filename);

    std::unordered_map<std::string, Sound> sounds;
};


