#pragma  once

#include "AnimatedSprite.h"
#include "EditableObject.h"
#include "Hitbox.h"
#include "algorithm"
#include "raylib.h"

class SpriteInstance : public EditableObject {
public:
    SpriteInstance(AnimatedSprite sprite, Vector2 position);

    AnimatedSprite &Sprite();

    const AnimatedSprite &Sprite() const;

    const Vector2 &Position() const;

    void SetPosition(Vector2 position);

    const Hitbox &GetHitbox() const;

    void SetHitbox(Hitbox hitbox);

    // The hitbox in world coordinates
    Rectangle Bounds() const;

    // Ring below the character, nullptr if none is shown
    const Texture2D *Selector() const;

    void SetSelector(const Texture2D *texture);

    // Solid characters cannot walk into each other
    bool IsSolid() const;

    void SetSolid(bool solid);

    // Does this character currently overlap with another one?
    // Set by Collision::UpdateFlags.
    bool IsColliding() const;

    void SetColliding(bool colliding);

    // Drawing depth: whatever has a larger Z is drawn further in front.
    // Z is derived from the Y position, so characters standing lower
    // automatically lie in front of those standing higher. It therefore follows
    // the movement on its own and never has to be updated.
    float Z() const;

    float ZOffset() const;

    void SetZOffset(float offset);

    // For developer tools: hitbox, position and Z
    Rectangle EditBounds() const override;

    Vector2 EditPosition() const override;

    void SetEditPosition(Vector2 position) override;

    float EditDepth() const override;

private:
    AnimatedSprite sprite;
    Vector2 position;

    Hitbox hitbox;

    const Texture2D *selector = nullptr;

    bool solid = false;
    bool colliding = false;

    float zOffset = 0.0f;
};


