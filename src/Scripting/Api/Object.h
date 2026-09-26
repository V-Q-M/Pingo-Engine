#pragma once

#include <string>

#include "ScriptWorld.h"

// One object of the scene, e.g. a character:
//
//     Object pingo = scene.GetObjectById(1);
//
//     pingo.SetHealth(pingo.GetHealth() - 10);
//     pingo.SetEffect("burning");
//
// An Object is only a handle: it looks its object up again on every call. An
// object that does not exist, or no longer exists, is empty: Exists() is
// false, reading gives 0 and an empty text, and setting does nothing. A script
// therefore never has to check before it acts.
//
// Everything a script changes is gone when the scene is left: scripts run
// while the game runs and never write into the files of the scene.
class Object {
public:
    // The empty object, see the class comment
    Object() = default;

    Object(ScriptWorld *world, int id);

    // Is there an object with this id in the scene?
    bool Exists() const;

    // The id it was looked up with, 0 for the empty object
    int GetId() const;

    // Name of the object, e.g. "Pingo"
    std::string GetName() const;

    // Id of its group, e.g. "ally" or "enemy"
    std::string GetGroup() const;

    bool IsInGroup(const std::string &group) const;

    // Health from 0 to GetMaxHealth. Objects without health, e.g. a rock,
    // have a maximum of 0 and cannot be hurt.
    int GetHealth() const;

    void SetHealth(int health);

    int GetMaxHealth() const;

    // Energy from 0 to GetMaxEnergy, 0 for an object without energy
    int GetEnergy() const;

    void SetEnergy(int energy);

    int GetMaxEnergy() const;

    // Where the object stands, its feet, in pixels of the world
    float GetX() const;

    void SetX(float x);

    float GetY() const;

    void SetY(float y);

    void SetPosition(float x, float y);

    // Moves it by this much, e.g. Move(0.0f, 20.0f * dt) for falling
    void Move(float x, float y);

    // The effect it is in, empty for none. Known effects are burning,
    // poisoned, stunned and angry, see scripts/Docs.md.
    std::string GetEffect() const;

    void SetEffect(const std::string &effect);

    bool HasEffect(const std::string &effect) const;

    // Takes the effect off again, the same as SetEffect("")
    void ClearEffect();

private:
    // The character behind the id, nullptr if it is gone
    Character *Find() const;

    ScriptWorld *world = nullptr;

    int id = 0;
};
