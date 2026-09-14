#include "SpriteInstance.h"

SpriteInstance::SpriteInstance(AnimatedSprite sprite, Vector2 position)
    : sprite(std::move(sprite)),
      position(position) {
    this->sprite.RandomizeAnimation();

    // Without its own values the hitbox covers the whole sprite frame
    hitbox = {
        static_cast<float>(this->sprite.Width()),
        static_cast<float>(this->sprite.Height()),
        0.0f
    };
}

const Hitbox &SpriteInstance::GetHitbox() const {
    return hitbox;
}

void SpriteInstance::SetHitbox(Hitbox hitbox) {
    this->hitbox = hitbox;
}

const Texture2D *SpriteInstance::Selector() const {
    return selector;
}

void SpriteInstance::SetSelector(const Texture2D *texture) {
    selector = texture;
}

bool SpriteInstance::IsSolid() const {
    return solid;
}

void SpriteInstance::SetSolid(bool solid) {
    this->solid = solid;
}

bool SpriteInstance::IsColliding() const {
    return colliding;
}

void SpriteInstance::SetColliding(bool colliding) {
    this->colliding = colliding;
}

Rectangle SpriteInstance::Bounds() const {
    return {
        position.x - hitbox.width / 2.0f,
        position.y + hitbox.offsetY - hitbox.height,
        hitbox.width,
        hitbox.height
    };
}

AnimatedSprite &SpriteInstance::Sprite() {
    return sprite;
}

const AnimatedSprite &SpriteInstance::Sprite() const {
    return sprite;
}

const Vector2 &SpriteInstance::Position() const {
    return position;
}

void SpriteInstance::SetPosition(Vector2 position) {
    this->position = position;
}

float SpriteInstance::Z() const {
    return position.y + zOffset;
}

float SpriteInstance::ZOffset() const {
    return zOffset;
}

void SpriteInstance::SetZOffset(float offset) {
    zOffset = offset;
}

Rectangle SpriteInstance::EditBounds() const {
    return Bounds();
}

Vector2 SpriteInstance::EditPosition() const {
    return position;
}

void SpriteInstance::SetEditPosition(Vector2 position) {
    SetPosition(position);
}

float SpriteInstance::EditDepth() const {
    return Z();
}
