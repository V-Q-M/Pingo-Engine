#include "SceneCatalog.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <system_error>
#include <utility>

#include <nlohmann/json.hpp>

#include "raylib.h"

#include "AssetFile.h"

// ordered_json keeps the order of the keys, so the saved file stays as
// readable as it was written
using json = nlohmann::ordered_json;

constexpr const char *ID_PLACEHOLDER = "{id}";

// Id from a name: lowercase letters, digits and _, e.g. "Dark Forest"
// becomes dark_forest
static std::string IdFromName(const std::string &name) {
    std::string id;

    for (char character: name) {
        if (character >= 'A' && character <= 'Z') {
            id += static_cast<char>(character - 'A' + 'a');
        } else if ((character >= 'a' && character <= 'z') || (character >= '0' && character <= '9')) {
            id += character;
        } else if (!id.empty() && id.back() != '_') {
            id += '_';
        }
    }

    while (!id.empty() && id.back() == '_') {
        id.pop_back();
    }

    return id.empty() ? "scene" : id;
}

std::vector<std::string> SceneCatalog::AvailableBackgrounds() {
    std::vector<std::string> backgrounds;

    std::error_code error;

    for (const auto &entry: std::filesystem::directory_iterator(std::string("assets/") + BACKGROUND_FOLDER, error)) {
        if (entry.path().extension() == ".png") {
            backgrounds.push_back(entry.path().filename().string());
        }
    }

    std::sort(backgrounds.begin(), backgrounds.end());

    return backgrounds;
}

void SceneCatalog::RegisterType(const std::string &name, SceneType type) {
    if (types.find(name) == types.end()) {
        typeNames.push_back(name);
    }

    types[name] = std::move(type);
}

const std::vector<std::string> &SceneCatalog::TypeNames() const {
    return typeNames;
}

const SceneType *SceneCatalog::FindType(const std::string &name) const {
    auto found = types.find(name);

    return found != types.end() ? &found->second : nullptr;
}

void SceneCatalog::Load(const std::string &filename, std::vector<SceneEntry> fallback) {
    this->filename = filename;

    entries.clear();

    std::ifstream file("assets/" + filename);

    if (!file) {
        TraceLog(LOG_WARNING, "SCENE: [%s] Datei nicht gefunden, nutze die Standardszenen", filename.c_str());

        entries = std::move(fallback);
        writable = true;
        return;
    }

    try {
        json j = json::parse(file);

        for (const json &item: j.at("scenes")) {
            SceneEntry entry;

            entry.id = item.at("id").get<std::string>();
            entry.name = item.value("name", entry.id);
            entry.type = item.at("type").get<std::string>();
            entry.background = item.value("background", std::string());
            entry.locked = item.value("locked", false);
            entry.entry = item.value("entry", false);

            if (IndexOf(entry.id) >= 0) {
                TraceLog(LOG_WARNING, "SCENE: [%s] Kennung \"%s\" doppelt, uebersprungen",
                         filename.c_str(), entry.id.c_str());
                continue;
            }

            if (FindType(entry.type) == nullptr) {
                TraceLog(LOG_WARNING, "SCENE: [%s] Art \"%s\" unbekannt", filename.c_str(), entry.type.c_str());
            }

            entries.push_back(entry);
        }

        writable = true;
    } catch (const json::exception &error) {
        TraceLog(LOG_WARNING, "SCENE: [%s] %s", filename.c_str(), error.what());

        entries = std::move(fallback);
        writable = false;
    }
}

bool SceneCatalog::Save() const {
    if (!writable || filename.empty()) {
        TraceLog(LOG_WARNING, "SCENE: [%s] war nicht lesbar und wird nicht ueberschrieben", filename.c_str());
        return false;
    }

    json list = json::array();

    for (const SceneEntry &entry: entries) {
        json item;

        item["id"] = entry.id;
        item["name"] = entry.name;
        item["type"] = entry.type;

        // Default values do not need to be in the file
        if (!entry.background.empty()) {
            item["background"] = entry.background;
        }

        if (entry.locked) {
            item["locked"] = true;
        }

        if (entry.entry) {
            item["entry"] = true;
        }

        list.push_back(item);
    }

    json j;
    j["scenes"] = list;

    bool saved = SaveAsset(filename, j.dump(2) + "\n");

    if (saved) {
        TraceLog(LOG_INFO, "SCENE: [%s] gespeichert", filename.c_str());
    } else {
        TraceLog(LOG_WARNING, "SCENE: [%s] konnte nicht gespeichert werden", filename.c_str());
    }

    return saved;
}

const std::vector<SceneEntry> &SceneCatalog::Entries() const {
    return entries;
}

std::size_t SceneCatalog::Count() const {
    return entries.size();
}

int SceneCatalog::IndexOf(const std::string &id) const {
    for (std::size_t i = 0; i < entries.size(); i++) {
        if (entries[i].id == id) {
            return static_cast<int>(i);
        }
    }

    return -1;
}

const SceneEntry *SceneCatalog::Find(const std::string &id) const {
    int index = IndexOf(id);

    return index >= 0 ? &entries[static_cast<std::size_t>(index)] : nullptr;
}

bool SceneCatalog::Enter(Engine &engine, std::size_t index) const {
    if (index >= entries.size()) {
        return false;
    }

    const SceneType *type = FindType(entries[index].type);

    if (type == nullptr || !type->enter) {
        TraceLog(LOG_WARNING, "SCENE: Art \"%s\" unbekannt", entries[index].type.c_str());
        return false;
    }

    type->enter(engine, entries[index].id);

    return true;
}

std::vector<std::string> SceneCatalog::FilesFor(const std::string &id, const std::string &type) const {
    std::vector<std::string> files;

    const SceneType *sceneType = FindType(type);

    if (sceneType == nullptr) {
        return files;
    }

    for (std::string file: sceneType->files) {
        std::size_t placeholder = file.find(ID_PLACEHOLDER);

        if (placeholder != std::string::npos) {
            file.replace(placeholder, std::string(ID_PLACEHOLDER).size(), id);
        }

        files.push_back(file);
    }

    return files;
}

std::string SceneCatalog::UniqueId(const std::string &base, const std::string &type) const {
    auto isFree = [&](const std::string &id) {
        if (IndexOf(id) >= 0) {
            return false;
        }

        // If files of an earlier scene are still lying around, they stay untouched
        for (const std::string &file: FilesFor(id, type)) {
            if (AssetExists(file)) {
                return false;
            }
        }

        return true;
    };

    std::string id = base;

    for (int number = 2; !isFree(id); number++) {
        id = base + "_" + std::to_string(number);
    }

    return id;
}

std::string SceneCatalog::UniqueCopyName(const SceneEntry &original) const {
    std::string base = original.name + " Copy";

    auto isFree = [&](const std::string &name) {
        for (const SceneEntry &entry: entries) {
            if (entry.name == name) {
                return false;
            }
        }

        return true;
    };

    std::string name = base;

    for (int number = 2; !isFree(name); number++) {
        name = base + " " + std::to_string(number);
    }

    return name;
}

bool SceneCatalog::CopyFiles(const std::vector<std::string> &sources, const std::vector<std::string> &targets) {
    for (std::size_t i = 0; i < sources.size() && i < targets.size(); i++) {
        std::ifstream file("assets/" + sources[i], std::ios::binary);

        if (!file) {
            continue;
        }

        std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        if (!SaveAsset(targets[i], text)) {
            TraceLog(LOG_WARNING, "SCENE: [%s] konnte nicht kopiert werden", sources[i].c_str());
            return false;
        }
    }

    return true;
}

std::vector<int> SceneCatalog::CompleteOptions(const SceneType &type, std::vector<int> values) {
    for (std::size_t i = values.size(); i < type.options.size(); i++) {
        values.push_back(type.options[i].initial);
    }

    values.resize(type.options.size());

    return values;
}

int SceneCatalog::Create(const std::string &name,
                         const std::string &type,
                         std::size_t templateIndex,
                         const std::vector<int> &values) {
    const SceneType *sceneType = FindType(type);

    if (sceneType == nullptr || name.empty()) {
        return -1;
    }

    SceneEntry entry;
    entry.id = UniqueId(IdFromName(name), type);
    entry.name = name;
    entry.type = type;

    std::vector<std::string> files = FilesFor(entry.id, type);

    // First the files of the template, then the options on top, e.g. a different size
    if (templateIndex < sceneType->templates.size()) {
        const SceneTemplate &sceneTemplate = sceneType->templates[templateIndex];

        if (!CopyFiles(sceneTemplate.files, files)) {
            return -1;
        }

        entry.background = sceneTemplate.background;
    }

    if (sceneType->applyOptions && !sceneType->applyOptions(files, values)) {
        TraceLog(LOG_WARNING, "SCENE: \"%s\" konnte nicht angelegt werden", entry.id.c_str());
        return -1;
    }

    entries.push_back(entry);

    Save();

    return static_cast<int>(entries.size() - 1);
}

std::vector<int> SceneCatalog::TemplateOptions(const std::string &type, std::size_t templateIndex) const {
    const SceneType *sceneType = FindType(type);

    if (sceneType == nullptr) {
        return {};
    }

    std::vector<int> values;

    // A template without files starts with the initial values
    if (templateIndex < sceneType->templates.size() && sceneType->readOptions &&
        !sceneType->templates[templateIndex].files.empty()) {
        values = sceneType->readOptions(sceneType->templates[templateIndex].files);
    }

    return CompleteOptions(*sceneType, values);
}

void SceneCatalog::Rename(std::size_t index, const std::string &name) {
    if (index >= entries.size() || name.empty() || entries[index].name == name) {
        return;
    }

    entries[index].name = name;

    Save();
}

std::vector<int> SceneCatalog::ReadOptions(std::size_t index) const {
    std::vector<int> values;

    const SceneType *type = index < entries.size() ? FindType(entries[index].type) : nullptr;

    if (type == nullptr) {
        return values;
    }

    if (type->readOptions) {
        values = type->readOptions(FilesFor(entries[index].id, entries[index].type));
    }

    return CompleteOptions(*type, values);
}

std::string SceneCatalog::OptionsWarning(std::size_t index, const std::vector<int> &values) const {
    const SceneType *type = index < entries.size() ? FindType(entries[index].type) : nullptr;

    if (type == nullptr || !type->optionsWarning) {
        return "";
    }

    return type->optionsWarning(FilesFor(entries[index].id, entries[index].type), values);
}

bool SceneCatalog::ApplyOptions(std::size_t index, const std::vector<int> &values) {
    const SceneType *type = index < entries.size() ? FindType(entries[index].type) : nullptr;

    if (type == nullptr || !type->applyOptions) {
        return false;
    }

    return type->applyOptions(FilesFor(entries[index].id, entries[index].type), values);
}

int SceneCatalog::Duplicate(std::size_t index) {
    if (index >= entries.size()) {
        return -1;
    }

    const SceneEntry &original = entries[index];

    // Locked and entry point stay with the original
    SceneEntry copy;
    copy.id = UniqueId(original.id + "_copy", original.type);
    copy.name = UniqueCopyName(original);
    copy.type = original.type;
    copy.background = original.background;

    // If the original lacks a file, the copy starts without it too
    if (!CopyFiles(FilesFor(original.id, original.type), FilesFor(copy.id, copy.type))) {
        return -1;
    }

    entries.insert(entries.begin() + static_cast<std::ptrdiff_t>(index + 1), copy);

    Save();

    return static_cast<int>(index + 1);
}

std::size_t SceneCatalog::CountOfType(const std::string &type) const {
    std::size_t count = 0;

    for (const SceneEntry &entry: entries) {
        if (entry.type == type) {
            count++;
        }
    }

    return count;
}

bool SceneCatalog::CanRemove(std::size_t index) const {
    if (index >= entries.size() || entries.size() <= 1) {
        return false;
    }

    // Some types must always have a scene, e.g. a menu
    const SceneType *type = FindType(entries[index].type);

    return type == nullptr || !type->required || CountOfType(entries[index].type) > 1;
}

bool SceneCatalog::Remove(std::size_t index) {
    if (!CanRemove(index)) {
        return false;
    }

    // The entry point moves on before the indices shift
    if (EntryPoint() == static_cast<int>(index)) {
        int next = NextEntryCandidate(index);

        for (std::size_t i = 0; i < entries.size(); i++) {
            entries[i].entry = static_cast<int>(i) == next;
        }
    }

    for (const std::string &file: FilesFor(entries[index].id, entries[index].type)) {
        if (!DeleteAsset(file)) {
            TraceLog(LOG_WARNING, "SCENE: [%s] konnte nicht geloescht werden", file.c_str());
        }
    }

    entries.erase(entries.begin() + static_cast<std::ptrdiff_t>(index));

    Save();

    return true;
}

void SceneCatalog::SetLocked(std::size_t index, bool locked) {
    if (index >= entries.size() || entries[index].locked == locked) {
        return;
    }

    entries[index].locked = locked;

    Save();
}

void SceneCatalog::SetBackground(std::size_t index, const std::string &background) {
    if (index >= entries.size() || entries[index].background == background) {
        return;
    }

    entries[index].background = background;

    Save();
}

int SceneCatalog::EntryPoint() const {
    for (std::size_t i = 0; i < entries.size(); i++) {
        if (entries[i].entry) {
            return static_cast<int>(i);
        }
    }

    for (std::size_t i = 0; i < entries.size(); i++) {
        const SceneType *type = FindType(entries[i].type);

        if (type != nullptr && type->entryPoint) {
            return static_cast<int>(i);
        }
    }

    return -1;
}

int SceneCatalog::NextEntryCandidate(std::size_t index) const {
    // Keep searching after the scene, wrap around at the end
    for (std::size_t step = 1; step < entries.size(); step++) {
        std::size_t candidate = (index + step) % entries.size();

        const SceneType *type = FindType(entries[candidate].type);

        if (type != nullptr && type->entryPoint) {
            return static_cast<int>(candidate);
        }
    }

    return -1;
}

void SceneCatalog::SetEntryPoint(std::size_t index, bool entry) {
    if (index >= entries.size()) {
        return;
    }

    const SceneType *type = FindType(entries[index].type);

    if (type == nullptr || !type->entryPoint) {
        return;
    }

    int target = static_cast<int>(index);

    if (!entry) {
        // Only the entry point itself can be unchecked, and only if there is a successor
        if (EntryPoint() != target) {
            return;
        }

        target = NextEntryCandidate(index);

        if (target < 0) {
            return;
        }
    }

    bool changed = false;

    for (std::size_t i = 0; i < entries.size(); i++) {
        bool marked = static_cast<int>(i) == target;

        if (entries[i].entry != marked) {
            entries[i].entry = marked;
            changed = true;
        }
    }

    if (changed) {
        Save();
    }
}
