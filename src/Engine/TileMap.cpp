#include "TileMap.h"

#include <algorithm>
#include <fstream>
#include <utility>

#include "AssetFile.h"

TileMap TileMap::Load(const std::string &filename, TileSet tileSet) {
    TileMap map;
    map.tileSet = std::move(tileSet);
    map.filename = filename;

    std::ifstream file("assets/" + filename);

    if (!file) {
        TraceLog(LOG_WARNING, "TILEMAP: [%s] Datei nicht gefunden, Karte bleibt leer", filename.c_str());
        return map;
    }

    std::vector<std::string> lines;
    std::string line;

    while (std::getline(file, line)) {
        // Files saved on Windows additionally end every line with \r
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.empty() || line.front() == '#') {
            continue;
        }

        lines.push_back(line);

        map.columns = std::max(map.columns, static_cast<int>(line.size()));
    }

    map.rows = static_cast<int>(lines.size());
    map.tiles.assign(static_cast<std::size_t>(map.columns * map.rows), EMPTY);

    for (int row = 0; row < map.rows; row++) {
        const std::string &text = lines[static_cast<std::size_t>(row)];

        for (int column = 0; column < static_cast<int>(text.size()); column++) {
            char character = text[static_cast<std::size_t>(column)];

            // Lines that are too short and everything except digits stay empty
            if (character >= '0' && character <= '9') {
                map.tiles[map.Index(column, row)] = character - '0';
            }
        }
    }

    return map;
}

TileMap TileMap::Empty(const std::string &filename, int columns, int rows, TileSet tileSet) {
    TileMap map;
    map.tileSet = std::move(tileSet);
    map.filename = filename;

    map.Resize(columns, rows);

    return map;
}

void TileMap::Resize(int columns, int rows) {
    columns = std::max(columns, 0);
    rows = std::max(rows, 0);

    std::vector<int> resized(static_cast<std::size_t>(columns * rows), EMPTY);

    // Whatever fits into both sizes stays in its place
    for (int row = 0; row < std::min(rows, this->rows); row++) {
        for (int column = 0; column < std::min(columns, this->columns); column++) {
            resized[static_cast<std::size_t>(row * columns + column)] = TileAt(column, row);
        }
    }

    this->columns = columns;
    this->rows = rows;
    tiles = std::move(resized);
}

int TileMap::Columns() const {
    return columns;
}

int TileMap::Rows() const {
    return rows;
}

const TileSet &TileMap::GetTileSet() const {
    return tileSet;
}

int TileMap::TileSize() const {
    return tileSet.TileSize();
}

Rectangle TileMap::Area() const {
    return {
        0.0f,
        0.0f,
        static_cast<float>(columns * TileSize()),
        static_cast<float>(rows * TileSize())
    };
}

std::size_t TileMap::Index(int column, int row) const {
    return static_cast<std::size_t>(row * columns + column);
}

const std::string &TileMap::Filename() const {
    return filename;
}

bool TileMap::SetTile(int column, int row, int tile) {
    if (column < 0 || row < 0 || column >= columns || row >= rows) {
        return false;
    }

    if (tile < EMPTY || tile > TileSet::MAX_TILE) {
        return false;
    }

    tiles[Index(column, row)] = tile;

    return true;
}

bool TileMap::CellAt(Vector2 position, int &column, int &row) const {
    if (position.x < 0.0f || position.y < 0.0f) {
        return false;
    }

    column = static_cast<int>(position.x) / TileSize();
    row = static_cast<int>(position.y) / TileSize();

    return column < columns && row < rows;
}

std::string TileMap::RowText(int row) const {
    std::string text;

    for (int column = 0; column < columns; column++) {
        text += static_cast<char>('0' + TileAt(column, row));
    }

    return text;
}

bool TileMap::Save(const std::string &path) const {
    std::vector<std::string> output;

    int row = 0;

    std::ifstream existing(path);
    std::string line;

    while (std::getline(existing, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        // Comments and empty lines stay as they are
        if (line.empty() || line.front() == '#') {
            output.push_back(line);
            continue;
        }

        // Every map line of the file is replaced by the current row
        if (row < rows) {
            output.push_back(RowText(row));
            row++;
        }
    }

    existing.close();

    // New file, or the map has more rows than the file
    for (; row < rows; row++) {
        output.push_back(RowText(row));
    }

    std::string text;

    for (const std::string &line: output) {
        text += line;
        text += '\n';
    }

    // If writing fails, the old map stays intact
    return WriteFileAtomically(path, text);
}

bool TileMap::SaveAsset() const {
    bool saved = Save("assets/" + filename);

#ifdef PINGO_ASSET_SOURCE_DIR
    saved = Save(std::string(PINGO_ASSET_SOURCE_DIR) + "/" + filename) && saved;
#endif

    return saved;
}

int TileMap::TileAt(int column, int row) const {
    if (column < 0 || row < 0 || column >= columns || row >= rows) {
        return EMPTY;
    }

    return tiles[Index(column, row)];
}

bool TileMap::IsWalkable(int column, int row) const {
    return tileSet.IsWalkable(TileAt(column, row));
}

Vector2 TileMap::CellOrigin(int column, int row) const {
    return {
        static_cast<float>(column * TileSize()),
        static_cast<float>(row * TileSize())
    };
}

// Horizontally centered and just above the bottom edge: the shadow still
// lies on the cell, the body sticks out upwards as usual in top-down games.
Vector2 TileMap::CellFeet(int column, int row) const {
    Vector2 origin = CellOrigin(column, row);
    float size = static_cast<float>(TileSize());

    return {
        origin.x + size / 2.0f,
        origin.y + size - size / 8.0f
    };
}

void TileMap::Draw() const {
    int size = TileSize();

    for (int row = 0; row < rows; row++) {
        for (int column = 0; column < columns; column++) {
            int tile = TileAt(column, row);

            if (tile == EMPTY) {
                continue;
            }

            tileSet.Draw(tile, column * size, row * size);
        }
    }
}
