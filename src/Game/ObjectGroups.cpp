#include "ObjectGroups.h"

#include <algorithm>
#include <fstream>

#include <nlohmann/json.hpp>

#include "raylib.h"

#include "Engine/AssetFile.h"
#include "Engine/Names.h"

// ordered_json keeps the order of the keys, so the file stays readable
using json = nlohmann::ordered_json;

ObjectGroups::ObjectGroups() : groups{{ALLY, "Ally"}, {ENEMY, "Enemy"}} {
}

ObjectGroups ObjectGroups::Load() {
    ObjectGroups result;

    std::ifstream file(std::string("assets/") + GROUPS_FILE);

    if (!file) {
        return result;
    }

    try {
        json j = json::parse(file);

        for (const json &item: j.at("groups")) {
            ObjectGroup group{item.at("id").get<std::string>(), item.value("name", std::string())};

            // The default groups are always there, with their own names
            if (group.id.empty() || IsDefault(group.id) || result.IndexOf(group.id) >= 0) {
                continue;
            }

            if (group.name.empty()) {
                group.name = group.id;
            }

            result.groups.push_back(group);
        }
    } catch (const json::exception &error) {
        TraceLog(LOG_WARNING, "GROUPS: [%s] %s", GROUPS_FILE, error.what());
    }

    return result;
}

bool ObjectGroups::Save() const {
    json list = json::array();

    for (const ObjectGroup &group: groups) {
        json item;

        item["id"] = group.id;
        item["name"] = group.name;

        list.push_back(item);
    }

    json j;
    j["groups"] = list;

    return SaveAsset(GROUPS_FILE, j.dump(2) + "\n");
}

const std::vector<ObjectGroup> &ObjectGroups::All() const {
    return groups;
}

int ObjectGroups::IndexOf(const std::string &id) const {
    for (std::size_t i = 0; i < groups.size(); i++) {
        if (groups[i].id == id) {
            return static_cast<int>(i);
        }
    }

    return -1;
}

std::string ObjectGroups::NameOf(const std::string &id) const {
    int index = IndexOf(id);

    return index >= 0 ? groups[static_cast<std::size_t>(index)].name : id;
}

bool ObjectGroups::IsNameTaken(const std::string &name, const std::string &exceptId) const {
    std::string wanted = ToLower(name);

    return std::any_of(groups.begin(), groups.end(), [&](const ObjectGroup &group) {
        return group.id != exceptId && ToLower(group.name) == wanted;
    });
}

bool ObjectGroups::IsDefault(const std::string &id) {
    return id == ALLY || id == ENEMY;
}

std::string ObjectGroups::Create(const std::string &name) {
    std::string base = IdFromName(name, "group");
    std::string id = base;

    for (int number = 2; IndexOf(id) >= 0; number++) {
        id = base + "_" + std::to_string(number);
    }

    groups.push_back({id, name});

    return id;
}

bool ObjectGroups::Rename(const std::string &id, const std::string &name) {
    int index = IndexOf(id);

    if (index < 0 || IsDefault(id)) {
        return false;
    }

    groups[static_cast<std::size_t>(index)].name = name;

    return true;
}

bool ObjectGroups::Remove(const std::string &id) {
    int index = IndexOf(id);

    if (index < 0 || IsDefault(id)) {
        return false;
    }

    groups.erase(groups.begin() + index);

    return true;
}
