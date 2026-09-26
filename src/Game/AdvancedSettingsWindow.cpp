#include "AdvancedSettingsWindow.h"

#include <cstddef>
#include <string>

// Width and height in pixels, 0 means no shadow
constexpr int BODY_MAX_SIZE = 256;

// Offsets in both directions
constexpr int BODY_MAX_OFFSET = 128;

void AdvancedSettingsWindow::Open(const CharacterBody &body, int viewWidth, int viewHeight, const FontRenderer &font) {
    form.Clear();
    form.SetTitle("Advanced settings");
    form.SetConfirmLabel("Done");

    // Same order as Field: width, height, X and Y of each box
    auto addBox = [&](const std::string &name, const CharacterBox &box) {
        form.AddNumber(name + " width", box.width, 0, BODY_MAX_SIZE);
        form.AddNumber(name + " height", box.height, 0, BODY_MAX_SIZE);
        form.AddNumber(name + " X", box.offsetX, -BODY_MAX_OFFSET, BODY_MAX_OFFSET);
        form.AddNumber(name + " Y", box.offsetY, -BODY_MAX_OFFSET, BODY_MAX_OFFSET);
    };

    addBox("Shadow", body.shadow);
    addBox("Hitbox", body.hitbox);

    // Last, so the boxes keep the field order of Field
    form.AddNumber("Shadow bounce", body.shadowBounce, 0, BODY_MAX_SIZE);

    form.Open(viewWidth, viewHeight, font);
}

bool AdvancedSettingsWindow::IsOpen() const {
    return form.IsOpen();
}

bool AdvancedSettingsWindow::IsHovering() const {
    return form.IsHovering();
}

AdvancedSettingsWindow::Result AdvancedSettingsWindow::Update(Vector2 mouse, bool clicked, const FontRenderer &font) {
    FormWindow::Result result = form.Update(mouse, clicked, font);

    if (result == FormWindow::Result::Confirmed) {
        form.Close();
        return Result::Done;
    }

    if (result == FormWindow::Result::Cancelled) {
        return Result::Cancelled;
    }

    return Result::None;
}

CharacterBody AdvancedSettingsWindow::Read() const {
    CharacterBody body;

    body.shadow = ReadBox(Field::Shadow);
    body.hitbox = ReadBox(Field::Hitbox);
    body.shadowBounce = form.Number(static_cast<std::size_t>(Field::Bounce));

    return body;
}

void AdvancedSettingsWindow::Close() {
    form.Close();
}

void AdvancedSettingsWindow::Draw(const FontRenderer &font) const {
    form.Draw(font);
}

CharacterBox AdvancedSettingsWindow::ReadBox(Field first) const {
    std::size_t index = static_cast<std::size_t>(first);

    return {form.Number(index), form.Number(index + 1), form.Number(index + 2), form.Number(index + 3)};
}
