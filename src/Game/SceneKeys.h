#pragma once

class Engine;

// Except in the Release build: F1 to F9 jump to the scenes in the order of
// the scene bar, Alt and a number does the same.
void HandleDebugSceneKeys(Engine &engine);
