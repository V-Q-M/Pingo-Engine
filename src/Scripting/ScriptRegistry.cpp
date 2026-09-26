#include "Api/ScriptRegistry.h"

#include <algorithm>
#include <utility>

std::vector<ScriptRegistry::Entry> &ScriptRegistry::Entries() {
    static std::vector<Entry> entries;

    return entries;
}

// A script that registers itself again replaces the one before it: that way
// the game always runs the newest version of it
void ScriptRegistry::Add(const std::string &scene, const std::string &name, Factory factory) {
    for (Entry &entry: Entries()) {
        if (entry.scene == scene && entry.name == name) {
            entry.factory = std::move(factory);
            return;
        }
    }

    Entries().push_back({scene, name, std::move(factory)});
}

bool ScriptRegistry::Knows(const std::string &scene, const std::string &name) {
    return std::any_of(Entries().begin(), Entries().end(), [&](const Entry &entry) {
        return entry.scene == scene && entry.name == name;
    });
}

std::unique_ptr<ScriptBase> ScriptRegistry::Create(const std::string &scene, const std::string &name) {
    for (const Entry &entry: Entries()) {
        if (entry.scene == scene && entry.name == name && entry.factory) {
            return entry.factory();
        }
    }

    return nullptr;
}

std::vector<std::string> ScriptRegistry::Names(const std::string &scene) {
    std::vector<std::string> names;

    for (const Entry &entry: Entries()) {
        if (entry.scene == scene) {
            names.push_back(entry.name);
        }
    }

    std::sort(names.begin(), names.end());

    return names;
}
