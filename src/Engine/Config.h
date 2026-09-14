#pragma once

#include "BuildMode.h"

// Switches engine features depending on the build mode:
//   Release      the finished game, without tools
//   Debugging    playable, plus debug tools like the FPS display and hitboxes
//   Development  not playable: the world stands still, the camera flies freely
//                and the scenes offer tools like the tile editor
class Config {
public:
    // The mode CMake set via PINGO_BUILD_MODE
    static Config Default();

    explicit Config(BuildMode mode);

    BuildMode Mode() const;

    // "Release", "Debugging" or "Development"
    const char *ModeName() const;

    bool IsRelease() const;

    bool IsDevelopment() const;

    // Debug tools like the FPS display, hitboxes and scene keys
    bool HasDebugTools() const;

    bool MusicEnabled() const;

    void SetMusicEnabled(bool enabled);

private:
    BuildMode mode;

    bool musicEnabled;
    bool debugTools;
};
