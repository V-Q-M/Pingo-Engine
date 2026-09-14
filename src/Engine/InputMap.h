#pragma once

#include <unordered_map>
#include <vector>

#include "raylib.h"

#include "InputAction.h"
#include "InputScheme.h"

class InputMap {
public:
    InputMap();

    void Bind(InputAction action, int key, InputScheme scheme);

    void SetSchemeEnabled(InputScheme scheme, bool enabled);

    bool IsSchemeEnabled(InputScheme scheme) const;

    // Turns off the movement actions, no matter which scheme. A scene without
    // movement then always gets (0, 0) as its direction.
    void SetMovementEnabled(bool enabled);

    bool IsMovementEnabled() const;

    bool IsDown(InputAction action) const;

    bool WasPressed(InputAction action) const;

    Vector2 MovementDirection() const;

private:
    bool IsActionEnabled(InputAction action) const;

    struct Binding {
        InputAction action;
        int key;
        InputScheme scheme;
    };

    std::vector<Binding> bindings;

    std::unordered_map<InputScheme, bool> enabledSchemes;

    bool movementEnabled = true;
};
