#include "Api/SceneApi.h"

void SceneApi::Attach(ScriptWorld *world) {
    this->world = world;
}

Object SceneApi::GetObjectById(int id) const {
    return Object(world, id);
}

std::vector<Object> SceneApi::GetObjects() const {
    std::vector<Object> objects;

    if (world == nullptr) {
        return objects;
    }

    for (int id: world->ObjectIds()) {
        objects.push_back(Object(world, id));
    }

    return objects;
}

std::vector<Object> SceneApi::GetObjectsInGroup(const std::string &group) const {
    std::vector<Object> objects;

    if (world == nullptr) {
        return objects;
    }

    for (int id: world->ObjectIdsInGroup(group)) {
        objects.push_back(Object(world, id));
    }

    return objects;
}

int SceneApi::GetObjectCount() const {
    return world != nullptr ? static_cast<int>(world->ObjectIds().size()) : 0;
}
