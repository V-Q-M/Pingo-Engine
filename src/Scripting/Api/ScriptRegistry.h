#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ScriptBase.h"

// Every script says at startup that it exists, so the engine can create it by
// the name of its scene and its own name, without knowing the class.
//
// A script registers itself with the macro at the end of its .cpp file:
//
//     PINGO_SCRIPT("combat", "Main", combat::Main)
//
// The same macro registers a Script of a normal scene and a TileScript of a
// tileset scene. The editor writes that line into every new script, there is
// nothing to do by hand. A script the game does not know yet, e.g. a file
// that was just created, is greyed out in the menu until the next build.
class ScriptRegistry {
public:
    using Factory = std::function<std::unique_ptr<ScriptBase>()>;

    // Takes the factory of a script, called by PINGO_SCRIPT before main runs.
    // The same script twice replaces the older one.
    static void Add(const std::string &scene, const std::string &name, Factory factory);

    // Is this script built into the game?
    static bool Knows(const std::string &scene, const std::string &name);

    // A new instance, nullptr if the game does not know the script
    static std::unique_ptr<ScriptBase> Create(const std::string &scene, const std::string &name);

    // The scripts of a scene that are built in, alphabetical
    static std::vector<std::string> Names(const std::string &scene);

private:
    struct Entry {
        std::string scene;
        std::string name;

        Factory factory;
    };

    // A function instead of a variable: this way the list exists before the
    // first script registers itself, no matter in which order that happens
    static std::vector<Entry> &Entries();
};

// Helper of PINGO_SCRIPT: registers the script while the program starts
class ScriptRegistration {
public:
    ScriptRegistration(const std::string &scene, const std::string &name, ScriptRegistry::Factory factory) {
        ScriptRegistry::Add(scene, name, std::move(factory));
    }
};

// Makes a script known to the engine, see the class comment above
#define PINGO_SCRIPT(scene, name, type)                                            \
    namespace {                                                                    \
        const ScriptRegistration pingoScriptRegistration(                          \
            scene,                                                                 \
            name,                                                                  \
            [] { return std::unique_ptr<ScriptBase>(new type()); }                 \
        );                                                                         \
    }
