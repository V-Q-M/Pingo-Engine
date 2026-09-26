#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "AdvancedSettingsWindow.h"
#include "CharacterList.h"
#include "CharacterLookFields.h"
#include "GroupField.h"
#include "ObjectGroups.h"
#include "SceneObjects.h"
#include "Engine/ConfirmDialog.h"
#include "Engine/EditHistory.h"
#include "Engine/CodeEditor.h"
#include "Engine/FormWindow.h"
#include "Engine/ObjectEditor.h"
#include "Engine/Scene.h"
#include "Scripting/Api/ScriptWorld.h"
#include "Scripting/ScriptFiles.h"
#include "Scripting/ScriptHost.h"
#include "Engine/WorldBounds.h"

// Base for scenes on the map with characters, e.g. hub and combat.
//
// Contains selection by mouse, movement, collision, world bounds and the
// drawing of characters and health bars. The SceneOptions of the derived
// scene decide whether the player character can move.
//
// If a scene loads its characters from a file with LoadObjects, they can be
// moved, added and deleted in the Development mode. Z and Y undo and redo
// changes.
class FieldScene : public Scene, private ScriptWorld {
public:
    void Exit() override;

    void Update(float dt) override;

    void UpdateDevelopment(float dt) override;

    void Draw() override;

    void DrawUI() override;

    void DrawScreen() override;

    bool DrawsScreenLayer() const override;

    // Where the code editor sits, in the pixels of the window
    Rectangle EditorArea() const;

    Rectangle WorldArea() const override;

    bool UsesArrowKeys() const override;

    bool CapturesKeyboard() const override;

    bool OnEscape() override;

    // damage and simulate, see ConsoleCommands for adding more
    void AddCommands(ConsoleCommands &commands) override;

protected:
    // name is shown at the top left, so you can see which scene you are in
    FieldScene(Engine &engine, SceneOptions options, std::string name);

    // Loads the characters from assets/<filename>. In the Development mode
    // changes are written back to this file.
    void LoadObjects(const std::string &filename);

    // Adds a character of the group and selects it. type is the index into the
    // character files, alphabetical.
    void AddObject(std::size_t type, Vector2 position, const std::string &group = ObjectGroups::ENEMY);

    void RemoveObject(const SpriteInstance *instance);

    // Opens the window in which name, group and look of this one character are
    // changed. Its type stays as it is.
    void OpenEditObjectForm(const SpriteInstance *instance);

    // Places a copy slightly offset next to it and selects it
    void DuplicateObject(const SpriteInstance *instance);

    // Saves the characters and stores the state as a step in the history
    void SaveObjects();

    // One step back or forward in the history, saved right away
    void UndoObjects();

    void RedoObjects();

    // Applies what the object editor reported in this frame
    void ApplyEditorChanges(const ObjectEditor::Changes &changes);

    // Opens the form for a new object type, its first character is created at
    // position
    void OpenNewObjectForm(Vector2 position);

    // Creates a new object type from the form and places it. false and an error
    // message in the form if something is missing.
    bool CreateObjectType();

    // Applies the edit window to its character. false and an error message in
    // the form if something is missing.
    bool ApplyObjectEdit();

    // Copies the definition of a type, the name gets "-Copy". No character is
    // created in the world.
    void DuplicateType(std::size_t type);

    // Asks before a type is deleted
    void AskDeleteType(std::size_t type);

    // Asks before a character is deleted
    void AskDeleteObject(const SpriteInstance *instance);

    // Asks before the changes of a character become their own type
    void AskSaveObject(const SpriteInstance *instance);

    // Has this character changes of its own that a type could keep? Position and
    // group belong to the placement, not to the type.
    bool HasObjectChanges(const SpriteInstance *instance) const;

    // Writes the character with its changes as a new type. The character becomes
    // a plain instance of it, so its changes are no longer its own.
    void SaveObjectType(const SpriteInstance *instance, const std::string &name);

    // Handles the delete dialog, the mouse in viewport coordinates
    void UpdateConfirmDialog(Vector2 mouse, bool clicked);

    // Deletes the definition file. Characters of this type disappear from the
    // scene, as one step in the history. Z does not bring the file itself back.
    void DeleteType(const std::string &file);

    // Which ally is the player character: 0 for the first character of the group
    // ally, and so on. Without a call there is none, and then nothing moves either.
    void SetPlayer(std::size_t allyIndex);

    // World size and behavior at the edge. Default: as large as the viewport,
    // with BoundsMode::OutOfBounds. With Follow the camera follows the player
    // character.
    void SetWorld(Rectangle area, BoundsMode mode);

    // Moves the player character one frame further. Default: free, with collision
    // and world bounds.
    virtual void MovePlayer(Character &player, float dt, const std::vector<SpriteInstance *> &all);

    // Draws the ground below the characters, above the engine's background.
    // Default: nothing.
    virtual void DrawGround();

    // World layer above all characters and health bars. Default: nothing.
    virtual void DrawOverlay();

    // What a new script of this scene is written as. Only a scene with
    // Kind::Scene gets a Main. Default: Kind::Scene.
    virtual ScriptFiles::Kind ScriptKind() const;

    // The script names were read again, e.g. after one was created: the scene
    // updates its menus. Default: the add menu of the object editor.
    virtual void ScriptsChanged();

    // A script file was created, renamed or deleted from the windows below.
    // Default: nothing.
    virtual void ScriptCreated(const std::string &name);

    virtual void ScriptRenamed(const std::string &from, const std::string &to);

    virtual void ScriptRemoved(const std::string &name);

    // Reads the scripts of this scene from their folder and, for a scene of
    // Kind::Scene, creates its Main script if it does not exist yet
    void LoadScripts();

    // The window for a new script, or for renaming one. An empty name means
    // a new script.
    void OpenScriptForm(const std::string &rename);

    // Asks before a script is deleted, its files are gone afterwards
    void AskDeleteScript(const std::string &name);

    // Opens header and source of the script in the code editor
    void OpenScript(const std::string &name);

    // Code editor, script window and the delete question, in every scene
    // with scripts, whether it has objects or not. true if one of them is
    // open and took the frame, the scene then leaves mouse and keys alone.
    bool UpdateScriptWindows();

    CharacterList characters;

    // The groups of all scenes, loaded together with the characters
    ObjectGroups groups;

    // The scripts of this scene for the menus, see ScriptFiles
    std::vector<std::string> scriptNames;

    // Only active in the Development mode, and only if the characters come from a
    // file
    ObjectEditor objectEditor;

    FormWindow newObjectForm;

    FormWindow editObjectForm;

    // Asks for the name of a new or renamed script
    FormWindow scriptForm;

    // Shows the files of a script, opened from the add menu
    CodeEditor codeEditor;

    // Group dropdown, sprite, size, color and frames in the two windows above
    GroupField newObjectGroup;
    GroupField editObjectGroup;

    CharacterLookFields newObjectLook;
    CharacterLookFields editObjectLook;

    // Shadow and hitbox, opened from the edit window
    AdvancedSettingsWindow advancedWindow;

    ConfirmDialog confirmDialog;

    // The scripts of the scene while the game runs, see SceneOptions::scripts
    ScriptHost scripts;

private:
    static constexpr std::size_t NO_PLAYER = static_cast<std::size_t>(-1);

    // Order of the first fields in the form for new objects, the look fields and
    // the health follow
    enum class NewObjectField {
        Name,
        Group
    };

    // Order of the first fields in the edit window, the look fields follow.
    // Health and energy are the maximum values of this one character, 0 means
    // it has none of it.
    enum class EditObjectField {
        Name,
        Group,
        Health,
        Energy
    };

    // The sections of the add menu, in the order they are shown
    enum class AddSection {
        Objects,
        Scripts
    };

    // Same order as in the menu a right click on a type in the add menu opens
    enum class TypeAction {
        Duplicate,
        Delete
    };

    // Same order as in the menu a right click on a script in the add menu opens
    enum class ScriptAction {
        Rename,
        Delete
    };

    // Same order as in the menu a right click on a character opens
    enum class ObjectAction {
        Duplicate,
        Edit,
        Save,
        Delete
    };

    // Default world, exactly as large as the viewport
    static constexpr float WORLD_WIDTH = 320.0f;
    static constexpr float WORLD_HEIGHT = 180.0f;

    static constexpr BoundsMode BOUNDS_MODE = BoundsMode::OutOfBounds;

    // What the scripts of the scene may do, see ScriptWorld
    Character *FindObject(int id) override;

    std::vector<int> ObjectIds() const override;

    std::vector<int> ObjectIdsInGroup(const std::string &group) const override;

    // nullptr if no player character is set
    Character *Player();

    // All characters, for collision and drawing
    std::vector<SpriteInstance *> AllSprites();

    // The same characters for the object editor
    std::vector<EditableObject *> AllEditables();

    // The characters a click can select while playing, see
    // SceneOptions::selectablePlayer
    std::vector<SpriteInstance *> SelectableSprites();

    // The selection of the editor. It only gets characters of this scene, so the
    // cast is safe.
    SpriteInstance *SelectedSprite() const;

    // Marks a character with the ring, nullptr clears the selection
    void Select(SpriteInstance *instance);

    bool EditsObjects() const;

    // Name and group of the selected character, for the editor
    std::string SelectedCaption();

    // Name and group of a character, empty if it does not belong to the scene.
    // With Settings::showObjectIds the id for the console follows.
    std::string CaptionFor(const SpriteInstance *instance);

    // The id of a character for the console, counted from 1. 0 if it does not
    // belong to the scene.
    int IdOf(const SpriteInstance *instance) const;

    // nullptr if no character has this id
    Character *CharacterWithId(int id);

    // The console commands, see AddCommands. They get exactly as many words
    // as they asked for and return the answer of the console.
    std::string Damage(const std::vector<std::string> &arguments);
    std::string Health(const std::vector<std::string> &arguments);
    std::string Energy(const std::vector<std::string> &arguments);

    std::string Simulate(const std::vector<std::string> &arguments);

    void LoadObjectTypes();

    // Objects and scripts in the add menu, see ObjectEditor::SetAddSections
    void RefreshAddMenu();

    // Creates or renames the script from the window. false and an error
    // message in it if the name does not work.
    bool ApplyScriptForm();

    // Writes what the editor has, the files that really changed. Closing
    // saves as well, see HandleScriptRequest.
    void WriteScript();

    void CloseScript();

    // Does what the code editor asked for after a frame: ":w", ":wq", ":q"
    void HandleScriptRequest(CodeEditor::Request request);

    // The form with its group windows above it
    void UpdateObjectForm(FormWindow &form, GroupField &group, CharacterLookFields &look, bool edit);

    // Shadow and hitbox of the character in the edit window with this look,
    // without its advanced settings
    CharacterBody BaseBodyForEdit(const CharacterLook &look) const;

    // Free file name for a new object type, e.g. characters/crate_2.json
    std::string UniqueCharacterFile(const std::string &name) const;

    // The name itself if no type has it yet, otherwise with "-Copy" appended,
    // and counting up from "-Copy2"
    std::string UniqueTypeName(const std::string &name) const;

    std::vector<ObjectPlacement> CollectPlacements() const;

    // Rebuilds the characters from the placements. All pointers to characters
    // become invalid, the selection is cleared.
    void ApplyPlacements(const std::vector<ObjectPlacement> &placements);

    void WritePlacements(const std::vector<ObjectPlacement> &placements);

    void RestoreHistoryState(const std::vector<ObjectPlacement> *state);

    std::string name;

    WorldBounds bounds;

    SpriteInstance *selected = nullptr;

    std::size_t playerIndex = NO_PLAYER;

    std::string objectsFile;

    // If the file could not be read, it is not overwritten
    bool objectsWritable = false;

    // Character files for AddObject, alphabetical, and their names
    std::vector<std::string> objectTypes;
    std::vector<std::string> objectTypeNames;

    // What the dialog is currently asking about: a type or a character. As long
    // as it is open, nothing changes about the characters, the pointer stays valid.
    std::string pendingDeleteType;

    // The script the delete question is about, the one being renamed and the
    // one the editor has open
    std::string pendingDeleteScript;
    std::string renamedScript;
    std::string editedScript;
    const SpriteInstance *pendingDeleteObject = nullptr;

    // The character whose changes the dialog offers to save, and the name its
    // type would get
    const SpriteInstance *pendingSaveObject = nullptr;
    std::string pendingSaveName;

    // Moved characters are only saved once the arrow keys and the mouse are
    // released again
    bool pendingSave = false;

    // Where the object from the form is created
    Vector2 newObjectPosition{0.0f, 0.0f};

    // The health field of the form for new objects, behind the look fields
    std::size_t newObjectHealthField = 0;

    // The character in the edit window, as its position in the character list.
    // As long as the window is open, nothing changes about the characters.
    std::size_t editPlacement = 0;

    // The "Advanced settings" button in the edit window
    std::size_t editAdvancedField = 0;

    // Shadow and hitbox from the advanced settings, as long as the edit window
    // is open. Without a value the character keeps those of its type.
    std::optional<CharacterBody> editObjectBody;

    // Only valid while the scene is open
    EditHistory<std::vector<ObjectPlacement>> objectHistory;
};
