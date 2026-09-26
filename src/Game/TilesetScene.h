#pragma once

#include <string>
#include <vector>

#include "FieldScene.h"
#include "Engine/ConfirmDialog.h"
#include "Engine/GridMover.h"
#include "Engine/TileMap.h"
#include "Engine/TileMapEditor.h"
#include "Scripting/Api/TileWorld.h"

// A world made of cells, e.g. the hub. The player character walks cell by
// cell across the map, the camera follows it. The map is stored in
// assets/maps/<id>.txt.
//
// Its scripts are tile scripts, see TileScript: in the Development build a
// right click on a cell offers its tiles and the scripts of the scene, and
// assigns them to cells. While the game runs every cell with a script gets its
// own instance of it.
//
// A tile script can ask before it leaves the scene, see MapApi::SwitchScene:
// the world then stands still under a Confirm / Cancel window.
class TilesetScene : public FieldScene, private TileWorld {
public:
    // Keys of the files chosen in the scene bar, see SceneEntry::choices
    static constexpr const char *TILE_ATLAS_CHOICE = "tileAtlas";
    static constexpr const char *TILE_DATA_CHOICE = "tileData";

    // id chooses the map, e.g. "hub" for assets/maps/hub.txt
    explicit TilesetScene(Engine &engine, const std::string &id = "hub");

    // With the travel question open only that one is answered, the world
    // stands still
    void Update(float dt) override;

    void UpdateDevelopment(float dt) override;

    void DrawUI() override;

    bool OnEscape() override;

    void LeaveFromPauseMenu() override;

protected:
    void MovePlayer(Character &player, float dt, const std::vector<SpriteInstance *> &all) override;

    void DrawGround() override;

    void DrawOverlay() override;

    // Tile scripts instead of a Main, see ScriptFiles::Kind
    ScriptFiles::Kind ScriptKind() const override;

    // The scripts menu of the tile editor follows the files
    void ScriptsChanged() override;

    // A script created from the menu of a cell lands on that cell
    void ScriptCreated(const std::string &name) override;

    // The cells follow their scripts
    void ScriptRenamed(const std::string &from, const std::string &to) override;

    void ScriptRemoved(const std::string &name) override;

private:
    // Where the player character starts, further to the top left on smaller maps
    static constexpr int START_COLUMN = 2;
    static constexpr int START_ROW = 1;

    // One instance per cell with a script, while the game runs
    void LoadTileScripts();

    // Does what the scripts menu of the editor asked for
    void ApplyEditorRequest(const TileMapEditor::Request &request);

    // Answers the travel question with the mouse or Enter
    void UpdateTravelDialog();

    // Closes the travel question without leaving, see SwitchToScene
    void CancelTravel();

    // What the tile scripts may do, see TileWorld
    int MapColumns() const override;

    int MapRows() const override;

    int TileNumberAt(int column, int row) const override;

    bool SetTileNumberAt(int column, int row, int number) override;

    std::string TileNameAt(int column, int row) const override;

    bool IsTileWalkable(int column, int row) const override;

    int PlayerColumn() const override;

    int PlayerRow() const override;

    int PreviousPlayerColumn() const override;

    int PreviousPlayerRow() const override;

    bool IsPlayerPushingAgainst(int column, int row) const override;

    bool IsPlayerReady() const override;

    void SetPlayerReady(bool ready) override;

    bool SwitchToScene(const std::string &id, bool confirm) override;

    // Before the GridMover, which holds a reference to the map
    TileMap map;

    int startColumn;
    int startRow;

    GridMover mover;

    // The cell of the player before the step of this frame, see
    // Tile::HasPlayerEntered
    int previousColumn;
    int previousRow;

    // Whether tile events may fire, see MapApi::IsPlayerReady. Not ready,
    // the player gets ready again on arriving anywhere but this cell. It
    // starts out not ready on the start cell.
    bool playerReady = false;
    int unreadyColumn;
    int unreadyRow;

    // "Travel to <name>?" while the game runs, and the id it would go to
    ConfirmDialog travelDialog;
    std::string pendingTravel;

    // Only active in the Development build
    TileMapEditor editor;

    // The cell whose menu asked for a new script, -1 for none
    int newScriptColumn = -1;
    int newScriptRow = -1;
};
