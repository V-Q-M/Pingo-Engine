#include "ColorHex.h"

#include <cstdio>

std::string ColorToHex(Color color) {
    char text[8];
    std::snprintf(text, sizeof(text), "#%02X%02X%02X", color.r, color.g, color.b);
    return text;
}

Color ColorFromHex(const std::string &text) {
    unsigned int r = 255, g = 255, b = 255;

    if (text.size() != 7 || std::sscanf(text.c_str(), "#%02x%02x%02x", &r, &g, &b) != 3) {
        return WHITE;
    }

    return {static_cast<unsigned char>(r), static_cast<unsigned char>(g), static_cast<unsigned char>(b), 255};
}
