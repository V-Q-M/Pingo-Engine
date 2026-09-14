#pragma once

#include "raylib.h"

// Something developer tools like the ObjectEditor can grab and move,
// e.g. a character in the world or a button in the main menu.
class EditableObject {
public:
    EditableObject() = default;

    EditableObject(const EditableObject &) = default;

    EditableObject(EditableObject &&) = default;

    EditableObject &operator=(const EditableObject &) = default;

    EditableObject &operator=(EditableObject &&) = default;

    virtual ~EditableObject() = default;

    // The area that reacts to clicks
    virtual Rectangle EditBounds() const = 0;

    // The position the editor shows and moves
    virtual Vector2 EditPosition() const = 0;

    virtual void SetEditPosition(Vector2 position) = 0;

    // If objects overlap, the click goes to the one with the largest depth:
    // the one that is also drawn on top
    virtual float EditDepth() const = 0;
};
