#include "ObjectEditor.h"

#include <cmath>
#include <utility>

#include "Renderer.h"

// Steps when moving, larger with Shift
constexpr float OBJECT_STEP = 1.0f;
constexpr float OBJECT_STEP_FAST = 10.0f;

constexpr int OBJECT_VARIANT_TEXT = FontVariant::White;
constexpr int OBJECT_VARIANT_ARROW = FontVariant::Yellow;

// Dark box behind the labels so they are readable everywhere
constexpr Color OBJECT_LABEL_BACKGROUND{24, 20, 37, 170};

// Cross where a character from the context menu is created. The menus open
// slightly offset so they do not cover the cross.
constexpr Color OBJECT_ADD_MARKER{255, 229, 26, 255};
constexpr float OBJECT_MENU_OFFSET = 5.0f;

// Distance of the arrows to the object's click area
constexpr float OBJECT_ARROW_GAP = 2.0f;

constexpr float OBJECT_LINE_GAP = 3.0f;

// The key help sits above the engine's mode label at the bottom left
constexpr float OBJECT_HELP_BOTTOM_OFFSET = 28.0f;

static bool KeyStep(int key) {
    return IsKeyPressed(key) || IsKeyPressedRepeat(key);
}

// Discard typed characters so they do not take effect unintentionally later
static void DiscardTypedCharacters() {
    while (GetCharPressed() != 0) {
    }
}

static void DrawLabel(const FontRenderer &font, const std::string &text, float centerX, float top, int variant) {
    int width = font.Measure(text, TextSpacing::Narrow);

    Vector2 topLeft = Renderer::SnapToPixel({centerX - width / 2.0f, top});

    DrawRectangle(
        static_cast<int>(topLeft.x) - 1,
        static_cast<int>(topLeft.y) - 1,
        width + 2,
        font.LetterHeight() + 2,
        OBJECT_LABEL_BACKGROUND
    );

    font.Draw(text, topLeft, variant, TextSpacing::Narrow);
}

ObjectEditor::ObjectEditor() {
    addMenu.SetTitle("Add");
}

bool ObjectEditor::IsMoveKeyDown() {
    return IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_UP) || IsKeyDown(KEY_DOWN);
}

void ObjectEditor::Select(EditableObject *instance) {
    selected = instance;
    dragging = false;
}

EditableObject *ObjectEditor::Selected() const {
    return selected;
}

bool ObjectEditor::IsDragging() const {
    return dragging;
}

void ObjectEditor::SetTypeNames(std::vector<std::string> names) {
    typeNames = std::move(names);

    RebuildAddMenu();
}

void ObjectEditor::SetAddExtra(std::string label, int variant) {
    addExtraLabel = std::move(label);
    addExtraVariant = variant;

    RebuildAddMenu();
}

void ObjectEditor::RebuildAddMenu() {
    std::vector<std::string> items = typeNames;

    if (!addExtraLabel.empty()) {
        items.push_back(addExtraLabel);
    }

    addMenu.SetItems(std::move(items));

    if (!addExtraLabel.empty()) {
        addMenu.SetItemVariant(typeNames.size(), addExtraVariant);
    }
}

// Sets the items and icons of a menu
static void SetMenuItems(ContextMenu &menu, std::vector<std::string> labels, const std::vector<char> &icons) {
    menu.SetItems(std::move(labels));

    for (std::size_t i = 0; i < icons.size(); i++) {
        menu.SetItemIcon(i, icons[i]);
    }
}

void ObjectEditor::SetTypeActions(std::vector<std::string> labels, const std::vector<char> &icons) {
    SetMenuItems(typeMenu, std::move(labels), icons);
}

void ObjectEditor::SetObjectActions(std::vector<std::string> labels, const std::vector<char> &icons) {
    SetMenuItems(objectMenu, std::move(labels), icons);
}

void ObjectEditor::SetObjectTitle(std::function<std::string(const EditableObject &)> title) {
    objectTitle = std::move(title);
}

void ObjectEditor::SetObjectMenuFilter(std::function<bool(const EditableObject &)> filter) {
    objectMenuFilter = std::move(filter);
}

void ObjectEditor::SetTopArrowClearance(float clearance) {
    topArrowClearance = clearance;
}

EditableObject *ObjectEditor::FindAt(Vector2 point, const std::vector<EditableObject *> &objects) {
    EditableObject *found = nullptr;

    for (EditableObject *object: objects) {
        if (!CheckCollisionPointRec(point, object->EditBounds())) {
            continue;
        }

        if (found == nullptr || object->EditDepth() >= found->EditDepth()) {
            found = object;
        }
    }

    return found;
}

bool ObjectEditor::IsMenuOpen() const {
    return addMenu.IsOpen() || typeMenu.IsOpen() || objectMenu.IsOpen();
}

CursorState ObjectEditor::Cursor(const std::vector<EditableObject *> &instances, Vector2 mouseWorld) const {
    if (dragging) {
        return CursorState::Grab;
    }

    // With a menu open only its items count, the objects below it rest
    if (IsMenuOpen()) {
        bool hovering = addMenu.IsHovering() || typeMenu.IsHovering() || objectMenu.IsHovering();

        return hovering ? CursorState::Hover : CursorState::Idle;
    }

    return FindAt(mouseWorld, instances) != nullptr ? CursorState::Hover : CursorState::Idle;
}

void ObjectEditor::CloseMenu() {
    addMenu.Close();
    typeMenu.Close();
    objectMenu.Close();

    menuTarget = nullptr;
}

const ContextMenu &ObjectEditor::AddMenu() const {
    return addMenu;
}

const ContextMenu &ObjectEditor::ObjectMenu() const {
    return objectMenu;
}

const ContextMenu &ObjectEditor::TypeMenu() const {
    return typeMenu;
}

bool ObjectEditor::MoveSelected(Vector2 delta) {
    if (selected == nullptr) {
        return false;
    }

    Vector2 position = selected->EditPosition();

    selected->SetEditPosition(Renderer::SnapToPixel({position.x + delta.x, position.y + delta.y}));

    return true;
}

void ObjectEditor::HandleRightClick(const std::vector<EditableObject *> &instances,
                                    const Input &input,
                                    const FontRenderer &font) {
    Vector2 menuPosition = {
        input.mouseViewport.x + OBJECT_MENU_OFFSET,
        input.mouseViewport.y + OBJECT_MENU_OFFSET
    };

    // In the open add menu a right click on a type opens its menu.
    // Nothing happens on the title or the extra item.
    if (addMenu.IsOpen() && CheckCollisionPointRec(input.mouseViewport, addMenu.Bounds(font))) {
        int item = addMenu.ItemAt(input.mouseViewport, font);

        typeMenu.Close();

        if (item != ContextMenu::NOTHING && static_cast<std::size_t>(item) < typeNames.size() && typeMenu.HasItems()) {
            typeMenuIndex = static_cast<std::size_t>(item);

            typeMenu.SetTitle(typeNames[typeMenuIndex]);
            typeMenu.Open(menuPosition, input.viewWidth, input.viewHeight, font);
        }

        return;
    }

    if (typeMenu.IsOpen() && CheckCollisionPointRec(input.mouseViewport, typeMenu.Bounds(font))) {
        return;
    }

    CloseMenu();

    EditableObject *hit = FindAt(input.mouseWorld, instances);

    if (hit != nullptr) {
        // The object gets selected, so you can see what the menu refers to
        selected = hit;

        if (objectMenu.HasItems() && (!objectMenuFilter || objectMenuFilter(*hit))) {
            menuTarget = hit;

            objectMenu.SetTitle(objectTitle ? objectTitle(*hit) : "");
            objectMenu.Open(menuPosition, input.viewWidth, input.viewHeight, font);
        }
    } else if (addMenu.HasItems()) {
        addPosition = Renderer::SnapToPixel(input.mouseWorld);
        addMenu.Open(menuPosition, input.viewWidth, input.viewHeight, font);
    }
}

ObjectEditor::Changes ObjectEditor::Update(const std::vector<EditableObject *> &instances,
                                           const Input &input,
                                           const FontRenderer &font) {
    Changes changes;

    if (!input.leftDown) {
        dragging = false;
    }

    // A right click opens a menu, also again at a different position
    if (input.rightClicked) {
        dragging = false;

        HandleRightClick(instances, input, font);

        DiscardTypedCharacters();

        return changes;
    }

    // As long as a menu is open, mouse and keyboard belong to it. A click next
    // to it closes it without selecting a character behind it.
    if (typeMenu.IsOpen()) {
        // The menu of a type lies above the add menu and only closes itself
        int action = typeMenu.Update(input.mouseViewport, input.leftClicked, font);

        if (action != ContextMenu::NOTHING) {
            changes.typeAction = action;
            changes.typeIndex = typeMenuIndex;
        }

        DiscardTypedCharacters();

        return changes;
    }

    if (objectMenu.IsOpen()) {
        int action = objectMenu.Update(input.mouseViewport, input.leftClicked, font);

        if (action != ContextMenu::NOTHING) {
            changes.objectAction = action;
            changes.actionTarget = menuTarget;
        }

        if (!objectMenu.IsOpen()) {
            menuTarget = nullptr;
        }

        DiscardTypedCharacters();

        return changes;
    }

    if (addMenu.IsOpen()) {
        int type = addMenu.Update(input.mouseViewport, input.leftClicked, font);

        if (type != ContextMenu::NOTHING) {
            changes.addType = type;
            changes.addPosition = addPosition;
        }

        DiscardTypedCharacters();

        return changes;
    }

    // A click into empty space clears the selection, one on a character grabs it
    if (input.leftClicked) {
        selected = FindAt(input.mouseWorld, instances);
        dragging = selected != nullptr;

        if (dragging) {
            dragOffset = {
                selected->EditPosition().x - input.mouseWorld.x,
                selected->EditPosition().y - input.mouseWorld.y
            };
        }
    }

    // Without mouse movement the position stays the same, so a simple click
    // moves nothing
    if (dragging && selected != nullptr) {
        Vector2 target = Renderer::SnapToPixel({
            input.mouseWorld.x + dragOffset.x,
            input.mouseWorld.y + dragOffset.y
        });

        Vector2 current = selected->EditPosition();

        if (target.x != current.x || target.y != current.y) {
            selected->SetEditPosition(target);
            changes.moved = true;
        }
    }

    if (selected != nullptr) {
        float step = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)
                         ? OBJECT_STEP_FAST
                         : OBJECT_STEP;

        Vector2 delta{0.0f, 0.0f};

        if (KeyStep(KEY_LEFT)) {
            delta.x -= step;
        }

        if (KeyStep(KEY_RIGHT)) {
            delta.x += step;
        }

        if (KeyStep(KEY_UP)) {
            delta.y -= step;
        }

        if (KeyStep(KEY_DOWN)) {
            delta.y += step;
        }

        if (delta.x != 0.0f || delta.y != 0.0f) {
            changes.moved = MoveSelected(delta);
        }

        changes.deleteRequested = IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_BACKSPACE);
    }

    // GetCharPressed returns characters instead of keys and therefore works with
    // every keyboard layout. Important for Z and Y: on German keyboards they are
    // swapped, but raylib's key codes follow the US layout.
    for (int character = GetCharPressed(); character != 0; character = GetCharPressed()) {
        if (character == 'z' || character == 'Z') {
            changes.undoRequested = true;
        }

        if (character == 'y' || character == 'Y') {
            changes.redoRequested = true;
        }
    }

    return changes;
}

void ObjectEditor::Draw(const FontRenderer &font, const std::string &caption) const {
    if (addMenu.IsOpen()) {
        int x = static_cast<int>(addPosition.x);
        int y = static_cast<int>(addPosition.y);

        DrawRectangle(x - 3, y, 7, 1, OBJECT_ADD_MARKER);
        DrawRectangle(x, y - 3, 1, 7, OBJECT_ADD_MARKER);
    }

    if (selected == nullptr) {
        return;
    }

    Rectangle box = selected->EditBounds();

    float glyphWidth = static_cast<float>(font.LetterWidth());
    float glyphHeight = static_cast<float>(font.LetterHeight());

    float centerX = box.x + box.width / 2.0f;
    float centerY = box.y + box.height / 2.0f;

    auto arrow = [&](char glyph, float x, float y) {
        font.Draw(std::string(1, glyph), Renderer::SnapToPixel({x, y}), OBJECT_VARIANT_ARROW);
    };

    arrow(FontRenderer::ARROW_LEFT, box.x - OBJECT_ARROW_GAP - glyphWidth, centerY - glyphHeight / 2.0f);
    arrow(FontRenderer::ARROW_RIGHT, box.x + box.width + OBJECT_ARROW_GAP, centerY - glyphHeight / 2.0f);
    arrow(FontRenderer::ARROW_UP, centerX - glyphWidth / 2.0f,
          box.y - topArrowClearance - OBJECT_ARROW_GAP - glyphHeight);
    arrow(FontRenderer::ARROW_DOWN, centerX - glyphWidth / 2.0f, box.y + box.height + OBJECT_ARROW_GAP);

    Vector2 position = selected->EditPosition();

    std::string coordinates = "X " + std::to_string(std::lround(position.x)) +
                              " Y " + std::to_string(std::lround(position.y));

    float top = box.y + box.height + OBJECT_ARROW_GAP + glyphHeight + OBJECT_LINE_GAP;

    DrawLabel(font, coordinates, centerX, top, OBJECT_VARIANT_TEXT);

    if (!caption.empty()) {
        DrawLabel(font, caption, centerX, top + glyphHeight + OBJECT_LINE_GAP, OBJECT_VARIANT_TEXT);
    }
}

void ObjectEditor::DrawHelp(const FontRenderer &font, int viewHeight, const std::string &extra) const {
    std::string mouse = "Klick wählen, ziehen  Rechtsklick Menü";
    std::string keys = "Pfeile bewegen  Shift x10";

    float keysTop = static_cast<float>(viewHeight) - OBJECT_HELP_BOTTOM_OFFSET;
    float mouseTop = keysTop - font.LetterHeight() - 4.0f;

    font.Draw(mouse, {8.0f, mouseTop}, OBJECT_VARIANT_TEXT, TextSpacing::Narrow);
    font.Draw(keys, {8.0f, keysTop}, OBJECT_VARIANT_TEXT, TextSpacing::Narrow);

    if (!extra.empty()) {
        font.Draw(extra, {8.0f, mouseTop - font.LetterHeight() - 4.0f}, OBJECT_VARIANT_TEXT, TextSpacing::Narrow);
    }
}

void ObjectEditor::DrawMenu(const FontRenderer &font) const {
    addMenu.Draw(font);
    objectMenu.Draw(font);

    // Lies above the add menu it was opened from
    typeMenu.Draw(font);
}
