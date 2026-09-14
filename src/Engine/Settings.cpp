#include "Settings.h"

#include <algorithm>
#include <fstream>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

Settings Settings::Load(const std::string &filename) {
    Settings settings;

    std::ifstream file(filename);

    if (!file) {
        return settings;
    }

    // A file broken by hand editing should not crash the game, but simply fall
    // back to the default values
    try {
        json j = json::parse(file);

        settings.masterVolume = std::clamp(j.value("masterVolume", settings.masterVolume), 0, 100);
        settings.musicVolume = std::clamp(j.value("musicVolume", settings.musicVolume), 0, 100);

        settings.fullscreen = j.value("fullscreen", settings.fullscreen);
        settings.showFps = j.value("showFps", settings.showFps);
        settings.showHitboxes = j.value("showHitboxes", settings.showHitboxes);
    } catch (const json::exception &) {
        return Settings{};
    }

    return settings;
}

bool Settings::Save(const std::string &filename) const {
    json j = {
        {"masterVolume", masterVolume},
        {"musicVolume", musicVolume},
        {"fullscreen", fullscreen},
        {"showFps", showFps},
        {"showHitboxes", showHitboxes}
    };

    std::ofstream file(filename);

    if (!file) {
        return false;
    }

    file << j.dump(2) << '\n';

    return static_cast<bool>(file);
}
