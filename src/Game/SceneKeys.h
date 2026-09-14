#pragma once

class Engine;

// Except in the Release build: F1 jumps to the entry point, F2 and F3 to hub
// and combat, as long as the game itself has no transitions yet.
void HandleDebugSceneKeys(Engine &engine);
