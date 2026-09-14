#pragma once

#include <string>
#include <vector>

#include "FieldScene.h"
#include "Engine/GridMover.h"
#include "Engine/TileMap.h"
#include "Engine/TileMapEditor.h"

// A world made of cells, e.g. the hub. The player character walks cell by
// cell across the map, the camera follows it. The map is stored in
// assets/maps/<id>.txt.
class TilesetScene : public FieldScene {
public:
    // id chooses the map, e.g. "hub" for assets/maps/hub.txt
    explicit TilesetScene(Engine &engine, const std::string &id = "hub");

    void UpdateDevelopment(float dt) override;

    void DrawUI() override;

    bool OnEscape() override;

    void LeaveFromPauseMenu() override;

protected:
    void MovePlayer(Character &player, float dt, const std::vector<SpriteInstance *> &all) override;

    void DrawGround() override;

    void DrawOverlay() override;

private:
    // Where the player character starts, further to the top left on smaller maps
    static constexpr int START_COLUMN = 2;
    static constexpr int START_ROW = 1;

    // Before the GridMover, which holds a reference to the map
    TileMap map;

    int startColumn;
    int startRow;

    GridMover mover;

    // Only active in the Development build
    TileMapEditor editor;
};
