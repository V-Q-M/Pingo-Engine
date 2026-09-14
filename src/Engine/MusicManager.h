#pragma once

#include <string>
#include "raylib.h"

class MusicManager {
public:
    MusicManager();

    ~MusicManager();

    void Play(const std::string &filename);

    void Stop();

    void Update();

    // The track that is currently playing, empty when silent
    const std::string &CurrentTrack() const;

    // 0.0 to 1.0. Kept for tracks played later.
    void SetVolume(float volume);

private:
    Music music{};
    bool loaded = false;

    float volume = 1.0f;

    std::string currentTrack;
};
