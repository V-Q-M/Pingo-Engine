#pragma once

#include <string>
#include <vector>

// A group characters belong to, e.g. the allies of the player
struct ObjectGroup {
    // Stays the same when the group is renamed. Scene files only store the id.
    std::string id;

    std::string name;
};

// The groups of all scenes, stored in assets/scenes/groups.json.
//
// ally and enemy always exist, come first and can neither be renamed nor
// deleted. Further groups are created in the editor. Because scene files only
// store the id, renaming a group does not touch them.
class ObjectGroups {
public:
    static constexpr const char *ALLY = "ally";
    static constexpr const char *ENEMY = "enemy";

    static constexpr const char *GROUPS_FILE = "scenes/groups.json";

    // Only the two default groups
    ObjectGroups();

    // Missing or broken file: only the two default groups
    static ObjectGroups Load();

    // Saves like SaveAsset into both asset folders
    bool Save() const;

    const std::vector<ObjectGroup> &All() const;

    // -1 if there is no group with this id
    int IndexOf(const std::string &id) const;

    // The name of a group, the id itself for an unknown one
    std::string NameOf(const std::string &id) const;

    // Does a group other than exceptId already have this name? Upper and lower
    // case do not matter.
    bool IsNameTaken(const std::string &name, const std::string &exceptId = "") const;

    static bool IsDefault(const std::string &id);

    // Appends a group and returns its id, derived from the name
    std::string Create(const std::string &name);

    // false for the default groups and unknown ids
    bool Rename(const std::string &id, const std::string &name);

    bool Remove(const std::string &id);

private:
    std::vector<ObjectGroup> groups;
};
