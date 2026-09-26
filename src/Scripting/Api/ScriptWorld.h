#pragma once

#include <string>
#include <vector>

class Character;

// The bridge from the scripts into the scene, filled by the engine.
//
// Scripts never touch this class: they work with scene and Object, see
// scripts/Docs.md. It exists so Object stays a small handle that looks up its
// character again on every call, and therefore never points into nothing when
// an object disappears.
class ScriptWorld {
public:
    virtual ~ScriptWorld() = default;

    // The character with this id, nullptr if the scene does not have it
    virtual Character *FindObject(int id) = 0;

    // The ids of all objects, in the order of the scene
    virtual std::vector<int> ObjectIds() const = 0;

    // The ids of the objects of one group, e.g. "enemy"
    virtual std::vector<int> ObjectIdsInGroup(const std::string &group) const = 0;
};
