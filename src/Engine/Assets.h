#pragma once

#include "MusicManager.h"
#include "TextureManager.h"
#include "SoundManager.h"

class Assets {
public:
    TextureManager &Texture();

    SoundManager &Sound();

    MusicManager &Music();

private:
    TextureManager textureManager;
    SoundManager soundManager;
    MusicManager musicManager;
};

