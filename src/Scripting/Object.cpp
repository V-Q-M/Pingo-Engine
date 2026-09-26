#include "Api/Object.h"

#include "Game/Character.h"
#include "Game/ObjectEffects.h"

Object::Object(ScriptWorld *world, int id)
    : world(world),
      id(id) {
}

Character *Object::Find() const {
    return world != nullptr ? world->FindObject(id) : nullptr;
}

bool Object::Exists() const {
    return Find() != nullptr;
}

int Object::GetId() const {
    return Exists() ? id : 0;
}

std::string Object::GetName() const {
    const Character *character = Find();

    return character != nullptr ? character->Name() : "";
}

std::string Object::GetGroup() const {
    const Character *character = Find();

    return character != nullptr ? character->Group() : "";
}

bool Object::IsInGroup(const std::string &group) const {
    return GetGroup() == group;
}

int Object::GetHealth() const {
    const Character *character = Find();

    return character != nullptr ? character->Health() : 0;
}

void Object::SetHealth(int health) {
    if (Character *character = Find()) {
        character->SetHealth(health);
    }
}

int Object::GetMaxHealth() const {
    const Character *character = Find();

    return character != nullptr ? character->MaxHealth() : 0;
}

int Object::GetEnergy() const {
    const Character *character = Find();

    return character != nullptr ? character->Energy() : 0;
}

void Object::SetEnergy(int energy) {
    if (Character *character = Find()) {
        character->SetEnergy(energy);
    }
}

int Object::GetMaxEnergy() const {
    const Character *character = Find();

    return character != nullptr ? character->MaxEnergy() : 0;
}

float Object::GetX() const {
    const Character *character = Find();

    return character != nullptr ? character->GetCharacter().Position().x : 0.0f;
}

void Object::SetX(float x) {
    SetPosition(x, GetY());
}

float Object::GetY() const {
    const Character *character = Find();

    return character != nullptr ? character->GetCharacter().Position().y : 0.0f;
}

void Object::SetY(float y) {
    SetPosition(GetX(), y);
}

void Object::SetPosition(float x, float y) {
    if (Character *character = Find()) {
        character->GetCharacter().SetPosition({x, y});
    }
}

void Object::Move(float x, float y) {
    SetPosition(GetX() + x, GetY() + y);
}

std::string Object::GetEffect() const {
    const Character *character = Find();

    return character != nullptr ? character->Effect() : "";
}

void Object::SetEffect(const std::string &effect) {
    Character *character = Find();

    if (character == nullptr) {
        return;
    }

    // An effect nobody knows would only look like it works
    if (!effect.empty() && ObjectEffects::Find(effect) == nullptr) {
        return;
    }

    character->SetEffect(effect);
}

bool Object::HasEffect(const std::string &effect) const {
    return GetEffect() == effect;
}

void Object::ClearEffect() {
    SetEffect(ObjectEffects::NONE);
}
