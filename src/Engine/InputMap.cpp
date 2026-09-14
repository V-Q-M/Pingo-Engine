#include "InputMap.h"

#include <cmath>

InputMap::InputMap() {
    Bind(InputAction::MoveUp, KEY_UP, InputScheme::ArrowKeys);
    Bind(InputAction::MoveDown, KEY_DOWN, InputScheme::ArrowKeys);
    Bind(InputAction::MoveLeft, KEY_LEFT, InputScheme::ArrowKeys);
    Bind(InputAction::MoveRight, KEY_RIGHT, InputScheme::ArrowKeys);

    Bind(InputAction::MoveUp, KEY_W, InputScheme::Wasd);
    Bind(InputAction::MoveDown, KEY_S, InputScheme::Wasd);
    Bind(InputAction::MoveLeft, KEY_A, InputScheme::Wasd);
    Bind(InputAction::MoveRight, KEY_D, InputScheme::Wasd);
}

void InputMap::Bind(InputAction action, int key, InputScheme scheme) {
    bindings.push_back({action, key, scheme});

    enabledSchemes.emplace(scheme, true);
}

void InputMap::SetSchemeEnabled(InputScheme scheme, bool enabled) {
    enabledSchemes[scheme] = enabled;
}

bool InputMap::IsSchemeEnabled(InputScheme scheme) const {
    auto it = enabledSchemes.find(scheme);

    if (it == enabledSchemes.end()) {
        return false;
    }

    return it->second;
}

void InputMap::SetMovementEnabled(bool enabled) {
    movementEnabled = enabled;
}

bool InputMap::IsMovementEnabled() const {
    return movementEnabled;
}

bool InputMap::IsActionEnabled(InputAction action) const {
    switch (action) {
        case InputAction::MoveUp:
        case InputAction::MoveDown:
        case InputAction::MoveLeft:
        case InputAction::MoveRight:
            return movementEnabled;
    }

    return true;
}

bool InputMap::IsDown(InputAction action) const {
    if (!IsActionEnabled(action)) {
        return false;
    }

    for (const Binding &binding: bindings) {
        if (binding.action != action || !IsSchemeEnabled(binding.scheme)) {
            continue;
        }

        if (IsKeyDown(binding.key)) {
            return true;
        }
    }

    return false;
}

bool InputMap::WasPressed(InputAction action) const {
    if (!IsActionEnabled(action)) {
        return false;
    }

    for (const Binding &binding: bindings) {
        if (binding.action != action || !IsSchemeEnabled(binding.scheme)) {
            continue;
        }

        if (IsKeyPressed(binding.key)) {
            return true;
        }
    }

    return false;
}

Vector2 InputMap::MovementDirection() const {
    Vector2 direction{0.0f, 0.0f};

    if (IsDown(InputAction::MoveLeft)) {
        direction.x -= 1.0f;
    }

    if (IsDown(InputAction::MoveRight)) {
        direction.x += 1.0f;
    }

    if (IsDown(InputAction::MoveUp)) {
        direction.y -= 1.0f;
    }

    if (IsDown(InputAction::MoveDown)) {
        direction.y += 1.0f;
    }

    // Without normalization you would be faster diagonally by a factor of sqrt(2)
    float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);

    if (length > 0.0f) {
        direction.x /= length;
        direction.y /= length;
    }

    return direction;
}
