#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "raylib.h"

#include "ConfirmDialog.h"
#include "ContextMenu.h"
#include "FontRenderer.h"
#include "FormWindow.h"
#include "SceneCatalog.h"
#include "TabBar.h"

class Engine;

// The scene bar in the Development mode: the tabs of all scenes at the top,
// plus their windows.
//
// A click on a tab switches to the scene. A right click opens a menu with
// Edit, Duplicate, Lock and Delete, locked scenes can neither be edited nor
// deleted. The "+" creates a new scene: first name and type, then template
// and options of the type, e.g. the size of a map.
//
// Everything the bar changes goes straight into the engine's SceneCatalog.
class SceneBar {
public:
    // Same order as in the menu of a tab
    enum class Action {
        Edit,
        Duplicate,

        // Label Lock or Unlock, depending on the state
        Lock,

        Delete
    };

    // Rebuilds the tabs from the catalog. currentId marks the open scene.
    void Refresh(const SceneCatalog &catalog, const std::string &currentId, int viewWidth);

    // The mouse in viewport coordinates. true if the click belongs to the bar
    // or one of its windows is open: the scene does not get it then.
    bool Update(Engine &engine, Vector2 mouse, bool leftClicked, bool rightClicked);

    // Was the mouse over something clickable during the last Update?
    bool IsHovering() const;

    bool HasOpenWindow() const;

    // Is a form open? Typed characters belong to it then.
    bool CapturesKeyboard() const;

    // Closes menu, form and dialog. true if one of them was open.
    bool CloseWindows();

    // If the open scene gets deleted, the bar first switches to another one.
    // The engine calls this as soon as the old scene is torn down: only then
    // does it disappear, and it can no longer save anything on exit.
    void FinishPendingRemoval(SceneCatalog &catalog);

    float Height(const FontRenderer &font) const;

    // Screen layer: the tabs, below the engine's displays
    void DrawTabs(int viewWidth, const FontRenderer &font) const;

    // Menu, form and dialog, above the scene
    void DrawWindows(const FontRenderer &font) const;

    const TabBar &Tabs() const;

    const ContextMenu &TabMenu() const;

    const FormWindow &Form() const;

    const ConfirmDialog &Dialog() const;

private:
    // What the form is currently showing
    enum class FormMode {
        None,

        // Name and type of a new scene
        CreateBasics,

        // Template, entry point and options of the new scene
        CreateOptions,

        Edit
    };

    // What the dialog is currently asking about
    enum class DialogMode {
        None,
        Delete,

        // The options of the type produced a warning
        SaveEdit
    };

    // Order of the fields in the first window for new scenes
    enum class BasicsField {
        Name,
        Type
    };

    // The first fields in the edit window, the fields of the type follow
    enum class EditField {
        Name,
        Background,

        TypeFields
    };

    void RefreshTabs(Engine &engine);

    void OpenMenu(Engine &engine, std::size_t index);

    void ApplyAction(Engine &engine, Action action);

    void OpenCreateForm(Engine &engine);

    void ConfirmCreateBasics(Engine &engine);

    void ConfirmCreateOptions(Engine &engine);

    // If a different template was chosen, the options take over its values
    void FollowTemplate(Engine &engine);

    // Creates the scene from newName and newType and switches to it
    void CreateScene(Engine &engine, std::size_t templateIndex, const std::vector<int> &values, bool entry);

    void OpenEditForm(Engine &engine, std::size_t index);

    // Checks the edit window and asks for confirmation on a warning
    void ConfirmEdit(Engine &engine);

    // Applies name, entry point and options from pendingName etc.
    void ApplyEdit(Engine &engine);

    void AskDelete(Engine &engine, std::size_t index);

    void RemoveScene(Engine &engine, const std::string &id);

    // Appends the checkbox for the entry point, if the type can be one, and the
    // options of the type to the form
    void AddTypeFields(const SceneType &type, bool entry, const std::vector<int> &values);

    // Reads the same fields back, starting at firstField
    void ReadTypeFields(const SceneType &type, std::size_t firstField, bool &entry, std::vector<int> &values) const;

    TabBar tabs;

    ContextMenu menu;

    FormWindow form;

    ConfirmDialog dialog;

    FormMode formMode = FormMode::None;

    DialogMode dialogMode = DialogMode::None;

    // The scene for menu, editing and deleting
    std::string targetId;

    // From the first step when creating
    std::string newName;
    std::string newType;

    // The template whose values the options are currently showing
    std::size_t shownTemplate = 0;

    // The backgrounds in the choice of the edit window, without "None" in front
    std::vector<std::string> backgroundFiles;

    // From the edit window, as long as the confirmation is open
    std::string pendingName;
    std::string pendingBackground;
    bool pendingEntry = false;
    std::vector<int> pendingValues;

    std::string sceneToRemove;
};
