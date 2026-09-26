#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "raylib.h"

#include "TileSet.h"

// Map made of square cells, read from a text file.
//
// Every line of the file is a row, every digit a cell. 0 is empty, the
// digits 1 to 9 refer to the cells in the TileSet. Lines starting with #
// and empty lines are ignored.
//
// A cell can also carry the name of a script, see TileScript. Those are
// comment lines of their own, so the rows stay plain digits:
//
//     # @script 3 5 Door
//
// column and row counted from 0 at the top left. A line that cannot be read,
// or points outside the map, is dropped.
class TileMap {
public:
    static constexpr int EMPTY = 0;

    // Start of a comment line that assigns a script to a cell, see above
    static constexpr const char *SCRIPT_LINE = "# @script";

    // If the file is missing, the map is empty
    static TileMap Load(const std::string &filename, TileSet tileSet);

    // A map made only of empty cells, saved under filename
    static TileMap Empty(const std::string &filename, int columns, int rows, TileSet tileSet = TileSet());

    int Columns() const;

    int Rows() const;

    int TileSize() const;

    const TileSet &GetTileSet() const;

    // The whole map in world coordinates
    Rectangle Area() const;

    // Path relative to the asset folder, as given when loading
    const std::string &Filename() const;

    // Digit of the cell, EMPTY outside the map
    int TileAt(int column, int row) const;

    // Sets a cell to a digit from 0 to 9. false outside the map or for an
    // invalid digit.
    bool SetTile(int column, int row, int tile);

    // Name of the script on the cell, empty for none and outside the map
    const std::string &ScriptAt(int column, int row) const;

    // Puts a script on the cell, an empty name takes it off. false outside
    // the map or for a name with spaces, which the file could not keep.
    bool SetScript(int column, int row, const std::string &script);

    // Gives every cell with the script from the name to. How many cells
    // changed.
    int RenameScript(const std::string &from, const std::string &to);

    // Takes the script off every cell. How many cells changed.
    int RemoveScript(const std::string &script);

    // Which cell is at a world position. false next to the map.
    bool CellAt(Vector2 position, int &column, int &row) const;

    // Writes the map to path. Comments and empty lines of an already existing
    // file are kept, only the rows are replaced.
    bool Save(const std::string &path) const;

    // Saves like SaveAsset into both asset folders, under Filename
    bool SaveAsset() const;

    // New cells are empty. If the map gets smaller, the cells at the right and
    // bottom edge are lost, with their scripts.
    void Resize(int columns, int rows);

    // Empty cells and cells the TileSet marks as blocking are not walkable
    bool IsWalkable(int column, int row) const;

    // Top left corner of the cell in world coordinates
    Vector2 CellOrigin(int column, int row) const;

    // Where the feet of a character standing on this cell are
    Vector2 CellFeet(int column, int row) const;

    void Draw() const;

private:
    std::size_t Index(int column, int row) const;

    std::string RowText(int row) const;

    // The script lines of the file, in the order of the cells
    std::vector<std::string> ScriptLines() const;

    // Reads one script line of the file, false if it does not fit the map
    bool ReadScriptLine(const std::string &line);

    std::string filename;

    TileSet tileSet;

    int columns = 0;
    int rows = 0;

    std::vector<int> tiles;

    // Parallel to tiles, empty for a cell without a script
    std::vector<std::string> scripts;
};
