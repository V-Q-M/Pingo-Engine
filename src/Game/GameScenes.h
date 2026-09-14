#pragma once

class SceneCatalog;

// The scenes of the game: defines their types Menu, Tileset and Normal and
// loads the list from assets/scenes/scenes.json. Whoever builds a new type of
// scene registers it here.
void RegisterGameScenes(SceneCatalog &catalog);
