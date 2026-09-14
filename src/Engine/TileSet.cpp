#include "TileSet.h"

#include <algorithm>
#include <fstream>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Placeholder while no atlas is loaded
constexpr Color PLACEHOLDER_YELLOW{255, 229, 26, 255};
constexpr Color PLACEHOLDER_BLUE{41, 110, 219, 255};

// For digits the TileSet does not know. Deliberately garish, so a typo in the
// map stands out right away.
constexpr Color MISSING_PINK{255, 0, 220, 255};
constexpr Color MISSING_BLACK{0, 0, 0, 255};

TileSet::TileSet(int tileSize)
    : tileSize(std::max(tileSize, 1)),
      tiles(MAX_TILE + 1) {
}

TileSet TileSet::Load(Assets &assets, const std::string &filename) {
    std::ifstream file("assets/" + filename);

    if (!file) {
        TraceLog(LOG_WARNING, "TILESET: [%s] Datei nicht gefunden, nutze Platzhalter", filename.c_str());
        return TileSet();
    }

    try {
        json j = json::parse(file);

        TileSet set(j.value("tileSize", 32));
        set.described = true;

        for (const auto &item: j.at("tiles").items()) {
            const std::string &key = item.key();

            // The keys are the digits from the map
            if (key.size() != 1 || key[0] < '1' || key[0] > '9') {
                TraceLog(LOG_WARNING, "TILESET: [%s] \"%s\" ist keine Ziffer von 1 bis 9", filename.c_str(), key.c_str());
                continue;
            }

            TileInfo &info = set.tiles[static_cast<std::size_t>(key[0] - '0')];
            info.defined = true;
            info.walkable = item.value().value("walkable", true);
            info.name = item.value().value("name", std::string());
        }

        Texture2D &texture = assets.Texture().Get(j.at("texture").get<std::string>());

        if (texture.id != 0) {
            set.texture = &texture;
            set.columns = texture.width / set.tileSize;
            set.rows = texture.height / set.tileSize;
        } else {
            TraceLog(LOG_WARNING, "TILESET: [%s] Atlas nicht geladen, zeichne Platzhalter", filename.c_str());
        }

        return set;
    } catch (const json::exception &error) {
        TraceLog(LOG_WARNING, "TILESET: [%s] %s, nutze Platzhalter", filename.c_str(), error.what());
        return TileSet();
    }
}

int TileSet::TileSize() const {
    return tileSize;
}

bool TileSet::IsDefined(int tile) const {
    if (tile < 1 || tile > MAX_TILE) {
        return false;
    }

    return tiles[static_cast<std::size_t>(tile)].defined;
}

bool TileSet::IsWalkable(int tile) const {
    if (tile < 1 || tile > MAX_TILE) {
        return false;
    }

    if (!described) {
        return true;
    }

    const TileInfo &info = tiles[static_cast<std::size_t>(tile)];

    return info.defined && info.walkable;
}

const std::string &TileSet::Name(int tile) const {
    static const std::string none;

    if (tile < 1 || tile > MAX_TILE) {
        return none;
    }

    return tiles[static_cast<std::size_t>(tile)].name;
}

std::vector<int> TileSet::AvailableTiles() const {
    std::vector<int> available = {0};

    for (int tile = 1; tile <= MAX_TILE; tile++) {
        if (!described || IsDefined(tile)) {
            available.push_back(tile);
        }
    }

    return available;
}

void TileSet::Draw(int tile, int x, int y) const {
    if (described && !IsDefined(tile)) {
        DrawMissing(x, y);
        return;
    }

    if (texture == nullptr) {
        DrawPlaceholder(x, y);
        return;
    }

    int index = tile - 1;

    // The description knows the digit, but the atlas has too few images
    if (index >= columns * rows) {
        DrawMissing(x, y);
        return;
    }

    Rectangle source = {
        static_cast<float>(index % columns * tileSize),
        static_cast<float>(index / columns * tileSize),
        static_cast<float>(tileSize),
        static_cast<float>(tileSize)
    };

    DrawTextureRec(*texture, source, {static_cast<float>(x), static_cast<float>(y)}, WHITE);
}

void TileSet::DrawPlaceholder(int x, int y) const {
    bool even = (x / tileSize + y / tileSize) % 2 == 0;

    DrawRectangle(x, y, tileSize, tileSize, even ? PLACEHOLDER_YELLOW : PLACEHOLDER_BLUE);
}

void TileSet::DrawMissing(int x, int y) const {
    int half = tileSize / 2;
    int rest = tileSize - half;

    DrawRectangle(x, y, half, half, MISSING_PINK);
    DrawRectangle(x + half, y, rest, half, MISSING_BLACK);
    DrawRectangle(x, y + half, half, rest, MISSING_BLACK);
    DrawRectangle(x + half, y + half, rest, rest, MISSING_PINK);
}
