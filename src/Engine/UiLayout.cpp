#include "UiLayout.h"

#include <algorithm>
#include <cmath>
#include <fstream>

#include <nlohmann/json.hpp>

#include "AssetFile.h"
#include "FontRenderer.h"

// ordered_json keeps the order of the keys, so the saved file stays as
// readable as it was written
using json = nlohmann::ordered_json;

bool UiElement::operator==(const UiElement &other) const {
    return type == other.type &&
           text == other.text &&
           position.x == other.position.x &&
           position.y == other.position.y &&
           scale == other.scale &&
           action == other.action &&
           target == other.target &&
           locked == other.locked &&
           variant == other.variant;
}

bool UiLayout::Load(const std::string &filename, std::vector<UiElement> &elements) {
    elements.clear();

    std::ifstream file("assets/" + filename);

    if (!file) {
        TraceLog(LOG_WARNING, "UI: [%s] Datei nicht gefunden", filename.c_str());
        return true;
    }

    try {
        json j = json::parse(file);

        for (const json &item: j.at("elements")) {
            std::string type = item.value("type", std::string("label"));

            if (type != "label" && type != "button") {
                TraceLog(LOG_WARNING, "UI: [%s] Typ \"%s\" unbekannt, uebersprungen", filename.c_str(), type.c_str());
                continue;
            }

            UiElement element;

            element.type = type == "button" ? UiElement::Type::Button : UiElement::Type::Label;
            element.text = item.value("text", std::string());
            element.position = {item.value("x", 0.0f), item.value("y", 0.0f)};
            element.scale = std::max(1, item.value("scale", DEFAULT_SCALE));
            element.action = item.value("action", std::string());
            element.target = item.value("target", std::string());
            element.locked = item.value("locked", false);

            if (item.contains("color")) {
                std::string color = item.value("color", std::string());

                element.variant = FontVariant::FromName(color);

                if (element.variant < 0) {
                    TraceLog(LOG_WARNING, "UI: [%s] Farbe \"%s\" unbekannt", filename.c_str(), color.c_str());
                }
            }

            elements.push_back(element);
        }
    } catch (const json::exception &error) {
        TraceLog(LOG_WARNING, "UI: [%s] %s", filename.c_str(), error.what());
        elements.clear();
        return false;
    }

    return true;
}

bool UiLayout::Save(const std::string &filename, const std::vector<UiElement> &elements) {
    json list = json::array();

    for (const UiElement &element: elements) {
        json item;

        item["type"] = element.type == UiElement::Type::Button ? "button" : "label";
        item["text"] = element.text;

        // Pixel art stands on whole pixels
        item["x"] = std::lround(element.position.x);
        item["y"] = std::lround(element.position.y);

        // Default values do not need to be in the file
        if (element.scale != DEFAULT_SCALE) {
            item["scale"] = element.scale;
        }

        if (!element.action.empty()) {
            item["action"] = element.action;
        }

        if (!element.target.empty()) {
            item["target"] = element.target;
        }

        if (element.locked) {
            item["locked"] = true;
        }

        if (element.variant >= 0) {
            item["color"] = FontVariant::Name(element.variant);
        }

        list.push_back(item);
    }

    json j = json::object();

    // Take over the remaining entries of the file, only the elements are new
    std::ifstream existing("assets/" + filename);

    if (existing) {
        try {
            json previous = json::parse(existing);

            if (previous.is_object()) {
                j = previous;
            }
        } catch (const json::exception &) {
            // Load does not even read a broken file
        }
    }

    j["elements"] = list;

    bool saved = SaveAsset(filename, j.dump(2) + "\n");

    if (saved) {
        TraceLog(LOG_INFO, "UI: [%s] gespeichert", filename.c_str());
    } else {
        TraceLog(LOG_WARNING, "UI: [%s] konnte nicht gespeichert werden", filename.c_str());
    }

    return saved;
}
