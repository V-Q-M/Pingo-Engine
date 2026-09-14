#include "MenuScene.h"

#include <algorithm>
#include <filesystem>
#include <utility>

#include "Themes.h"
#include "Engine/AssetFile.h"
#include "Engine/EditStatus.h"
#include "Engine/Engine.h"

// Longest text for new elements. At double size it just barely fits into
// the viewport.
constexpr std::size_t TEXT_MAX_LENGTH = 20;

// How far a duplicated element lands next to the original
constexpr float DUPLICATE_OFFSET = 10.0f;

// Like the add menu, the scene menu opens slightly offset
constexpr float MENU_OFFSET = 5.0f;

static SceneOptions MenuOptions(const std::string &id) {
    SceneOptions options;

    options.id = id;
    options.pauseMenu = false;
    options.movement = false;
    options.theme = DefaultTheme();

    return options;
}

static std::string Trimmed(std::string text) {
    text.erase(0, text.find_first_not_of(' '));
    text.erase(text.find_last_not_of(' ') + 1);

    return text;
}

MenuScene::MenuScene(Engine &engine, const std::string &id)
    : Scene(engine, MenuOptions(id)),
      layoutFile("scenes/" + id + ".json") {
}

void MenuScene::Enter() {
    // The font is only fixed from Enter on, and the size of the elements
    // depends on it
    std::vector<UiElement> elements;

    bool present = std::filesystem::exists("assets/" + layoutFile);

    // Without a file the menu stays empty, a broken file is not overwritten
    layoutWritable = UiLayout::Load(layoutFile, elements);

    // If the file only exists in the asset folder of the sources, it is not
    // overwritten with an empty menu
    if (!present && AssetExists(layoutFile)) {
        layoutWritable = false;
    }

    canvas.SetElements(elements, engine.GetFont());

    if (!engine.GetConfig().IsDevelopment()) {
        return;
    }

    editor.SetTypeNames({"Text", "Button"});
    editor.SetObjectActions(
        {"Edit", "Duplicate", "Delete"},
        {FontRenderer::ICON_EDIT, FontRenderer::ICON_DUPLICATE, FontRenderer::ICON_DELETE}
    );

    // No health bar sits above texts, so the top arrow may come closer
    editor.SetTopArrowClearance(0.0f);

    editor.SetObjectTitle([this](const EditableObject &object) {
        return CaptionFor(canvas.IndexOf(&object));
    });

    // Play, Settings and Quit have no menu and can only be moved
    editor.SetObjectMenuFilter([this](const EditableObject &object) {
        return IsEditable(canvas.IndexOf(&object));
    });

    sceneMenu.SetTitle("Scene");

    // Starting point of the history
    history.Reset(canvas.Elements());
}

void MenuScene::Exit() {
    if (pendingSave) {
        SaveLayout();
    }
}

bool MenuScene::IsEditable(int index) const {
    return index >= 0 &&
           static_cast<std::size_t>(index) < canvas.Count() &&
           !canvas.At(static_cast<std::size_t>(index)).Element().locked;
}

std::vector<std::string> MenuScene::SceneNames() const {
    std::vector<std::string> names;

    for (const SceneEntry &scene: engine.GetScenes().Entries()) {
        names.push_back(scene.name);
    }

    return names;
}

int MenuScene::DefaultVariant(const UiElement &element) const {
    const Theme &theme = engine.GetTheme();

    return element.type == UiElement::Type::Button ? theme.textVariant : theme.titleVariant;
}

std::string MenuScene::CaptionFor(int index) const {
    if (index < 0 || static_cast<std::size_t>(index) >= canvas.Count()) {
        return "";
    }

    const UiElement &element = canvas.At(static_cast<std::size_t>(index)).Element();

    if (element.type == UiElement::Type::Label) {
        return "Text";
    }

    if (element.action == "scene") {
        const SceneEntry *scene = engine.GetScenes().Find(element.target);

        return "Button " + (scene != nullptr ? scene->name : element.target);
    }

    if (element.action == "settings") {
        return "Button Settings";
    }

    if (element.action == "quit") {
        return "Button Quit";
    }

    return "Button";
}

void MenuScene::Activate(const UiElement &button) {
    if (button.type != UiElement::Type::Button) {
        return;
    }

    if (button.action == "scene") {
        engine.EnterScene(button.target);
    } else if (button.action == "settings") {
        engine.OpenSettings();
    } else if (button.action == "quit") {
        engine.Quit();
    } else {
        TraceLog(LOG_WARNING, "UI: Aktion \"%s\" unbekannt", button.action.c_str());
    }
}

void MenuScene::Update(float) {
    Renderer &renderer = engine.GetRenderer();

    hoveredButton = canvas.ButtonAt(renderer.MouseViewportPosition());

    if (hoveredButton >= 0) {
        engine.RequestCursor(CursorState::Hover);
    }

    if (hoveredButton >= 0 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Activate(canvas.At(static_cast<std::size_t>(hoveredButton)).Element());
    }
}

void MenuScene::UpdateDevelopment(float) {
    Renderer &renderer = engine.GetRenderer();
    const FontRenderer &font = engine.GetFont();

    // The elements lie on the screen layer, so the editor works in viewport
    // coordinates
    Vector2 mouse = renderer.MouseViewportPosition();
    bool leftClicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    if (textForm.IsOpen()) {
        if (textForm.Update(mouse, leftClicked, font) == FormWindow::Result::Confirmed) {
            ConfirmTextForm();
        }

        if (textForm.IsHovering()) {
            engine.RequestCursor(CursorState::Hover);
        }

        return;
    }

    if (editForm.IsOpen()) {
        if (editForm.Update(mouse, leftClicked, font) == FormWindow::Result::Confirmed) {
            ConfirmEditForm();
        }

        if (editForm.IsHovering()) {
            engine.RequestCursor(CursorState::Hover);
        }

        return;
    }

    if (confirmDialog.IsOpen()) {
        UpdateDeleteDialog(mouse, leftClicked);

        if (confirmDialog.IsHovering()) {
            engine.RequestCursor(CursorState::Hover);
        }

        return;
    }

    if (sceneMenu.IsOpen()) {
        int scene = sceneMenu.Update(mouse, leftClicked, font);

        if (sceneMenu.IsHovering()) {
            engine.RequestCursor(CursorState::Hover);
        }

        if (scene != ContextMenu::NOTHING) {
            ChooseButtonScene(static_cast<std::size_t>(scene));
        } else if (!sceneMenu.IsOpen()) {
            pendingButtonText.clear();
        }

        while (GetCharPressed() != 0) {
        }

        return;
    }

    // Shift + click on a button runs it instead of selecting it
    bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

    if (leftClicked && shift && !editor.IsMenuOpen()) {
        int button = canvas.ButtonAt(mouse);

        if (button >= 0) {
            Activate(canvas.At(static_cast<std::size_t>(button)).Element());
            return;
        }
    }

    ObjectEditor::Input input;
    input.mouseWorld = mouse;
    input.mouseViewport = mouse;
    input.leftClicked = leftClicked;
    input.leftDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    input.rightClicked = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
    input.viewWidth = renderer.GetWidth();
    input.viewHeight = renderer.GetHeight();

    ApplyEditorChanges(editor.Update(canvas.Editables(), input, font));

    engine.RequestCursor(editor.Cursor(canvas.Editables(), mouse));
}

void MenuScene::ApplyEditorChanges(const ObjectEditor::Changes &changes) {
    if (changes.moved) {
        pendingSave = true;
    }

    if (changes.undoRequested || changes.redoRequested) {
        // Store a pending move as its own step first
        if (pendingSave) {
            SaveLayout();
        }

        if (changes.undoRequested) {
            UndoLayout();
        } else {
            RedoLayout();
        }

        return;
    }

    int selected = canvas.IndexOf(editor.Selected());

    if (changes.deleteRequested && IsEditable(selected)) {
        AskDeleteElement(static_cast<std::size_t>(selected));
    }

    if (changes.addType >= 0) {
        OpenTextForm(static_cast<AddType>(changes.addType), changes.addPosition);
    }

    int target = canvas.IndexOf(changes.actionTarget);

    if (changes.objectAction >= 0 && IsEditable(target)) {
        switch (static_cast<ElementAction>(changes.objectAction)) {
            case ElementAction::Edit:
                OpenEditForm(static_cast<std::size_t>(target));
                break;

            case ElementAction::Duplicate:
                DuplicateElement(static_cast<std::size_t>(target));
                break;

            case ElementAction::Delete:
                AskDeleteElement(static_cast<std::size_t>(target));
                break;
        }
    }

    // The whole movement is one step in the history
    if (pendingSave && !ObjectEditor::IsMoveKeyDown() && !editor.IsDragging()) {
        SaveLayout();
    }
}

void MenuScene::OpenTextForm(AddType type, Vector2 position) {
    pendingAddType = type;
    addPosition = position;

    textForm.Clear();
    textForm.SetTitle(type == AddType::Text ? "New text" : "New button");

    // For a button the choice of the scene follows after the text
    textForm.SetConfirmLabel(type == AddType::Text ? "Create" : "Next");
    textForm.AddText("Text", "", TEXT_MAX_LENGTH);

    Renderer &renderer = engine.GetRenderer();

    textForm.Open(renderer.GetWidth(), renderer.GetHeight(), engine.GetFont());
}

bool MenuScene::ConfirmTextForm() {
    std::string text = Trimmed(textForm.Text(0));

    if (text.empty()) {
        textForm.SetError("Text missing");
        return false;
    }

    textForm.Close();

    if (pendingAddType == AddType::Text) {
        UiElement label;
        label.type = UiElement::Type::Label;
        label.text = text;
        label.position = addPosition;

        AddElement(label);

        return true;
    }

    pendingButtonText = text;

    Renderer &renderer = engine.GetRenderer();

    // The scenes may have changed since last time
    sceneMenu.SetItems(SceneNames());

    sceneMenu.Open(
        {addPosition.x + MENU_OFFSET, addPosition.y + MENU_OFFSET},
        renderer.GetWidth(),
        renderer.GetHeight(),
        engine.GetFont()
    );

    return true;
}

void MenuScene::ChooseButtonScene(std::size_t sceneIndex) {
    const std::vector<SceneEntry> &scenes = engine.GetScenes().Entries();

    if (sceneIndex >= scenes.size() || pendingButtonText.empty()) {
        return;
    }

    UiElement button;
    button.type = UiElement::Type::Button;
    button.text = pendingButtonText;
    button.position = addPosition;
    button.action = "scene";
    button.target = scenes[sceneIndex].id;

    pendingButtonText.clear();

    AddElement(button);
}

void MenuScene::OpenEditForm(std::size_t index) {
    if (!IsEditable(static_cast<int>(index))) {
        return;
    }

    editIndex = static_cast<int>(index);

    const UiElement &element = canvas.At(index).Element();
    const FontRenderer &font = engine.GetFont();

    editForm.Clear();
    editForm.SetTitle(element.type == UiElement::Type::Button ? "Edit button" : "Edit text");
    editForm.SetConfirmLabel("Save");

    // Same order as EditField
    editForm.AddText("Text", element.text, TEXT_MAX_LENGTH);

    std::vector<Color> swatches;
    int colors = std::min(FontVariant::Count, font.VariantCount());

    for (int variant = 0; variant < colors; variant++) {
        swatches.push_back(FontVariant::Swatch(variant));
    }

    int current = element.variant >= 0 ? element.variant : DefaultVariant(element);

    editForm.AddColor("Color", swatches, static_cast<std::size_t>(std::max(current, 0)));

    if (element.type == UiElement::Type::Button && element.action == "scene") {
        int selected = engine.GetScenes().IndexOf(element.target);

        editForm.AddChoice("Scene", SceneNames(), static_cast<std::size_t>(std::max(selected, 0)));
    }

    Renderer &renderer = engine.GetRenderer();

    editForm.Open(renderer.GetWidth(), renderer.GetHeight(), font);
}

bool MenuScene::ConfirmEditForm() {
    if (!IsEditable(editIndex)) {
        editForm.Close();
        editIndex = -1;
        return false;
    }

    std::string text = Trimmed(editForm.Text(static_cast<std::size_t>(EditField::Text)));

    if (text.empty()) {
        editForm.SetError("Text missing");
        return false;
    }

    std::size_t index = static_cast<std::size_t>(editIndex);

    std::vector<UiElement> elements = canvas.Elements();
    UiElement &element = elements[index];

    element.text = text;

    // The theme color stays without an entry, so it follows a new theme
    int chosen = static_cast<int>(editForm.ColorIndex(static_cast<std::size_t>(EditField::Color)));

    element.variant = chosen == DefaultVariant(element) ? -1 : chosen;

    const std::vector<SceneEntry> &scenes = engine.GetScenes().Entries();

    if (editForm.FieldCount() > static_cast<std::size_t>(EditField::Scene)) {
        std::size_t scene = editForm.Choice(static_cast<std::size_t>(EditField::Scene));

        if (scene < scenes.size()) {
            element.target = scenes[scene].id;
        }
    }

    editForm.Close();
    editIndex = -1;

    editor.Select(nullptr);

    canvas.SetElements(elements, engine.GetFont());

    editor.Select(&canvas.At(index));

    SaveLayout();

    return true;
}

void MenuScene::AskDeleteElement(std::size_t index) {
    if (!IsEditable(static_cast<int>(index))) {
        return;
    }

    pendingDeleteIndex = static_cast<int>(index);

    Renderer &renderer = engine.GetRenderer();

    confirmDialog.Open(
        "Delete " + canvas.At(index).Element().text,
        "Are you sure?",
        renderer.GetWidth(),
        renderer.GetHeight(),
        engine.GetFont(),
        FontVariant::Red
    );
}

void MenuScene::UpdateDeleteDialog(Vector2 mouse, bool clicked) {
    ConfirmDialog::Result result = confirmDialog.Update(mouse, clicked, engine.GetFont());

    if (result == ConfirmDialog::Result::Yes && IsEditable(pendingDeleteIndex)) {
        RemoveElement(static_cast<std::size_t>(pendingDeleteIndex));
    }

    if (result != ConfirmDialog::Result::None) {
        pendingDeleteIndex = -1;
    }
}

void MenuScene::AddElement(UiElement element) {
    // The list is rebuilt, old pointers are no longer valid afterwards
    editor.Select(nullptr);

    std::vector<UiElement> elements = canvas.Elements();

    element.position = Renderer::SnapToPixel(element.position);
    elements.push_back(std::move(element));

    canvas.SetElements(elements, engine.GetFont());

    editor.Select(&canvas.At(canvas.Count() - 1));

    SaveLayout();
}

void MenuScene::DuplicateElement(std::size_t index) {
    UiElement copy = canvas.At(index).Element();

    copy.locked = false;
    copy.position = {copy.position.x + DUPLICATE_OFFSET, copy.position.y + DUPLICATE_OFFSET};

    AddElement(std::move(copy));
}

void MenuScene::RemoveElement(std::size_t index) {
    editor.Select(nullptr);

    std::vector<UiElement> elements = canvas.Elements();

    elements.erase(elements.begin() + static_cast<std::ptrdiff_t>(index));

    canvas.SetElements(elements, engine.GetFont());

    SaveLayout();
}

void MenuScene::SaveLayout() {
    pendingSave = false;

    std::vector<UiElement> elements = canvas.Elements();

    history.Push(elements);

    WriteLayout(elements);
}

void MenuScene::WriteLayout(const std::vector<UiElement> &elements) {
    if (!layoutWritable) {
        TraceLog(LOG_WARNING, "UI: [%s] war nicht lesbar und wird nicht ueberschrieben", layoutFile.c_str());
        return;
    }

    UiLayout::Save(layoutFile, elements);
}

void MenuScene::UndoLayout() {
    RestoreHistoryState(history.Undo());
}

void MenuScene::RedoLayout() {
    RestoreHistoryState(history.Redo());
}

void MenuScene::RestoreHistoryState(const std::vector<UiElement> *state) {
    if (state == nullptr) {
        return;
    }

    // Remember by slot, the old pointers are no longer valid after the rebuild
    int selected = canvas.IndexOf(editor.Selected());

    std::vector<UiElement> elements = *state;

    editor.Select(nullptr);

    canvas.SetElements(elements, engine.GetFont());

    if (selected >= 0 && static_cast<std::size_t>(selected) < canvas.Count()) {
        editor.Select(&canvas.At(static_cast<std::size_t>(selected)));
    }

    WriteLayout(elements);
}

bool MenuScene::UsesArrowKeys() const {
    return engine.GetConfig().IsDevelopment() &&
           (editor.Selected() != nullptr || editor.IsMenuOpen() || textForm.IsOpen() || editForm.IsOpen() ||
            confirmDialog.IsOpen() || sceneMenu.IsOpen());
}

bool MenuScene::CapturesKeyboard() const {
    return engine.GetConfig().IsDevelopment() && (textForm.IsOpen() || editForm.IsOpen());
}

bool MenuScene::OnEscape() {
    if (!engine.GetConfig().IsDevelopment()) {
        return false;
    }

    if (textForm.IsOpen()) {
        textForm.Close();
        return true;
    }

    if (editForm.IsOpen()) {
        editForm.Close();
        editIndex = -1;
        return true;
    }

    if (confirmDialog.IsOpen()) {
        confirmDialog.Close();
        pendingDeleteIndex = -1;
        return true;
    }

    if (sceneMenu.IsOpen()) {
        sceneMenu.Close();
        pendingButtonText.clear();
        return true;
    }

    if (editor.IsMenuOpen()) {
        editor.CloseMenu();
        return true;
    }

    return false;
}

void MenuScene::DrawUI() {
    // While the settings are open, they replace the main menu
    if (engine.IsMenuOpen()) {
        return;
    }

    Renderer &renderer = engine.GetRenderer();
    const FontRenderer &font = engine.GetFont();
    const Theme &theme = engine.GetTheme();

    // The overlay makes texts readable on a background image, black stays black
    if (engine.GetBackground() != nullptr) {
        DrawRectangle(0, 0, renderer.GetWidth(), renderer.GetHeight(), theme.menuOverlay);
    }

    bool development = engine.GetConfig().IsDevelopment();

    // In the Development mode buttons do not react to the mouse, they are only edited
    canvas.Draw(font, theme.textVariant, theme.hoverVariant, theme.titleVariant, development ? -1 : hoveredButton);

    if (!development) {
        return;
    }

    editor.Draw(font, CaptionFor(canvas.IndexOf(editor.Selected())));
    if (engine.ShowsEditorHelp()) {
        editor.DrawHelp(font, renderer.GetHeight(), "Shift+Klick ausführen");
    }

    DrawEditStatus(engine, history.IsModified(), history.CanUndo(), history.CanRedo());

    // Last, so menus and form lie above everything
    editor.DrawMenu(font);
    sceneMenu.Draw(font);
    textForm.Draw(font);
    editForm.Draw(font);
    confirmDialog.Draw(font);
}
