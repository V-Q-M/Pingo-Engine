#include "Config.h"

Config Config::Default() {
#if defined(PINGO_MODE_DEVELOPMENT)
    return Config(BuildMode::Development);
#elif defined(PINGO_MODE_DEBUGGING)
    return Config(BuildMode::Debugging);
#else
    return Config(BuildMode::Release);
#endif
}

// This is what defines the modes. Whoever adds a switch only has to
// extend this list.
Config::Config(BuildMode mode)
    : mode(mode),
      // Music only gets in the way while building the world
      musicEnabled(mode != BuildMode::Development),
      debugTools(mode != BuildMode::Release) {
}

BuildMode Config::Mode() const {
    return mode;
}

const char *Config::ModeName() const {
    switch (mode) {
        case BuildMode::Release:
            return "Release";
        case BuildMode::Debugging:
            return "Debugging";
        case BuildMode::Development:
            return "Development";
    }

    return "Release";
}

bool Config::IsRelease() const {
    return mode == BuildMode::Release;
}

bool Config::IsDevelopment() const {
    return mode == BuildMode::Development;
}

bool Config::HasDebugTools() const {
    return debugTools;
}

bool Config::MusicEnabled() const {
    return musicEnabled;
}

void Config::SetMusicEnabled(bool enabled) {
    musicEnabled = enabled;
}
