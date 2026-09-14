#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "SceneObjects.h"
#include "Team.h"
#include "Engine/ConfirmDialog.h"
#include "Engine/EditHistory.h"
#include "Engine/FormWindow.h"
#include "Engine/ObjectEditor.h"
#include "Engine/Scene.h"
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
class FieldScene : public Scene {
public:
    void Exit() override;

    void Update(float dt) override;

    void UpdateDevelopment(float dt) override;

    void Draw() override;

    void DrawUI() override;

    Rectangle WorldArea() const override;

    bool UsesArrowKeys() const override;

    bool CapturesKeyboard() const override;

    bool OnEscape() override;

protected:
    // name is shown at the top left, so you can see which scene you are in
    FieldScene(Engine &engine, SceneOptions options, std::string name);

    // Loads the characters from assets/<filename> into the teams. In the
    // Development mode changes are written back to this file.
    void LoadObjects(const std::string &filename);

    // Adds a character and selects it. type is the index into the character
    // files, alphabetical. New characters belong to the enemies.
    void AddObject(std::size_t type, Vector2 position);

    void RemoveObject(const SpriteInstance *instance);

    // Switches the character between heroes and enemies
    void SwitchTeam(const SpriteInstance *instance);

    // Places a copy slightly offset next to it and selects it
    void DuplicateObject(const SpriteInstance *instance);

    // Saves the characters and stores the state as a step in the history
    void SaveObjects();

    // One step back or forward in the history, saved right away
    void UndoObjects();

    void RedoObjects();

    // Applies what the object editor reported in this frame
    void ApplyEditorChanges(const ObjectEditor::Changes &changes);

    // Opens the form for a new dummy object, created at position
    void OpenNewObjectForm(Vector2 position);

    // Creates a new object type from the form and places it. false and an error
    // message in the form if something is missing.
    bool CreateDummyObject();

    // Copies the definition of a type, the name gets "-Copy". No character is
    // created in the world.
    void DuplicateType(std::size_t type);

    // Asks before a type is deleted
    void AskDeleteType(std::size_t type);

    // Asks before a character is deleted
    void AskDeleteObject(const SpriteInstance *instance);

    // Handles the delete dialog, the mouse in viewport coordinates
    void UpdateConfirmDialog(Vector2 mouse, bool clicked);

    // Deletes the definition file. Characters of this type disappear from the
    // scene, as one step in the history. Z does not bring the file itself back.
    void DeleteType(const std::string &file);

    // Which hero is the player character. Without a call there is none, and then
    // nothing moves either.
    void SetPlayer(std::size_t heroIndex);

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

    Team heroes;
    Team enemies;

    // Only active in the Development mode, and only if the characters come from a
    // file
    ObjectEditor objectEditor;

    FormWindow newObjectForm;

    ConfirmDialog confirmDialog;

private:
    static constexpr std::size_t NO_PLAYER = static_cast<std::size_t>(-1);

    // Order of the fields in the form for new objects
    enum class NewObjectField {
        Name,
        Size,
        Health,
        Color
    };

    // Same order as in the menu a right click on a type in the add menu opens
    enum class TypeAction {
        Duplicate,
        Delete
    };

    // Same order as in the menu a right click on a character opens
    enum class ObjectAction {
        Duplicate,
        SwapTeam,
        Delete
    };

    // Default world, exactly as large as the viewport
    static constexpr float WORLD_WIDTH = 320.0f;
    static constexpr float WORLD_HEIGHT = 180.0f;

    static constexpr BoundsMode BOUNDS_MODE = BoundsMode::OutOfBounds;

    // nullptr if no player character is set
    Character *Player();

    // Both teams in one list, for collision and drawing
    std::vector<SpriteInstance *> AllSprites();

    // The same characters for the object editor
    std::vector<EditableObject *> AllEditables();

    // The selection of the editor. It only gets characters of this scene, so the
    // cast is safe.
    SpriteInstance *SelectedSprite() const;

    // Marks a character with the ring, nullptr clears the selection
    void Select(SpriteInstance *instance);

    bool EditsObjects() const;

    // The team the character belongs to, otherwise nullptr
    Team *TeamOf(const SpriteInstance *instance);

    // Name and team of the selected character, for the editor
    std::string SelectedCaption();

    // Name and team of a character, empty if it belongs to no team
    std::string CaptionFor(const SpriteInstance *instance);

    void LoadObjectTypes();

    void UpdateNewObjectForm();

    // Free file name for a new object type, e.g. characters/crate_2.json
    std::string UniqueCharacterFile(const std::string &name) const;

    std::vector<ObjectPlacement> CollectPlacements() const;

    // Rebuilds the teams from the placements. All pointers to characters become
    // invalid, the selection is cleared.
    void ApplyPlacements(const std::vector<ObjectPlacement> &placements);

    void WritePlacements(const std::vector<ObjectPlacement> &placements);

    void RestoreHistoryState(const std::vector<ObjectPlacement> *state);

    std::string name;

    Texture2D &selectorTexture;

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
    const SpriteInstance *pendingDeleteObject = nullptr;

    // Moved characters are only saved once the arrow keys and the mouse are
    // released again
    bool pendingSave = false;

    // Where the object from the form is created
    Vector2 newObjectPosition{0.0f, 0.0f};

    // Only valid while the scene is open
    EditHistory<std::vector<ObjectPlacement>> objectHistory;
};
