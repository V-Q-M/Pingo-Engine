#pragma once

#include <string>
#include <vector>

// The script files of the game, one folder per scene: scripts/<scene>/.
// Every script is a header and a source file, e.g. scripts/combat/Main.h and
// scripts/combat/Main.cpp, and lives in the namespace of its folder.
//
// Only the Development build writes here, and only into the sources: scripts
// are code, they are compiled with the game and not copied like an asset. A
// new or renamed script therefore only reaches the game with the next build,
// see ScriptRegistry.
class ScriptFiles {
public:
    // What a new script is written as
    enum class Kind {
        // A Script of a normal scene: Main, or a tool that Main calls
        Scene,

        // A TileScript of a tileset scene, which runs on its tiles by itself
        Tile
    };

    // Folder of all scripts, relative to the project
    static constexpr const char *FOLDER = "scripts";

    // The script every normal scene has and that cannot be deleted
    static constexpr const char *MAIN = "Main";

    // Longest name of a script
    static constexpr std::size_t NAME_MAX_LENGTH = 16;

    // The lines that are only there for the engine: they stand in every
    // script, say nothing about what it does, and the editor hides exactly
    // these, see CodeEditor.
    static constexpr const char *HEADER_GUARD = "#pragma once";
    static constexpr const char *SCRIPT_INCLUDE = "#include \"Script.h\"";
    static constexpr const char *TILE_SCRIPT_INCLUDE = "#include \"TileScript.h\"";
    static constexpr const char *REGISTRY_INCLUDE = "#include \"ScriptRegistry.h\"";
    static constexpr const char *REGISTRY_COMMENT = "// Makes the script known to the engine, see ScriptRegistry";
    static constexpr const char *REGISTRY_MACRO = "PINGO_SCRIPT(";

    // Are the sources reachable? Only then can scripts be written, so only in
    // the Development build.
    static bool CanWrite();

    // The scripts of a scene by their names, alphabetical with Main first.
    // Empty if the scene has no folder yet.
    static std::vector<std::string> Available(const std::string &scene);

    // Writes header and source of a new script from the template of its
    // kind. false if the name is taken or the files cannot be written.
    static bool Create(const std::string &scene, const std::string &name, Kind kind = Kind::Scene);

    // Renames both files and the class inside them. false if the new name is
    // taken or nothing could be written.
    static bool Rename(const std::string &scene, const std::string &from, const std::string &to);

    // Deletes header and source. Main cannot be deleted.
    static bool Remove(const std::string &scene, const std::string &name);

    // Is the name free and usable as a class name?
    static bool IsValidName(const std::string &name);

    // Names a new script of this kind cannot have although they are valid:
    // the base classes of the API, whose template would derive from itself,
    // and Main in a tileset scene, where it would stay undeletable
    static bool IsReservedName(const std::string &name, Kind kind);

    // The text of one file of a script, empty if it cannot be read
    static std::string Read(const std::string &scene, const std::string &name, const std::string &extension);

    // Writes it back, e.g. after editing it in the game
    static bool Write(const std::string &scene,
                      const std::string &name,
                      const std::string &extension,
                      const std::string &text);

    // Path of a file of the script, e.g. "<sources>/scripts/combat/Main.cpp".
    // Empty if the sources are not reachable.
    static std::string Path(const std::string &scene, const std::string &name, const std::string &extension);
};
