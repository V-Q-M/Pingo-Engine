#pragma once

class Engine;

// "Changes detected" and the undo keys at the top right, for developer tools.
// Belongs in the screen layer and moves below the FPS display if it is on.
void DrawEditStatus(Engine &engine, bool modified, bool canUndo, bool canRedo);
