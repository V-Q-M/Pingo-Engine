#pragma once

#include <string>
#include <unordered_map>

#include "raylib.h"

class TextureManager {
public:
    TextureManager();

    ~TextureManager();

    TextureManager(const TextureManager &) = delete;

    TextureManager &operator=(const TextureManager &) = delete;

    Texture2D &Get(const std::string &filename);

    // Solid colored rectangle, created once and then reused
    Texture2D &GetSolid(Color color, int width, int height);

private:
    Texture2D &Load(const std::string &filename);

    std::unordered_map<std::string, Texture2D> textures;
};

