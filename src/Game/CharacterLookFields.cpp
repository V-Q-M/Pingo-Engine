#include "CharacterLookFields.h"

#include <algorithm>
#include <filesystem>

#include "ObjectEffects.h"

constexpr int SQUARE_DEFAULT_SIZE = 24;
constexpr int SQUARE_MIN_SIZE = 4;
constexpr int SQUARE_MAX_SIZE = 64;

constexpr int FRAME_DEFAULT_SIZE = 32;
constexpr int FRAME_MAX_SIZE = 256;
constexpr int FRAME_MAX_COUNT = 64;

// The option in front of the sprites for a colored square
constexpr const char *NO_SPRITE = "None";

// Rows are counted from 1, like in an image editor
constexpr int EFFECT_FIRST_ROW = 1;
constexpr int EFFECT_MAX_ROW = 32;

// Frames for a sprite sheet: the ones of a type that already uses it, otherwise
// square frames as high as the image, side by side
static CharacterLook GuessFrames(const std::string &texture, Assets &assets) {
    for (const std::string &file: CharacterDefinition::AvailableTypes()) {
        CharacterDefinition type = CharacterDefinition::Load(file);

        if (type.texture == texture) {
            return type.Look();
        }
    }

    Texture2D &image = assets.Texture().Get(texture);

    CharacterLook look;

    look.texture = texture;
    look.frameHeight = std::clamp(image.height, 1, FRAME_MAX_SIZE);
    look.frameWidth = look.frameHeight;
    look.frameCount = std::clamp(image.width / look.frameHeight, 1, FRAME_MAX_COUNT);

    return look;
}

static std::size_t ColorIndexOf(Color color) {
    const std::vector<Color> &colors = CharacterLookFields::Colors();

    for (std::size_t i = 0; i < colors.size(); i++) {
        if (colors[i].r == color.r && colors[i].g == color.g && colors[i].b == color.b) {
            return i;
        }
    }

    return 0;
}

const std::vector<Color> &CharacterLookFields::Colors() {
    static const std::vector<Color> colors{
        {255, 44, 34, 255},
        {255, 229, 26, 255},
        {61, 255, 20, 255},
        {0, 249, 255, 255},
        {41, 110, 219, 255},
        {153, 113, 84, 255},
        {128, 128, 128, 255},
        {255, 255, 255, 255}
    };

    return colors;
}

void CharacterLookFields::Add(FormWindow &form, const CharacterLook &look) {
    sprites = CharacterDefinition::AvailableSprites();

    std::size_t selected = 0;

    if (!look.IsSquare()) {
        auto found = std::find(sprites.begin(), sprites.end(), look.texture);

        // A sheet outside the folder still shows up, so editing does not lose it
        if (found == sprites.end()) {
            sprites.push_back(look.texture);
            found = sprites.end() - 1;
        }

        selected = static_cast<std::size_t>(found - sprites.begin()) + 1;
    }

    // The sheets are listed without folder and extension
    std::vector<std::string> options = {NO_SPRITE};

    for (const std::string &sprite: sprites) {
        options.push_back(std::filesystem::path(sprite).stem().string());
    }

    int squareSize = look.IsSquare() ? look.frameWidth : SQUARE_DEFAULT_SIZE;
    int frameWidth = look.IsSquare() ? FRAME_DEFAULT_SIZE : look.frameWidth;
    int frameHeight = look.IsSquare() ? FRAME_DEFAULT_SIZE : look.frameHeight;

    // Same order as Field
    first = form.AddDropdown("Sprite", options, selected);
    form.AddNumber("Size", squareSize, SQUARE_MIN_SIZE, SQUARE_MAX_SIZE);
    form.AddColor("Color", Colors(), ColorIndexOf(look.color));
    form.AddNumber("Frame width", frameWidth, 1, FRAME_MAX_SIZE);
    form.AddNumber("Frame height", frameHeight, 1, FRAME_MAX_SIZE);
    form.AddNumber("Frames", look.frameCount, 1, FRAME_MAX_COUNT);

    // Every effect: a checkbox and, below it, the row it animates
    firstEffect = form.FieldCount();

    for (const ObjectEffect &effect: ObjectEffects::All()) {
        auto row = look.effectRows.find(effect.id);
        bool shows = row != look.effectRows.end();

        form.AddToggle(effect.label, shows);
        form.AddNumber(effect.label + " row", shows ? row->second : EFFECT_FIRST_ROW, EFFECT_FIRST_ROW, EFFECT_MAX_ROW);
    }

    shownSprite = selected;

    ShowFieldsFor(form);
}

void CharacterLookFields::Follow(FormWindow &form, Assets &assets) {
    std::size_t sprite = form.Choice(Index(Field::Sprite));

    // A checked effect shows its row right away, so this runs every frame
    if (sprite == shownSprite) {
        ShowFieldsFor(form);
        return;
    }

    shownSprite = sprite;

    if (sprite > 0) {
        CharacterLook guess = GuessFrames(sprites[sprite - 1], assets);

        form.SetNumber(Index(Field::FrameWidth), guess.frameWidth);
        form.SetNumber(Index(Field::FrameHeight), guess.frameHeight);
        form.SetNumber(Index(Field::Frames), guess.frameCount);
    }

    ShowFieldsFor(form);
}

CharacterLook CharacterLookFields::Read(const FormWindow &form) const {
    CharacterLook look;

    std::size_t sprite = form.Choice(Index(Field::Sprite));

    if (sprite == 0) {
        look.frameWidth = form.Number(Index(Field::Size));
        look.frameHeight = look.frameWidth;
        look.color = form.ColorValue(Index(Field::Color));

        return look;
    }

    look.texture = sprites.at(sprite - 1);
    look.frameWidth = form.Number(Index(Field::FrameWidth));
    look.frameHeight = form.Number(Index(Field::FrameHeight));
    look.frameCount = form.Number(Index(Field::Frames));

    const std::vector<ObjectEffect> &effects = ObjectEffects::All();

    for (std::size_t i = 0; i < effects.size(); i++) {
        if (form.IsChecked(EffectIndex(i))) {
            look.effectRows[effects[i].id] = form.Number(EffectIndex(i) + 1);
        }
    }

    return look;
}

std::size_t CharacterLookFields::Index(Field field) const {
    return first + static_cast<std::size_t>(field);
}

std::size_t CharacterLookFields::EffectIndex(std::size_t effect) const {
    return firstEffect + effect * 2;
}

void CharacterLookFields::ShowFieldsFor(FormWindow &form) const {
    bool square = form.Choice(Index(Field::Sprite)) == 0;

    form.SetFieldVisible(Index(Field::Size), square);
    form.SetFieldVisible(Index(Field::Color), square);

    form.SetFieldVisible(Index(Field::FrameWidth), !square);
    form.SetFieldVisible(Index(Field::FrameHeight), !square);
    form.SetFieldVisible(Index(Field::Frames), !square);

    // Only sheets have rows, and only a checked effect asks for one
    for (std::size_t i = 0; i < ObjectEffects::All().size(); i++) {
        form.SetFieldVisible(EffectIndex(i), !square);
        form.SetFieldVisible(EffectIndex(i) + 1, !square && form.IsChecked(EffectIndex(i)));
    }
}
