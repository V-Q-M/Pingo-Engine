#include "Names.h"

std::string TrimSpaces(std::string text) {
    text.erase(0, text.find_first_not_of(' '));
    text.erase(text.find_last_not_of(' ') + 1);

    return text;
}

std::string ToLower(std::string text) {
    for (char &character: text) {
        if (character >= 'A' && character <= 'Z') {
            character = static_cast<char>(character - 'A' + 'a');
        }
    }

    return text;
}

std::string IdFromName(const std::string &name, const std::string &fallback) {
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

    return id.empty() ? fallback : id;
}
