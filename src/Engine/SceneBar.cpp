#include "SceneBar.h"

#include <filesystem>
#include <utility>

#include "Engine.h"

// Longest name of a scene
constexpr std::size_t SCENE_NAME_MAX_LENGTH = 16;

static std::string Trimmed(std::string text) {
    text.erase(0, text.find_first_not_of(' '));
    text.erase(text.find_last_not_of(' ') + 1);

    return text;
}

void SceneBar::Refresh(const SceneCatalog &catalog, const std::string &currentId, int viewWidth) {
    std::vector<std::string> labels;

    for (const SceneEntry &entry: catalog.Entries()) {
        // The lock in front of the name shows whether the scene is locked
        char lock = entry.locked ? FontRenderer::ICON_LOCK : FontRenderer::ICON_UNLOCK;

        labels.push_back(std::string(1, lock) + entry.name);
    }

    // Many scenes share the width of the viewport
    tabs.SetMaxWidth(viewWidth);
    tabs.SetTabs(std::move(labels));
    tabs.SetActive(catalog.IndexOf(currentId));
}

void SceneBar::RefreshTabs(Engine &engine) {
    Refresh(engine.GetScenes(), engine.CurrentSceneId(), engine.GetRenderer().GetWidth());
}

bool SceneBar::Update(Engine &engine, Vector2 mouse, bool leftClicked, bool rightClicked) {
    const FontRenderer &font = engine.GetFont();

    // The confirmation blocks everything until Yes or No is clicked
    if (dialog.IsOpen()) {
        ConfirmDialog::Result result = dialog.Update(mouse, leftClicked, font);

        if (result == ConfirmDialog::Result::Yes && dialogMode == DialogMode::Delete) {
            RemoveScene(engine, targetId);
        } else if (result == ConfirmDialog::Result::Yes && dialogMode == DialogMode::SaveEdit) {
            ApplyEdit(engine);
        }

        if (result != ConfirmDialog::Result::None) {
            dialogMode = DialogMode::None;
        }

        return true;
    }

    if (form.IsOpen()) {
        FormWindow::Result result = form.Update(mouse, leftClicked, font);

        if (formMode == FormMode::CreateOptions) {
            FollowTemplate(engine);
        }

        if (result == FormWindow::Result::Confirmed) {
            switch (formMode) {
                case FormMode::CreateBasics:
                    ConfirmCreateBasics(engine);
                    break;

                case FormMode::CreateOptions:
                    ConfirmCreateOptions(engine);
                    break;

                case FormMode::Edit:
                    ConfirmEdit(engine);
                    break;

                case FormMode::None:
                    form.Close();
                    break;
            }
        }

        if (!form.IsOpen()) {
            formMode = FormMode::None;
        }

        return true;
    }

    int tab = tabs.TabAt(mouse, font);

    // A right click on a tab opens its menu, even if one is already open
    if (rightClicked && tab >= 0) {
        OpenMenu(engine, static_cast<std::size_t>(tab));
        return true;
    }

    if (menu.IsOpen()) {
        if (rightClicked && !CheckCollisionPointRec(mouse, menu.Bounds(font))) {
            menu.Close();
        }

        int action = menu.Update(mouse, leftClicked, font);

        if (action != ContextMenu::NOTHING) {
            ApplyAction(engine, static_cast<Action>(action));
        }

        return true;
    }

    int hit = tabs.Update(mouse, leftClicked, font);

    if (hit >= 0 && hit != tabs.Active()) {
        // As always, the change happens at the next frame
        engine.GetScenes().Enter(engine, static_cast<std::size_t>(hit));
    } else if (hit == TabBar::PLUS) {
        OpenCreateForm(engine);
    }

    return (leftClicked || rightClicked) && tabs.Contains(mouse, engine.GetRenderer().GetWidth(), font);
}

void SceneBar::OpenMenu(Engine &engine, std::size_t index) {
    const SceneCatalog &catalog = engine.GetScenes();

    if (index >= catalog.Count()) {
        return;
    }

    const SceneEntry &entry = catalog.Entries()[index];

    targetId = entry.id;

    // Same order as Action. The lock shows what the item does.
    menu.SetTitle(entry.name);
    menu.SetItems({"Edit", "Duplicate", entry.locked ? "Unlock" : "Lock", "Delete"});

    menu.SetItemIcon(static_cast<std::size_t>(Action::Edit), FontRenderer::ICON_EDIT);
    menu.SetItemIcon(static_cast<std::size_t>(Action::Duplicate), FontRenderer::ICON_DUPLICATE);
    menu.SetItemIcon(
        static_cast<std::size_t>(Action::Lock),
        entry.locked ? FontRenderer::ICON_UNLOCK : FontRenderer::ICON_LOCK
    );
    menu.SetItemIcon(static_cast<std::size_t>(Action::Delete), FontRenderer::ICON_DELETE);

    // Locked scenes can neither be edited nor deleted, and some scenes must
    // always exist
    menu.SetItemEnabled(static_cast<std::size_t>(Action::Edit), !entry.locked);
    menu.SetItemEnabled(static_cast<std::size_t>(Action::Delete), !entry.locked && catalog.CanRemove(index));

    const FontRenderer &font = engine.GetFont();
    Renderer &renderer = engine.GetRenderer();

    // Directly below the tab, the bar stays free
    Rectangle bounds = tabs.TabBounds(index, font);

    menu.Open({bounds.x, bounds.y + bounds.height + 1.0f}, renderer.GetWidth(), renderer.GetHeight(), font);
}

void SceneBar::ApplyAction(Engine &engine, Action action) {
    SceneCatalog &catalog = engine.GetScenes();

    int index = catalog.IndexOf(targetId);

    if (index < 0) {
        return;
    }

    std::size_t scene = static_cast<std::size_t>(index);
    bool locked = catalog.Entries()[scene].locked;

    switch (action) {
        case Action::Edit:
            if (!locked) {
                OpenEditForm(engine, scene);
            }
            break;

        case Action::Duplicate:
            catalog.Duplicate(scene);
            break;

        case Action::Lock:
            catalog.SetLocked(scene, !locked);
            break;

        case Action::Delete:
            if (!locked) {
                AskDelete(engine, scene);
            }
            break;
    }

    RefreshTabs(engine);
}

void SceneBar::AddTypeFields(const SceneType &type, bool entry, const std::vector<int> &values) {
    if (type.entryPoint) {
        form.AddToggle("Entry point", entry);
    }

    for (std::size_t i = 0; i < type.options.size(); i++) {
        const SceneNumberOption &option = type.options[i];

        int value = i < values.size() ? values[i] : option.initial;

        form.AddNumber(option.label, value, option.minimum, option.maximum);
    }
}

void SceneBar::ReadTypeFields(const SceneType &type,
                              std::size_t firstField,
                              bool &entry,
                              std::vector<int> &values) const {
    std::size_t field = firstField;

    entry = false;
    values.clear();

    if (type.entryPoint) {
        entry = form.IsChecked(field);
        field++;
    }

    for (std::size_t i = 0; i < type.options.size(); i++) {
        values.push_back(form.Number(field));
        field++;
    }
}

void SceneBar::OpenCreateForm(Engine &engine) {
    const SceneCatalog &catalog = engine.GetScenes();

    std::vector<std::string> labels;

    for (const std::string &name: catalog.TypeNames()) {
        labels.push_back(catalog.FindType(name)->label);
    }

    if (labels.empty()) {
        return;
    }

    formMode = FormMode::CreateBasics;

    // Same order as BasicsField
    form.Clear();
    form.SetTitle("New scene");
    form.SetConfirmLabel("Create");
    form.AddText("Name", "", SCENE_NAME_MAX_LENGTH);
    form.AddDropdown("Type", labels, 0);

    Renderer &renderer = engine.GetRenderer();

    form.Open(renderer.GetWidth(), renderer.GetHeight(), engine.GetFont());
}

void SceneBar::ConfirmCreateBasics(Engine &engine) {
    const SceneCatalog &catalog = engine.GetScenes();

    std::string name = Trimmed(form.Text(static_cast<std::size_t>(BasicsField::Name)));

    if (name.empty()) {
        form.SetError("Name missing");
        return;
    }

    newName = name;
    newType = catalog.TypeNames().at(form.Choice(static_cast<std::size_t>(BasicsField::Type)));

    const SceneType *type = catalog.FindType(newType);

    // Without templates and options the scene is created right away
    if (type->templates.empty() && !type->entryPoint && type->options.empty()) {
        form.Close();
        CreateScene(engine, 0, {}, false);
        return;
    }

    formMode = FormMode::CreateOptions;
    shownTemplate = 0;

    // The template comes first, then the fields of the type
    form.Clear();
    form.SetTitle("New " + type->label);
    form.SetConfirmLabel("Create");

    if (!type->templates.empty()) {
        std::vector<std::string> templates;

        for (const SceneTemplate &sceneTemplate: type->templates) {
            templates.push_back(sceneTemplate.label);
        }

        form.AddDropdown("Template", templates, 0);
    }

    AddTypeFields(*type, false, catalog.TemplateOptions(newType, 0));

    Renderer &renderer = engine.GetRenderer();

    form.Open(renderer.GetWidth(), renderer.GetHeight(), engine.GetFont());
}

void SceneBar::FollowTemplate(Engine &engine) {
    const SceneCatalog &catalog = engine.GetScenes();
    const SceneType *type = catalog.FindType(newType);

    if (type == nullptr || type->templates.empty() || !form.IsOpen()) {
        return;
    }

    std::size_t chosen = form.Choice(0);

    if (chosen == shownTemplate) {
        return;
    }

    shownTemplate = chosen;

    // The numbers come after template and entry point
    std::size_t field = type->entryPoint ? 2 : 1;

    for (int value: catalog.TemplateOptions(newType, chosen)) {
        form.SetNumber(field, value);
        field++;
    }
}

void SceneBar::ConfirmCreateOptions(Engine &engine) {
    const SceneType *type = engine.GetScenes().FindType(newType);

    form.Close();

    if (type == nullptr) {
        return;
    }

    std::size_t templateIndex = 0;
    std::size_t firstField = 0;

    if (!type->templates.empty()) {
        templateIndex = form.Choice(0);
        firstField = 1;
    }

    bool entry = false;
    std::vector<int> values;

    ReadTypeFields(*type, firstField, entry, values);

    CreateScene(engine, templateIndex, values, entry);
}

void SceneBar::CreateScene(Engine &engine, std::size_t templateIndex, const std::vector<int> &values, bool entry) {
    SceneCatalog &catalog = engine.GetScenes();

    int index = catalog.Create(newName, newType, templateIndex, values);

    if (index < 0) {
        return;
    }

    if (entry) {
        catalog.SetEntryPoint(static_cast<std::size_t>(index), true);
    }

    // Like with a new tab, it goes straight in
    catalog.Enter(engine, static_cast<std::size_t>(index));

    RefreshTabs(engine);
}

void SceneBar::OpenEditForm(Engine &engine, std::size_t index) {
    const SceneCatalog &catalog = engine.GetScenes();

    const SceneEntry &entry = catalog.Entries().at(index);
    const SceneType *type = catalog.FindType(entry.type);

    targetId = entry.id;
    formMode = FormMode::Edit;

    // Same order as EditField
    form.Clear();
    form.SetTitle("Edit scene");
    form.SetConfirmLabel("Save");
    form.AddText("Name", entry.name, SCENE_NAME_MAX_LENGTH);

    // The images are listed without extension, "None" keeps the background black
    backgroundFiles = SceneCatalog::AvailableBackgrounds();

    std::vector<std::string> backgrounds = {"None"};
    std::size_t selectedBackground = 0;

    for (std::size_t i = 0; i < backgroundFiles.size(); i++) {
        backgrounds.push_back(std::filesystem::path(backgroundFiles[i]).stem().string());

        if (backgroundFiles[i] == entry.background) {
            selectedBackground = i + 1;
        }
    }

    form.AddDropdown("Background", backgrounds, selectedBackground);

    if (type != nullptr) {
        AddTypeFields(*type, catalog.EntryPoint() == static_cast<int>(index), catalog.ReadOptions(index));
    }

    Renderer &renderer = engine.GetRenderer();

    form.Open(renderer.GetWidth(), renderer.GetHeight(), engine.GetFont());
}

void SceneBar::ConfirmEdit(Engine &engine) {
    const SceneCatalog &catalog = engine.GetScenes();

    int index = catalog.IndexOf(targetId);

    if (index < 0) {
        form.Close();
        return;
    }

    std::string name = Trimmed(form.Text(static_cast<std::size_t>(EditField::Name)));

    if (name.empty()) {
        form.SetError("Name missing");
        return;
    }

    const SceneEntry &entry = catalog.Entries()[static_cast<std::size_t>(index)];
    const SceneType *type = catalog.FindType(entry.type);

    std::size_t background = form.Choice(static_cast<std::size_t>(EditField::Background));

    pendingName = name;
    pendingBackground = background == 0 ? "" : backgroundFiles.at(background - 1);
    pendingEntry = false;
    pendingValues.clear();

    std::string warning;

    if (type != nullptr) {
        ReadTypeFields(*type, static_cast<std::size_t>(EditField::TypeFields), pendingEntry, pendingValues);

        warning = catalog.OptionsWarning(static_cast<std::size_t>(index), pendingValues);
    }

    form.Close();

    if (warning.empty()) {
        ApplyEdit(engine);
        return;
    }

    dialogMode = DialogMode::SaveEdit;

    Renderer &renderer = engine.GetRenderer();

    dialog.Open(
        "Save " + entry.name,
        "Are you sure?\n" + warning,
        renderer.GetWidth(),
        renderer.GetHeight(),
        engine.GetFont(),
        FontVariant::Red
    );
}

void SceneBar::ApplyEdit(Engine &engine) {
    SceneCatalog &catalog = engine.GetScenes();

    int index = catalog.IndexOf(targetId);

    if (index < 0) {
        return;
    }

    std::size_t scene = static_cast<std::size_t>(index);
    const SceneType *type = catalog.FindType(catalog.Entries()[scene].type);

    catalog.Rename(scene, pendingName);
    catalog.SetBackground(scene, pendingBackground);

    if (type != nullptr && type->entryPoint) {
        catalog.SetEntryPoint(scene, pendingEntry);
    }

    if (type != nullptr && !type->options.empty() && pendingValues != catalog.ReadOptions(scene)) {
        catalog.ApplyOptions(scene, pendingValues);

        // The open scene still holds the old data and gets reloaded
        if (engine.CurrentSceneId() == targetId) {
            catalog.Enter(engine, scene);
        }
    }

    RefreshTabs(engine);
}

void SceneBar::AskDelete(Engine &engine, std::size_t index) {
    const SceneCatalog &catalog = engine.GetScenes();

    if (!catalog.CanRemove(index)) {
        return;
    }

    targetId = catalog.Entries()[index].id;
    dialogMode = DialogMode::Delete;

    Renderer &renderer = engine.GetRenderer();

    dialog.Open(
        "Delete " + catalog.Entries()[index].name,
        "Are you sure?\nThis cannot be undone.",
        renderer.GetWidth(),
        renderer.GetHeight(),
        engine.GetFont(),
        FontVariant::Red
    );
}

void SceneBar::RemoveScene(Engine &engine, const std::string &id) {
    SceneCatalog &catalog = engine.GetScenes();

    int index = catalog.IndexOf(id);

    if (index < 0 || !catalog.CanRemove(static_cast<std::size_t>(index))) {
        return;
    }

    // Leave the open scene first, it is deleted after the change
    if (engine.CurrentSceneId() == id) {
        std::size_t neighbour = index > 0 ? static_cast<std::size_t>(index - 1) : 1;

        if (catalog.Enter(engine, neighbour)) {
            sceneToRemove = id;
        }

        return;
    }

    catalog.Remove(static_cast<std::size_t>(index));

    RefreshTabs(engine);
}

void SceneBar::FinishPendingRemoval(SceneCatalog &catalog) {
    if (sceneToRemove.empty()) {
        return;
    }

    int index = catalog.IndexOf(sceneToRemove);

    if (index >= 0) {
        catalog.Remove(static_cast<std::size_t>(index));
    }

    sceneToRemove.clear();
}

bool SceneBar::IsHovering() const {
    // As long as a window is open, the tabs below it rest
    if (HasOpenWindow()) {
        return menu.IsHovering() || form.IsHovering() || dialog.IsHovering();
    }

    return tabs.IsHovering();
}

bool SceneBar::HasOpenWindow() const {
    return menu.IsOpen() || form.IsOpen() || dialog.IsOpen();
}

bool SceneBar::CapturesKeyboard() const {
    return form.IsOpen();
}

bool SceneBar::CloseWindows() {
    bool wasOpen = HasOpenWindow();

    menu.Close();
    form.Close();
    dialog.Close();

    formMode = FormMode::None;
    dialogMode = DialogMode::None;

    return wasOpen;
}

float SceneBar::Height(const FontRenderer &font) const {
    return tabs.Height(font);
}

void SceneBar::DrawTabs(int viewWidth, const FontRenderer &font) const {
    tabs.Draw(viewWidth, font);
}

void SceneBar::DrawWindows(const FontRenderer &font) const {
    menu.Draw(font);
    form.Draw(font);
    dialog.Draw(font);
}

const TabBar &SceneBar::Tabs() const {
    return tabs;
}

const ContextMenu &SceneBar::TabMenu() const {
    return menu;
}

const FormWindow &SceneBar::Form() const {
    return form;
}

const ConfirmDialog &SceneBar::Dialog() const {
    return dialog;
}
