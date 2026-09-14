#include "Character.h"

#include <algorithm>

#include "Engine/Collision.h"

Character::Character(Assets &assets,
                     const CharacterDefinition &definition,
                     Vector2 position)
    : name(definition.name),
      definitionFile(definition.file),
      moveSpeed(definition.moveSpeed),
      health(definition.maxHealth),
      maxHealth(definition.maxHealth),
      character(
          AnimatedSprite(
              definition.IsDummy()
                  ? assets.Texture().GetSolid(definition.color, definition.frameWidth, definition.frameHeight)
                  : assets.Texture().Get(definition.texture),
              definition.frameWidth,
              definition.frameHeight,
              definition.frameCount,
              definition.shadowWidth,
              definition.shadowHeight,
              definition.shadowOffsetY
          ),
          position
      ) {
    character.Sprite().SetAnimationSpeed(definition.animationSpeed);

    character.SetSolid(definition.isSolid);

    character.SetHitbox({
        definition.hitboxWidth,
        definition.hitboxHeight,
        definition.hitboxOffsetY
    });
}

void Character::Move(Vector2 direction,
                     float dt,
                     const std::vector<SpriteInstance *> &obstacles) {
    Vector2 delta = {
        direction.x * moveSpeed * dt,
        direction.y * moveSpeed * dt
    };

    Collision::MoveWithCollision(character, delta, obstacles);
}

const std::string &Character::Name() const {
    return name;
}

const std::string &Character::DefinitionFile() const {
    return definitionFile;
}

float Character::MoveSpeed() const {
    return moveSpeed;
}

int Character::Health() const {
    return health;
}

int Character::MaxHealth() const {
    return maxHealth;
}

void Character::SetHealth(int value) {
    health = std::clamp(value, 0, maxHealth);
}

bool Character::IsIndestructible() const {
    return maxHealth <= 0;
}

float Character::HealthFraction() const {
    if (maxHealth <= 0) {
        return 0.0f;
    }

    return static_cast<float>(health) / static_cast<float>(maxHealth);
}

SpriteInstance &Character::GetCharacter() {
    return character;
}

const SpriteInstance &Character::GetCharacter() const {
    return character;
}
