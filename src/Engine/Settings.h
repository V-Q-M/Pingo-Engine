#pragma once

#include <string>

// Settings the player changes in the menu. They are saved as JSON and loaded
// again on the next start.
struct Settings {
    // Volumes from 0 to 100
    int masterVolume = 100;
    int musicVolume = 80;

    bool fullscreen = false;
    bool showFps = false;

    // Only takes effect in the Development build
    bool showHitboxes = true;

    // Shows the id of an object next to its name, for the console
    bool showObjectIds = false;

    // Id of the Soundcard whose version of the music plays
    std::string soundcard = "none";

    // If the file is missing or broken, the default values apply
    static Settings Load(const std::string &filename);

    // false if the file could not be written
    bool Save(const std::string &filename) const;
};
