#include "MusicManager.h"

MusicManager::MusicManager() = default;

MusicManager::~MusicManager() {
    if (loaded) {
        StopMusicStream(music);
        UnloadMusicStream(music);
    }
}

void MusicManager::Play(const std::string &filename) {
    if (loaded && currentTrack == filename) {
        return;
    }
    if (loaded) {
        StopMusicStream(music);
        UnloadMusicStream(music);
    }

    music = LoadMusicStream(("assets/" + filename).c_str());

    music.looping = true;

    PlayMusicStream(music);

    SetMusicVolume(music, volume);

    currentTrack = filename;
    loaded = true;
}

void MusicManager::Stop() {
    if (!loaded) {
        return;
    }

    StopMusicStream(music);
    UnloadMusicStream(music);

    loaded = false;
    currentTrack.clear();
}

void MusicManager::Update() {
    if (loaded) {
        UpdateMusicStream(music);
    }
}

void MusicManager::SetVolume(float volume) {
    this->volume = volume;

    if (loaded) {
        SetMusicVolume(music, volume);
    }
}

const std::string &MusicManager::CurrentTrack() const {
    return currentTrack;
}
