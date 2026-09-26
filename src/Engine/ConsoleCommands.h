#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

// A command of the console, e.g. "damage (id) (amount)".
struct ConsoleCommand {
    // What is typed first, e.g. "damage". Case does not matter when typing.
    std::string name;

    // The arguments as help shows them, e.g. "(id) (amount)". Empty for a
    // command without arguments.
    std::string usage;

    // How many words follow the name. With a different number the console
    // answers with the usage instead of running the command.
    std::size_t arguments = 0;

    // Does the work and returns what the console prints, e.g. "Pingi 70/100".
    // Gets the words after the name, exactly as many as arguments says.
    std::function<std::string(const std::vector<std::string> &arguments)> run;
};

// All commands the console knows right now.
//
// The engine clears the list whenever a scene starts, adds its own commands
// (help, clear) and then asks the scene for its commands, see
// Scene::AddCommands. That way every scene only offers what makes sense in
// it, and nothing points into a scene that no longer exists.
//
// A new command takes three steps:
//
//   1. Write what it does as a method of the scene, taking the words and
//      returning the answer:
//
//          std::string FieldScene::Heal(const std::vector<std::string> &arguments) {
//              Character *target = CharacterWithId(ParseNumber(arguments[0]));
//              ...
//              return target->Name() + " healed";
//          }
//
//   2. Add it in the AddCommands of the scene, next to the others:
//
//          commands.Add({"heal", "(id) (amount)", 2, [this](const auto &arguments) {
//              return Heal(arguments);
//          }});
//
//   3. Done: help lists it, and typing "heal [3] 20" runs it.
//
// Commands for every scene belong into Engine::AddEngineCommands instead.
// Names are compared in lowercase, arguments arrive as they were typed: a
// command decides itself whether their case matters, see ToLower in Names.h.
class ConsoleCommands {
public:
    // Replaces a command with the same name
    void Add(ConsoleCommand command);

    void Clear();

    // nullptr if there is no command with this name, case does not matter
    const ConsoleCommand *Find(const std::string &name) const;

    // In the order they were added
    const std::vector<ConsoleCommand> &All() const;

    // Splits the line into words and runs the command of the first one.
    // Returns what the console prints: the answer of the command, its usage
    // for a wrong number of arguments, or a hint for an unknown command.
    std::string Run(const std::string &line) const;

    // "name usage" of one command, e.g. "damage (id) (amount)"
    static std::string Usage(const ConsoleCommand &command);

private:
    std::vector<ConsoleCommand> commands;
};
