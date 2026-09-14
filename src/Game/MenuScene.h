#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "Engine/ConfirmDialog.h"
#include "Engine/ContextMenu.h"
#include "Engine/EditHistory.h"
#include "Engine/FormWindow.h"
#include "Engine/ObjectEditor.h"
#include "Engine/Scene.h"
#include "Engine/UiCanvas.h"

// A menu, e.g. the main menu: no pause menu, no movement. Texts and buttons
// come from assets/scenes/<id>.json, e.g. assets/scenes/main_menu.json.
// Without a file the menu is empty. The engine draws the background.
//
// In the Development mode the elements can be moved, a right click into
// empty space creates new texts and buttons, one on an element edits,
// duplicates or deletes it. Play, Settings and Quit can only be moved.
// Shift + click runs a button.
class MenuScene : public Scene {
public:
    explicit MenuScene(Engine &engine, const std::string &id = "main_menu");

    void Enter() override;

    void Exit() override;

    void Update(float dt) override;

    void UpdateDevelopment(float dt) override;

    void DrawUI() override;

    bool UsesArrowKeys() const override;

    bool CapturesKeyboard() const override;

    bool OnEscape() override;

protected:
    // Runs a button as if it had been clicked
    void Activate(const UiElement &button);

    // Applies what the editor reported in this frame
    void ApplyEditorChanges(const ObjectEditor::Changes &changes);

    // The form for a new text or button was confirmed. false and an error
    // message in the form if the text is missing.
    bool ConfirmTextForm();

    // Creates the button from the form, with the scene from sceneMenu
    void ChooseButtonScene(std::size_t sceneIndex);

    // Opens the window in which text, color and, for buttons, the scene of an
    // element are changed
    void OpenEditForm(std::size_t index);

    // Applies the window. false and an error message if the text is missing.
    bool ConfirmEditForm();

    // Asks before an element is deleted
    void AskDeleteElement(std::size_t index);

    // Handles the delete dialog, the mouse in viewport coordinates
    void UpdateDeleteDialog(Vector2 mouse, bool clicked);

    void AddElement(UiElement element);

    void DuplicateElement(std::size_t index);

    void RemoveElement(std::size_t index);

    // Saves and stores the state as a step in the history
    void SaveLayout();

    void UndoLayout();

    void RedoLayout();

    UiCanvas canvas;

    // Only active in the Development mode
    ObjectEditor editor;

    FormWindow textForm;

    FormWindow editForm;

    ConfirmDialog confirmDialog;

    // Chooses the scene for a new button
    ContextMenu sceneMenu;

    EditHistory<std::vector<UiElement>> history;

private:
    // Same order as in the add menu
    enum class AddType {
        Text,
        Button
    };

    // Same order as in the menu a right click on an element opens
    enum class ElementAction {
        Edit,
        Duplicate,
        Delete
    };

    // Order of the fields in the edit window. Scene only exists for buttons.
    enum class EditField {
        Text,
        Color,
        Scene
    };

    void OpenTextForm(AddType type, Vector2 position);

    void RestoreHistoryState(const std::vector<UiElement> *state);

    void WriteLayout(const std::vector<UiElement> &elements);

    // Does the element exist, and is it not locked?
    bool IsEditable(int index) const;

    // Description for the editor, e.g. "Button Hub"
    std::string CaptionFor(int index) const;

    // The color an element has without its own color
    int DefaultVariant(const UiElement &element) const;

    // Names of all scenes, in the order of the catalog
    std::vector<std::string> SceneNames() const;

    // The file with the elements, relative to assets
    std::string layoutFile;


    // If the file could not be read, it is not overwritten
    bool layoutWritable = true;

    int hoveredButton = -1;

    // Moved elements are only saved once arrow keys and mouse are released
    // again
    bool pendingSave = false;

    // Where the new element is created, and what is currently being created
    Vector2 addPosition{0.0f, 0.0f};
    AddType pendingAddType = AddType::Text;
    std::string pendingButtonText;

    // The element in the edit window or the delete dialog, otherwise -1. As long
    // as one of them is open, nothing changes about the elements.
    int editIndex = -1;
    int pendingDeleteIndex = -1;
};
