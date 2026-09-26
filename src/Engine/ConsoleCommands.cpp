#include "ConsoleCommands.h"

#include <sstream>
#include <utility>

#include "Names.h"

void ConsoleCommands::Add(ConsoleCommand command) {
    command.name = ToLower(command.name);

    for (ConsoleCommand &existing: commands) {
        if (existing.name == command.name) {
            existing = std::move(command);
            return;
        }
    }

    commands.push_back(std::move(command));
}

void ConsoleCommands::Clear() {
    commands.clear();
}

const ConsoleCommand *ConsoleCommands::Find(const std::string &name) const {
    std::string wanted = ToLower(name);

    for (const ConsoleCommand &command: commands) {
        if (command.name == wanted) {
            return &command;
        }
    }

    return nullptr;
}

const std::vector<ConsoleCommand> &ConsoleCommands::All() const {
    return commands;
}

std::string ConsoleCommands::Run(const std::string &line) const {
    std::vector<std::string> words;
    std::istringstream stream(line);

    for (std::string word; stream >> word;) {
        words.push_back(word);
    }

    if (words.empty()) {
        return "";
    }

    const ConsoleCommand *command = Find(words.front());

    if (command == nullptr || !command->run) {
        return "Unknown command " + words.front() + ", try help";
    }

    std::vector<std::string> arguments(words.begin() + 1, words.end());

    // A wrong number of words never reaches the command, so it can rely on it
    if (arguments.size() != command->arguments) {
        return Usage(*command);
    }

    return command->run(arguments);
}

std::string ConsoleCommands::Usage(const ConsoleCommand &command) {
    return command.usage.empty() ? command.name : command.name + " " + command.usage;
}
