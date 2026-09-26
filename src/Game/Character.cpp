#include "Character.h"

#include <algorithm>
#include <utility>

#include "Engine/Collision.h"

Character::Character(Assets &assets,
                     const CharacterDefinition &definition,
                     Vector2 position)
    : name(definition.name),
      definitionFile(definition.file),
      moveSpeed(definition.moveSpeed),
      health(definition.maxHealth),
      maxHealth(definition.maxHealth),
      energy(definition.maxEnergy),
      maxEnergy(definition.maxEnergy),
      effectRows(definition.effectRows),
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
              definition.shadowOffsetY,
              definition.shadowOffsetX
          ),
          position
      ) {
    character.Sprite().SetAnimationSpeed(definition.animationSpeed);
    character.Sprite().SetShadowBounce(definition.shadowBounce);

    character.SetSolid(definition.isSolid);

    character.SetHitbox({
        definition.hitboxWidth,
        definition.hitboxHeight,
        definition.hitboxOffsetY,
        definition.hitboxOffsetX
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

const CharacterOverrides &Character::Overrides() const {
    return overrides;
}

void Character::SetOverrides(CharacterOverrides overrides) {
    this->overrides = std::move(overrides);
}

const std::string &Character::Group() const {
    return group;
}

void Character::SetGroup(std::string group) {
    this->group = std::move(group);
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

int Character::Energy() const {
    return energy;
}

int Character::MaxEnergy() const {
    return maxEnergy;
}

void Character::SetEnergy(int value) {
    energy = std::clamp(value, 0, maxEnergy);
}

bool Character::HasEnergy() const {
    return maxEnergy > 0;
}

float Character::EnergyFraction() const {
    if (maxEnergy <= 0) {
        return 0.0f;
    }

    return static_cast<float>(energy) / static_cast<float>(maxEnergy);
}

const std::string &Character::Effect() const {
    return effect;
}

// A character without a row for the effect keeps animating its normal row: the
// effect is still on it, it just does not show.
void Character::SetEffect(const std::string &effect) {
    this->effect = effect;

    auto row = effectRows.find(effect);

    // In the files the rows are counted from 1, the sheet starts at 0
    character.Sprite().SetRow(row != effectRows.end() ? row->second - 1 : 0);
}

bool Character::ShowsEffect(const std::string &effect) const {
    return effectRows.find(effect) != effectRows.end();
}

SpriteInstance &Character::GetCharacter() {
    return character;
}

const SpriteInstance &Character::GetCharacter() const {
    return character;
}
