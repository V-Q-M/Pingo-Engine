#include "ScriptFiles.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <system_error>

#include "raylib.h"

#include "Engine/AssetFile.h"

// What a new script starts with. The Main of a scene gets the Update the
// engine calls, every other script of a normal scene starts with an example
// method. A tile script gets its Update as well: the engine calls it for
// every tile it sits on.
static std::string HeaderTemplate(const std::string &scene, const std::string &name, ScriptFiles::Kind kind) {
    const bool tile = kind == ScriptFiles::Kind::Tile;
    const bool main = !tile && name == ScriptFiles::MAIN;

    // Why the namespace: the folder of the scene is also the namespace of
    // its scripts, so two scenes can both have a Gravity without ever
    // meaning the same class
    std::string text = std::string(ScriptFiles::HEADER_GUARD) + "\n\n" +
                       (tile ? ScriptFiles::TILE_SCRIPT_INCLUDE : ScriptFiles::SCRIPT_INCLUDE) + "\n\n"
                       "// Every script of the scene \"" + scene + "\" lives in this namespace, so\n"
                       "// another scene can have a script of the same name\n"
                       "namespace " + scene + " {\n";

    if (tile) {
        text += "    // A script on the tiles of the map: the engine calls its Update every\n"
                "    // frame while the game runs, once for every tile it is assigned to.\n";
    } else if (main) {
        text += "    // The script of the scene: the engine calls its Update every frame\n"
                "    // while the game runs. Call your own scripts from here.\n";
    } else {
        text += "    // A script of the scene. Its public methods can be called from\n"
                "    // Main or from another script of this scene.\n";
    }

    text += "    class " + name + " : public " + (tile ? "TileScript" : "Script") + " {\n    public:\n";

    if (main || tile) {
        text += "        void Update(float dt) override;\n";
    } else {
        text += "        // Runs whenever " + std::string(ScriptFiles::MAIN) + " calls it\n"
                "        void Run(float dt);\n";
    }

    text += "    };\n}\n";

    return text;
}

static std::string SourceTemplate(const std::string &scene, const std::string &name, ScriptFiles::Kind kind) {
    const bool tile = kind == ScriptFiles::Kind::Tile;
    const bool main = !tile && name == ScriptFiles::MAIN;

    // The registration of the script comes first, so the editor can hide it
    // without a gap in the middle of the file, see CodeEditor
    std::string text = std::string(ScriptFiles::REGISTRY_INCLUDE) + "\n\n"
                       "#include \"" + scene + "/" + name + ".h\"\n\n"
                       "namespace " + scene + " {\n";

    if (tile) {
        text += "    void " + name + "::Update(float dt) {\n"
                "        (void) dt;\n\n"
                "        // What happens on this tile, e.g.\n"
                "        // if (tile.HasPlayerEntered()) {\n"
                "        //     map.SwitchScene(\"combat\");\n"
                "        // }\n"
                "    }\n";
    } else if (main) {
        text += "    void " + name + "::Update(float dt) {\n"
                "        (void) dt;\n\n"
                "        // Your game loop, e.g. Gravity().Run(dt);\n"
                "    }\n";
    } else {
        text += "    void " + name + "::Run(float dt) {\n"
                "        (void) dt;\n\n"
                "        // What this script does\n"
                "    }\n";
    }

    text += "}\n\n" + std::string(ScriptFiles::REGISTRY_COMMENT) + "\n" +
            ScriptFiles::REGISTRY_MACRO + "\"" + scene + "\", \"" + name + "\", " + scene + "::" + name + ")\n";

    return text;
}

bool ScriptFiles::CanWrite() {
#ifdef PINGO_SCRIPT_SOURCE_DIR
    return true;
#else
    return false;
#endif
}

std::string ScriptFiles::Path(const std::string &scene, const std::string &name, const std::string &extension) {
#ifdef PINGO_SCRIPT_SOURCE_DIR
    return std::string(PINGO_SCRIPT_SOURCE_DIR) + "/" + scene + "/" + name + extension;
#else
    (void) scene;
    (void) name;
    (void) extension;

    return "";
#endif
}

bool ScriptFiles::IsValidName(const std::string &name) {
    if (name.empty() || name.size() > NAME_MAX_LENGTH) {
        return false;
    }

    // A class name starts with a letter and has no spaces
    if (!std::isalpha(static_cast<unsigned char>(name.front()))) {
        return false;
    }

    return std::all_of(name.begin(), name.end(), [](char letter) {
        return std::isalnum(static_cast<unsigned char>(letter)) || letter == '_';
    });
}

bool ScriptFiles::IsReservedName(const std::string &name, Kind kind) {
    if (kind == Kind::Tile && name == MAIN) {
        return true;
    }

    return name == "Script" || name == "TileScript" || name == "ScriptBase";
}

std::vector<std::string> ScriptFiles::Available(const std::string &scene) {
    std::vector<std::string> names;

    std::string folder = Path(scene, "", "");

    if (folder.empty()) {
        return names;
    }

    std::error_code ignored;

    for (const std::filesystem::directory_entry &entry:
         std::filesystem::directory_iterator(folder, ignored)) {
        if (entry.path().extension() == ".h") {
            names.push_back(entry.path().stem().string());
        }
    }

    std::sort(names.begin(), names.end());

    // Main belongs to the scene itself and stands in front of the others
    auto main = std::find(names.begin(), names.end(), MAIN);

    if (main != names.end()) {
        std::rotate(names.begin(), main, main + 1);
    }

    return names;
}

bool ScriptFiles::Create(const std::string &scene, const std::string &name, Kind kind) {
    if (!IsValidName(name) || IsReservedName(name, kind) || !CanWrite()) {
        return false;
    }

    std::vector<std::string> existing = Available(scene);

    if (std::find(existing.begin(), existing.end(), name) != existing.end()) {
        return false;
    }

    bool written = WriteFileAtomically(Path(scene, name, ".h"), HeaderTemplate(scene, name, kind));

    written = WriteFileAtomically(Path(scene, name, ".cpp"), SourceTemplate(scene, name, kind)) && written;

    if (!written) {
        TraceLog(LOG_WARNING, "SCRIPTS: [%s] konnte nicht angelegt werden", name.c_str());
    }

    return written;
}

// The name stands in the file several times: as the class, in the include and
// in the line that registers the script. Replacing it keeps the file working.
static std::string WithNewName(const std::string &text, const std::string &from, const std::string &to) {
    std::string result;
    std::size_t at = 0;

    while (at < text.size()) {
        std::size_t found = text.find(from, at);

        if (found == std::string::npos) {
            result += text.substr(at);
            break;
        }

        // Only whole words, so "MainMenu" does not become "GravityMenu"
        bool beforeOk = found == 0 || !(std::isalnum(static_cast<unsigned char>(text[found - 1])) ||
                                        text[found - 1] == '_');
        std::size_t after = found + from.size();
        bool afterOk = after >= text.size() || !(std::isalnum(static_cast<unsigned char>(text[after])) ||
                                                 text[after] == '_');

        result += text.substr(at, found - at);
        result += beforeOk && afterOk ? to : from;

        at = after;
    }

    return result;
}

std::string ScriptFiles::Read(const std::string &scene, const std::string &name, const std::string &extension) {
    std::ifstream file(Path(scene, name, extension));

    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

bool ScriptFiles::Write(const std::string &scene,
                        const std::string &name,
                        const std::string &extension,
                        const std::string &text) {
    if (!CanWrite()) {
        return false;
    }

    return WriteFileAtomically(Path(scene, name, extension), text);
}

static std::string ReadFile(const std::string &path) {
    std::ifstream file(path);
    std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    return text;
}

bool ScriptFiles::Rename(const std::string &scene, const std::string &from, const std::string &to) {
    // Main is reserved for the kind that has one, and renaming never makes one
    if (from == MAIN || to == MAIN || !IsValidName(to) || IsReservedName(to, Kind::Scene) || !CanWrite() ||
        from == to) {
        return false;
    }

    std::vector<std::string> existing = Available(scene);

    if (std::find(existing.begin(), existing.end(), to) != existing.end()) {
        return false;
    }

    if (std::find(existing.begin(), existing.end(), from) == existing.end()) {
        return false;
    }

    bool written = true;

    for (const std::string &extension: {".h", ".cpp"}) {
        std::string text = ReadFile(Path(scene, from, extension));

        written = WriteFileAtomically(Path(scene, to, extension), WithNewName(text, from, to)) && written;
    }

    if (written) {
        Remove(scene, from);
    }

    return written;
}

bool ScriptFiles::Remove(const std::string &scene, const std::string &name) {
    if (name == MAIN || !CanWrite()) {
        return false;
    }

    std::error_code ignored;

    std::filesystem::remove(Path(scene, name, ".h"), ignored);
    std::filesystem::remove(Path(scene, name, ".cpp"), ignored);

    std::vector<std::string> left = Available(scene);

    return std::find(left.begin(), left.end(), name) == left.end();
}
