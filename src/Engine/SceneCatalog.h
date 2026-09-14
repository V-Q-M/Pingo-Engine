#pragma once

#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "SceneEntry.h"

class Engine;

// A number that can be set when creating and editing a scene, e.g. the
// width of a map
struct SceneNumberOption {
    std::string label;

    int minimum = 1;
    int maximum = 99;

    // Value when creating a new scene
    int initial = 1;
};

// What a new scene starts with, e.g. "Default" with the files of the hub
struct SceneTemplate {
    // Name in the choice
    std::string label;

    // Files relative to the asset folder, in the order of SceneType::files.
    // Empty: the scene starts without files, so empty.
    std::vector<std::string> files;

    // Background image of the new scene, see SceneEntry::background
    std::string background;
};

// A type of scene, e.g. "tileset". Any number of scenes can be created from
// one type, each with its own id and its own files.
struct SceneType {
    // Name in the window for new scenes, e.g. "Tileset"
    std::string label;

    // Schedules the change to the scene with this id, e.g.
    // engine.ChangeScene<NormalScene>(id)
    std::function<void(Engine &, const std::string &)> enter;

    // Files of a scene relative to the asset folder, {id} stands for the id,
    // e.g. "scenes/{id}.json". They are copied when duplicating and deleted when
    // deleting.
    std::vector<std::string> files;

    // There must always be at least one scene of this type
    bool required = false;

    // A scene of this type can be the entry point of the game
    bool entryPoint = false;

    // The templates in the window for new scenes, the first one is preselected
    std::vector<SceneTemplate> templates;

    // Adjustable numbers, e.g. columns and rows of a map
    std::vector<SceneNumberOption> options;

    // The values of the options, read from the files of a scene or a template.
    // files is in the order of files above.
    std::function<std::vector<int>(const std::vector<std::string> &files)> readOptions;

    // Warning before new values apply to an existing scene, e.g. because
    // something gets lost. Empty if there is nothing to warn about.
    std::function<std::string(const std::vector<std::string> &files, const std::vector<int> &values)> optionsWarning;

    // Applies the values to the files of a scene, e.g. creates a map or changes
    // its size. false on an error.
    std::function<bool(const std::vector<std::string> &files, const std::vector<int> &values)> applyOptions;
};

// All scenes of the game in their order, saved as JSON under assets.
//
// The game defines which types exist with RegisterType. The list itself is
// stored in a file and can be changed in the Development mode via the scene
// bar: create, edit, duplicate, lock and delete scenes. Changes are saved
// right away.
class SceneCatalog {
public:
    // Folder under assets with the images a scene can have as its background
    static constexpr const char *BACKGROUND_FOLDER = "backgrounds";

    // All backgrounds as file names, alphabetical, e.g. "map.png"
    static std::vector<std::string> AvailableBackgrounds();

    void RegisterType(const std::string &name, SceneType type);

    // The types in the order they were registered
    const std::vector<std::string> &TypeNames() const;

    // nullptr if the type does not exist
    const SceneType *FindType(const std::string &name) const;

    // Loads the list from assets/<filename>. If the file is missing, fallback
    // applies and is saved on the first change. If it is broken, fallback applies
    // as well, but the file is then never overwritten.
    void Load(const std::string &filename, std::vector<SceneEntry> fallback);

    bool Save() const;

    const std::vector<SceneEntry> &Entries() const;

    std::size_t Count() const;

    // -1 if the scene does not exist
    int IndexOf(const std::string &id) const;

    // nullptr if the scene does not exist
    const SceneEntry *Find(const std::string &id) const;

    // Schedules the change to the scene. false if it or its type does not exist.
    bool Enter(Engine &engine, std::size_t index) const;

    // Appends a new scene, the id is derived from the name. It starts with the
    // files of the template, then the values of the options apply. Index of the
    // scene, -1 on an error.
    int Create(const std::string &name,
               const std::string &type,
               std::size_t templateIndex,
               const std::vector<int> &values);

    // The values of the options a template starts with, e.g. the size of the hub
    std::vector<int> TemplateOptions(const std::string &type, std::size_t templateIndex) const;

    void Rename(std::size_t index, const std::string &name);

    // The values of the options of a scene. If the type cannot read them, the initial values.
    std::vector<int> ReadOptions(std::size_t index) const;

    // Empty if the values can apply without confirmation
    std::string OptionsWarning(std::size_t index, const std::vector<int> &values) const;

    bool ApplyOptions(std::size_t index, const std::vector<int> &values);

    // Puts a copy with a new id directly after the original, including copies of
    // its files. Index of the copy, -1 on an error.
    int Duplicate(std::size_t index);

    // The last scene and the last scene of a type that must always exist cannot
    // be deleted
    bool CanRemove(std::size_t index) const;

    // Deletes the scene and its files. If it was the entry point, the next scene
    // that can be one becomes it. Cannot be undone.
    bool Remove(std::size_t index);

    void SetLocked(std::size_t index, bool locked);

    // Empty for no background
    void SetBackground(std::size_t index, const std::string &background);

    // Where the game starts: the marked scene, without a mark the first one that
    // can be one. -1 if there is none.
    int EntryPoint() const;

    // With true the scene becomes the entry point, all others lose the mark.
    // With false the entry point passes it on to the next scene that can be one.
    // If there is none, it stays the entry point.
    void SetEntryPoint(std::size_t index, bool entry);

private:
    // The files of a scene, with its id instead of {id}
    std::vector<std::string> FilesFor(const std::string &id, const std::string &type) const;

    // Id that is neither taken nor overwrites existing files, e.g. base,
    // otherwise base_2, base_3 and so on
    std::string UniqueId(const std::string &base, const std::string &type) const;

    // Name for a copy, e.g. "Combat Copy" or "Combat Copy 2"
    std::string UniqueCopyName(const SceneEntry &original) const;

    std::size_t CountOfType(const std::string &type) const;

    // Fills missing values with the initial values of the options
    static std::vector<int> CompleteOptions(const SceneType &type, std::vector<int> values);

    // Copies file by file into both asset folders. If a source is missing, its
    // target is skipped. false if a copy fails.
    static bool CopyFiles(const std::vector<std::string> &sources, const std::vector<std::string> &targets);

    // The next scene after index that can be the entry point, -1 if none
    int NextEntryCandidate(std::size_t index) const;

    std::string filename;

    std::vector<SceneEntry> entries;

    std::map<std::string, SceneType> types;

    std::vector<std::string> typeNames;

    // If the file could not be read, it is not overwritten
    bool writable = false;
};
