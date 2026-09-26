#pragma once

#include <string>
#include <vector>

#include "Object.h"

// The scene a script belongs to. Every script has it ready as "scene":
//
//     Object pingo = scene.GetObjectById(1);
//
//     for (Object enemy: scene.GetObjectsInGroup("enemy")) {
//         enemy.SetEffect("burning");
//     }
//
// The ids are the ones the editor shows next to the name of an object when
// "Object ids" is on in the settings, and the ones the console uses.
class SceneApi {
public:
    // The object with this id. An id nobody has gives an empty object that
    // does nothing, see Object.
    Object GetObjectById(int id) const;

    // Every object of the scene, in the order of its file
    std::vector<Object> GetObjects() const;

    // Every object of one group, e.g. "enemy"
    std::vector<Object> GetObjectsInGroup(const std::string &group) const;

    // How many objects the scene has
    int GetObjectCount() const;

private:
    friend class ScriptHost;

    // The engine hands the scene to the script, see ScriptHost
    void Attach(ScriptWorld *world);

    ScriptWorld *world = nullptr;
};
