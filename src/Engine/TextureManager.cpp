#include "TextureManager.h"

TextureManager::TextureManager() {
}

TextureManager::~TextureManager() {
    for (auto &[name, texture]: textures) {
        UnloadTexture(texture);
    }
}

Texture2D &TextureManager::Get(const std::string &filename) {
    auto it = textures.find(filename);

    if (it != textures.end()) {
        return it->second;
    }

    return Load(filename);
}

Texture2D &TextureManager::GetSolid(Color color, int width, int height) {
    // The name can never collide with a file name
    std::string key = "solid:" + std::to_string(color.r) + "," + std::to_string(color.g) + "," +
                      std::to_string(color.b) + "," + std::to_string(color.a) + ":" +
                      std::to_string(width) + "x" + std::to_string(height);

    auto it = textures.find(key);

    if (it != textures.end()) {
        return it->second;
    }

    Image image = GenImageColor(width, height, color);
    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);

    SetTextureFilter(texture, TEXTURE_FILTER_POINT);

    auto [iterator, inserted] = textures.emplace(key, texture);
    return iterator->second;
}

Texture2D &TextureManager::Load(const std::string &filename) {
    Texture2D texture = LoadTexture(("assets/" + filename).c_str());
    SetTextureFilter(texture, TEXTURE_FILTER_POINT);

    auto [iterator, inserted] = textures.emplace(filename, texture);
    return iterator->second;
}

