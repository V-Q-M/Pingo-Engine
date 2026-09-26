#include "FieldScene.h"

#include <algorithm>
#include <utility>

#include "ObjectEffects.h"
#include "Scripting/Api/ScriptRegistry.h"
#include "Scripting/ScriptFiles.h"
#include "SceneObjects.h"
#include "Engine/AssetFile.h"
#include "Engine/Collision.h"
#include "Engine/EditStatus.h"
#include "Engine/Engine.h"
#include "Engine/Names.h"

// How far a duplicated character lands next to the original
constexpr float DUPLICATE_OFFSET = 10.0f;

// Font variant of the add menu item that creates a new type or script
constexpr int NEW_TYPE_VARIANT = FontVariant::Grey;

// A script whose file exists but which is not compiled into the game yet
constexpr int UNBUILT_SCRIPT_VARIANT = FontVariant::Grey;

// Distance of the code editor to the edges of the screen
constexpr float SCRIPT_EDITOR_MARGIN = 4.0f;

// Names of types and of single characters
constexpr std::size_t OBJECT_NAME_MAX_LENGTH = 12;

constexpr int NEW_OBJECT_HEALTH = 100;
constexpr int OBJECT_MAX_HEALTH = 9999;
constexpr int OBJECT_MAX_ENERGY = 9999;

// The number in a word, so an id also works as "[3]" or "#3" and an amount
// may carry its sign, e.g. "-20"
static int ParseNumber(const std::string &text) {
    std::string digits;

    for (char letter: text) {
        if ((letter >= '0' && letter <= '9') || letter == '+' || letter == '-') {
            digits += letter;
        }
    }

    try {
        return std::stoi(digits);
    } catch (const std::exception &) {
        return 0;
    }
}

// The id of an object as the console and the editor write it. Brackets instead
// of a hash: the font has them, and they can be typed into the console.
static std::string ObjectId(int id) {
    return "[" + std::to_string(id) + "]";
}

FieldScene::FieldScene(Engine &engine, SceneOptions options, std::string name)
    : Scene(engine, std::move(options)),
      name(std::move(name)),
      bounds({0.0f, 0.0f, WORLD_WIDTH, WORLD_HEIGHT}, BOUNDS_MODE) {
}

void FieldScene::LoadObjects(const std::string &filename) {
    objectsFile = filename;

    std::vector<ObjectPlacement> placements;
    objectsWritable = SceneObjects::Load(filename, placements);

    ApplyPlacements(placements);

    // While the game runs the Main script of the scene comes along
    if (Options().scripts && !engine.GetConfig().IsDevelopment()) {
        scripts.Load(Options().id, *this);
    }

    if (engine.GetConfig().IsDevelopment()) {
        groups = ObjectGroups::Load();

        LoadScripts();
        LoadObjectTypes();

        objectEditor.SetObjectActions(
            {"Duplicate", "Edit", "Save object", "Delete"},
            {
                FontRenderer::ICON_DUPLICATE,
                FontRenderer::ICON_EDIT,
                FontRenderer::ICON_SAVE,
                FontRenderer::ICON_DELETE
            }
        );

        // Only a character that changed something about its type has anything
        // worth saving as a new one
        objectEditor.SetObjectActionFilter([this](const EditableObject &object, std::size_t action) {
            if (static_cast<ObjectAction>(action) != ObjectAction::Save) {
                return true;
            }

            return HasObjectChanges(static_cast<const SpriteInstance *>(&object));
        });

        objectEditor.SetObjectTitle([this](const EditableObject &object) {
            return CaptionFor(static_cast<const SpriteInstance *>(&object));
        });

        // Starting point of the history, the way the characters stand now
        objectHistory.Reset(CollectPlacements());
    }
}

void FieldScene::ApplyPlacements(const std::vector<ObjectPlacement> &placements) {
    Select(nullptr);
    objectEditor.Select(nullptr);

    characters.Clear();

    Assets &assets = engine.GetAssets();

    for (const ObjectPlacement &placement: placements) {
        Character &character = characters.Add(
            assets,
            placement.character,
            placement.position,
            placement.group,
            placement.overrides
        );

        if (placement.health >= 0) {
            character.SetHealth(placement.health);
        }
    }
}

void FieldScene::LoadObjectTypes() {
    objectTypes = CharacterDefinition::AvailableTypes();

    objectTypeNames.clear();

    for (const std::string &file: objectTypes) {
        objectTypeNames.push_back(CharacterDefinition::Load(file).name);
    }

    RefreshAddMenu();
}

// The add menu has two sections: the object types as before, and the scripts
// of the scene. Both end with "+New".
void FieldScene::RefreshAddMenu() {
    ObjectEditor::AddSection objects;

    objects.label = "Objects";
    objects.items = objectTypeNames;
    objects.extra = "+New";
    objects.extraVariant = NEW_TYPE_VARIANT;

    // Same order as TypeAction
    objects.actions = {"Duplicate", "Delete"};
    objects.actionIcons = {FontRenderer::ICON_DUPLICATE, FontRenderer::ICON_DELETE};

    if (!Options().scripts) {
        objectEditor.SetAddSections({objects});
        return;
    }

    ObjectEditor::AddSection section;

    section.label = "Scripts";
    section.items = scriptNames;
    section.extra = "+New";
    section.extraVariant = NEW_TYPE_VARIANT;

    // A script the game does not know yet only runs after the next build
    for (const std::string &name: scriptNames) {
        bool built = ScriptRegistry::Knows(Options().id, name);

        section.itemVariants.push_back(built ? FontVariant::White : UNBUILT_SCRIPT_VARIANT);
    }

    // Same order as ScriptAction
    section.actions = {"Rename", "Delete"};
    section.actionIcons = {FontRenderer::ICON_EDIT, FontRenderer::ICON_DELETE};

    // The script of the scene stays where it is
    section.actionFilter = [this](std::size_t item, std::size_t) {
        return item < scriptNames.size() && scriptNames[item] != ScriptFiles::MAIN;
    };

    // Same order as AddSection
    objectEditor.SetAddSections({objects, section});
}

// Every normal scene with scripts has a Main, the engine calls its Update.
// If the file is missing, e.g. for a scene that was just created, it is
// written. Tile scripts run by themselves and need none.
void FieldScene::LoadScripts() {
    if (!Options().scripts) {
        return;
    }

    scriptNames = ScriptFiles::Available(Options().id);

    bool needsMain = ScriptKind() == ScriptFiles::Kind::Scene &&
                     std::find(scriptNames.begin(), scriptNames.end(), ScriptFiles::MAIN) == scriptNames.end();

    if (needsMain && ScriptFiles::Create(Options().id, ScriptFiles::MAIN)) {
        scriptNames = ScriptFiles::Available(Options().id);
    }

    ScriptsChanged();
}

ScriptFiles::Kind FieldScene::ScriptKind() const {
    return ScriptFiles::Kind::Scene;
}

void FieldScene::ScriptsChanged() {
    RefreshAddMenu();
}

void FieldScene::ScriptCreated(const std::string &) {
}

void FieldScene::ScriptRenamed(const std::string &, const std::string &) {
}

void FieldScene::ScriptRemoved(const std::string &) {
}

// The same window for both: without a name to rename it asks for a new script
void FieldScene::OpenScriptForm(const std::string &rename) {
    renamedScript = rename;

    scriptForm.Clear();
    scriptForm.SetTitle(rename.empty() ? "New script" : "Rename script");
    scriptForm.SetConfirmLabel(rename.empty() ? "Create" : "Rename");
    scriptForm.SetHint("Runs after the next build");
    scriptForm.AddText("Name", rename, ScriptFiles::NAME_MAX_LENGTH);

    Renderer &renderer = engine.GetRenderer();

    scriptForm.Open(renderer.GetWidth(), renderer.GetHeight(), engine.GetFont());
}

bool FieldScene::ApplyScriptForm() {
    std::string name = TrimSpaces(scriptForm.Text(0));

    if (!ScriptFiles::IsValidName(name)) {
        scriptForm.SetError("Letters and digits only");
        return false;
    }

    // Renaming never makes a Main either, see ScriptFiles::Rename
    bool reserved = ScriptFiles::IsReservedName(name, ScriptKind()) ||
                    (!renamedScript.empty() && name == ScriptFiles::MAIN);

    if (reserved) {
        scriptForm.SetError("Name is reserved");
        return false;
    }

    std::string renamed = renamedScript;

    bool done = renamed.empty()
                    ? ScriptFiles::Create(Options().id, name, ScriptKind())
                    : ScriptFiles::Rename(Options().id, renamed, name);

    if (!done) {
        scriptForm.SetError(renamed.empty() ? "Name is taken" : "Renaming failed");
        return false;
    }

    scriptForm.Close();
    renamedScript.clear();

    LoadScripts();

    if (renamed.empty()) {
        ScriptCreated(name);
    } else {
        ScriptRenamed(renamed, name);
    }

    return true;
}

// Both files of the script, the source first: that is where the work happens
void FieldScene::OpenScript(const std::string &name) {
    editedScript = name;

    std::vector<CodeEditor::File> files;

    for (const std::string &extension: {".cpp", ".h"}) {
        CodeEditor::File file;

        file.label = name + extension;
        file.path = extension;
        file.text = ScriptFiles::Read(Options().id, name, extension);

        files.push_back(file);
    }

    Renderer &renderer = engine.GetRenderer();

    // The lines that make the script known to the engine are none of the
    // developer's business: the editor hides them, see ScriptRegistry. Only
    // these exact lines, so a comment of their own always stays visible.
    codeEditor.SetHiddenLines([](const std::string &line) {
        std::size_t first = line.find_first_not_of(' ');

        if (first == std::string::npos) {
            return false;
        }

        std::string code = line.substr(first);

        return code.rfind(ScriptFiles::REGISTRY_MACRO, 0) == 0 ||
               code == ScriptFiles::REGISTRY_INCLUDE ||
               code == ScriptFiles::REGISTRY_COMMENT ||
               code == ScriptFiles::HEADER_GUARD ||
               code == ScriptFiles::SCRIPT_INCLUDE ||
               code == ScriptFiles::TILE_SCRIPT_INCLUDE;
    });

    codeEditor.Open(
        std::move(files),
        EditorArea(),
        GetScreenWidth(),
        GetScreenHeight(),
        renderer.GetScale()
    );
}

// Where the editor sits in the pixels of the window: the same place as in the
// viewport, only in the real pixels of the screen. It is worked out again
// every frame, so it follows a window that is resized.
Rectangle FieldScene::EditorArea() const {
    Renderer &renderer = engine.GetRenderer();

    // Below the scene bar, above the mode label at the bottom left
    float top = engine.UiTop();

    Rectangle window{
        SCRIPT_EDITOR_MARGIN,
        top,
        static_cast<float>(renderer.GetWidth()) - 2.0f * SCRIPT_EDITOR_MARGIN,
        engine.UiBottom() - top - SCRIPT_EDITOR_MARGIN
    };

    auto scale = static_cast<float>(renderer.GetScale());
    Vector2 letterbox = renderer.GetLetterbox();

    return {
        letterbox.x + window.x * scale,
        letterbox.y + window.y * scale,
        window.width * scale,
        window.height * scale
    };
}

// Closing saves: only the files that really changed are written
void FieldScene::WriteScript() {
    for (const CodeEditor::File &file: codeEditor.Files()) {
        if (file.changed && !ScriptFiles::Write(Options().id, editedScript, file.path, file.text)) {
            TraceLog(LOG_WARNING, "SCRIPTS: [%s] konnte nicht gespeichert werden", file.label.c_str());
        }
    }

    codeEditor.MarkSaved();
}

void FieldScene::CloseScript() {
    codeEditor.Close();
    editedScript.clear();

    // A script that was just created only counts from the next build on, the
    // menu says so
    LoadScripts();
}

void FieldScene::HandleScriptRequest(CodeEditor::Request request) {
    switch (request) {
        case CodeEditor::Request::Save:
            WriteScript();
            break;

        case CodeEditor::Request::SaveAndClose:
            WriteScript();
            CloseScript();
            break;

        case CodeEditor::Request::Close:
            CloseScript();
            break;

        case CodeEditor::Request::None:
            break;
    }
}

void FieldScene::AskDeleteScript(const std::string &name) {
    if (name == ScriptFiles::MAIN) {
        return;
    }

    pendingDeleteScript = name;
    pendingDeleteType.clear();
    pendingDeleteObject = nullptr;
    pendingSaveObject = nullptr;

    Renderer &renderer = engine.GetRenderer();

    confirmDialog.Open(
        "Delete " + name,
        "Its files are gone then.",
        renderer.GetWidth(),
        renderer.GetHeight(),
        engine.GetFont(),
        FontVariant::Red
    );
}

std::vector<ObjectPlacement> FieldScene::CollectPlacements() const {
    std::vector<ObjectPlacement> placements;

    for (const Character &character: characters.Characters()) {
        ObjectPlacement placement;

        placement.character = character.DefinitionFile();
        placement.group = character.Group();
        placement.position = character.GetCharacter().Position();
        placement.overrides = character.Overrides();

        // Full health does not need to be in the file
        if (character.Health() != character.MaxHealth()) {
            placement.health = character.Health();
        }

        placements.push_back(placement);
    }

    return placements;
}

void FieldScene::SaveObjects() {
    pendingSave = false;

    std::vector<ObjectPlacement> placements = CollectPlacements();

    // Every save is a step in the history. Held arrow keys are only saved when
    // released, so they end up as a single step.
    objectHistory.Push(placements);

    WritePlacements(placements);
}

void FieldScene::WritePlacements(const std::vector<ObjectPlacement> &placements) {
    if (!objectsWritable) {
        TraceLog(LOG_WARNING, "SCENE: [%s] war nicht lesbar und wird nicht ueberschrieben", objectsFile.c_str());
        return;
    }

    SceneObjects::Save(objectsFile, placements);
}

void FieldScene::UndoObjects() {
    RestoreHistoryState(objectHistory.Undo());
}

void FieldScene::RedoObjects() {
    RestoreHistoryState(objectHistory.Redo());
}

void FieldScene::RestoreHistoryState(const std::vector<ObjectPlacement> *state) {
    if (state == nullptr) {
        return;
    }

    // Remember the selection by its position in the list, because after the
    // rebuild the old pointers are no longer valid. If a move is undone, the same
    // character stays selected.
    int index = characters.IndexOf(SelectedSprite());

    std::vector<ObjectPlacement> placements = *state;

    ApplyPlacements(placements);

    if (index >= 0 && static_cast<std::size_t>(index) < characters.Characters().size()) {
        objectEditor.Select(&characters.Characters()[static_cast<std::size_t>(index)].GetCharacter());
    }

    WritePlacements(placements);
}

bool FieldScene::EditsObjects() const {
    return engine.GetConfig().IsDevelopment() && !objectsFile.empty();
}

void FieldScene::AddObject(std::size_t type, Vector2 position, const std::string &group) {
    if (type >= objectTypes.size()) {
        return;
    }

    // The list may move when adding, old pointers are no longer valid then
    Select(nullptr);
    objectEditor.Select(nullptr);

    Character &added = characters.Add(engine.GetAssets(), objectTypes[type], Renderer::SnapToPixel(position), group);

    objectEditor.Select(&added.GetCharacter());

    SaveObjects();
}

void FieldScene::RemoveObject(const SpriteInstance *instance) {
    if (characters.Find(instance) == nullptr) {
        return;
    }

    Select(nullptr);
    objectEditor.Select(nullptr);

    characters.Remove(instance);

    SaveObjects();
}

void FieldScene::DuplicateObject(const SpriteInstance *instance) {
    Character *original = characters.Find(instance);

    if (original == nullptr) {
        return;
    }

    // Copy first: the list may move when adding
    Character copy = *original;

    Vector2 position = copy.GetCharacter().Position();

    copy.GetCharacter().SetPosition({position.x + DUPLICATE_OFFSET, position.y + DUPLICATE_OFFSET});
    copy.GetCharacter().SetSelected(false);

    Select(nullptr);
    objectEditor.Select(nullptr);

    Character &added = characters.Add(std::move(copy));

    objectEditor.Select(&added.GetCharacter());

    SaveObjects();
}

std::string FieldScene::SelectedCaption() {
    return CaptionFor(SelectedSprite());
}

std::string FieldScene::CaptionFor(const SpriteInstance *instance) {
    const Character *character = characters.Find(instance);

    if (character == nullptr) {
        return "";
    }

    std::string caption = character->Name() + " " + groups.NameOf(character->Group());

    // The id is what the console talks to, so it only shows with the setting
    if (engine.GetConfig().HasDebugTools() && engine.GetSettings().showObjectIds) {
        caption += " " + ObjectId(IdOf(instance));
    }

    return caption;
}

int FieldScene::IdOf(const SpriteInstance *instance) const {
    int index = characters.IndexOf(instance);

    return index < 0 ? 0 : index + 1;
}

Character *FieldScene::CharacterWithId(int id) {
    if (id < 1 || static_cast<std::size_t>(id) > characters.Characters().size()) {
        return nullptr;
    }

    return &characters.Characters()[static_cast<std::size_t>(id - 1)];
}

// The console commands of every scene with characters. The registry checks
// the number of words before a command runs, see ConsoleCommands.
void FieldScene::AddCommands(ConsoleCommands &commands) {
    commands.Add({"damage", "(amount) (id)", 2, [this](const std::vector<std::string> &arguments) {
        return Damage(arguments);
    }});

    commands.Add({"health", "(amount) (id)", 2, [this](const std::vector<std::string> &arguments) {
        return Health(arguments);
    }});

    commands.Add({"energy", "(amount) (id)", 2, [this](const std::vector<std::string> &arguments) {
        return Energy(arguments);
    }});

    commands.Add({"simulate", "(effect) (id)", 2, [this](const std::vector<std::string> &arguments) {
        return Simulate(arguments);
    }});
}

// "damage (amount) (id)" takes health from an object
std::string FieldScene::Damage(const std::vector<std::string> &arguments) {
    int amount = ParseNumber(arguments[0]);
    int id = ParseNumber(arguments[1]);

    Character *target = CharacterWithId(id);

    if (target == nullptr) {
        return "No object " + ObjectId(id);
    }

    if (target->IsIndestructible()) {
        return target->Name() + " has no health";
    }

    target->SetHealth(target->Health() - amount);

    SaveObjects();

    return target->Name() + " " + std::to_string(target->Health()) + "/" + std::to_string(target->MaxHealth());
}

// "health (amount) (id)" adds health to an object, a negative amount takes it
std::string FieldScene::Health(const std::vector<std::string> &arguments) {
    int amount = ParseNumber(arguments[0]);
    int id = ParseNumber(arguments[1]);

    Character *target = CharacterWithId(id);

    if (target == nullptr) {
        return "No object " + ObjectId(id);
    }

    if (target->IsIndestructible()) {
        return target->Name() + " has no health";
    }

    target->SetHealth(target->Health() + amount);

    SaveObjects();

    return target->Name() + " " + std::to_string(target->Health()) + "/" + std::to_string(target->MaxHealth());
}

// "energy (amount) (id)" adds energy to an object, a negative amount takes it
std::string FieldScene::Energy(const std::vector<std::string> &arguments) {
    int amount = ParseNumber(arguments[0]);
    int id = ParseNumber(arguments[1]);

    Character *target = CharacterWithId(id);

    if (target == nullptr) {
        return "No object " + ObjectId(id);
    }

    if (!target->HasEnergy()) {
        return target->Name() + " has no energy";
    }

    target->SetEnergy(target->Energy() + amount);

    SaveObjects();

    return target->Name() + " " + std::to_string(target->Energy()) + "/" + std::to_string(target->MaxEnergy());
}

// "simulate (effect) (id)" puts an effect on an object, "simulate none (id)"
// takes it off again
std::string FieldScene::Simulate(const std::vector<std::string> &arguments) {
    int id = ParseNumber(arguments[1]);

    Character *target = CharacterWithId(id);

    if (target == nullptr) {
        return "No object " + ObjectId(id);
    }

    const std::string wanted = ToLower(arguments[0]);

    if (wanted == "none" || wanted == "off") {
        target->SetEffect(ObjectEffects::NONE);

        return target->Name() + " back to normal";
    }

    const ObjectEffect *effect = ObjectEffects::Match(wanted);

    if (effect == nullptr) {
        return ObjectEffects::Names();
    }

    target->SetEffect(effect->id);

    // Without a row of its own the effect is on the object but cannot be seen
    return target->Name() + " " + effect->id + (target->ShowsEffect(effect->id) ? "" : ", no row for it");
}

void FieldScene::SetPlayer(std::size_t allyIndex) {
    playerIndex = allyIndex;
}

void FieldScene::SetWorld(Rectangle area, BoundsMode mode) {
    bounds = WorldBounds(area, mode);
}

// Without movement in the SceneOptions the input returns (0, 0) and the
// character stands still. The scene itself does not need to know about it.
void FieldScene::MovePlayer(Character &player, float dt, const std::vector<SpriteInstance *> &all) {
    player.Move(engine.GetInput().MovementDirection(), dt, all);

    bounds.Apply(player.GetCharacter());
}

void FieldScene::DrawGround() {
    // The engine draws the background image, see SceneEntry::background
}

void FieldScene::DrawOverlay() {
}

// The scripts talk about objects by the id the editor and the console show,
// see IdOf: it is the place in the list, counted from 1.
Character *FieldScene::FindObject(int id) {
    return CharacterWithId(id);
}

std::vector<int> FieldScene::ObjectIds() const {
    std::vector<int> ids;

    for (std::size_t i = 0; i < characters.Characters().size(); i++) {
        ids.push_back(static_cast<int>(i) + 1);
    }

    return ids;
}

std::vector<int> FieldScene::ObjectIdsInGroup(const std::string &group) const {
    std::vector<int> ids;

    const std::vector<Character> &all = characters.Characters();

    for (std::size_t i = 0; i < all.size(); i++) {
        if (all[i].Group() == group) {
            ids.push_back(static_cast<int>(i) + 1);
        }
    }

    return ids;
}

Character *FieldScene::Player() {
    std::size_t allies = 0;

    for (Character &character: characters.Characters()) {
        if (character.Group() != ObjectGroups::ALLY) {
            continue;
        }

        if (allies == playerIndex) {
            return &character;
        }

        allies++;
    }

    return nullptr;
}

std::vector<EditableObject *> FieldScene::AllEditables() {
    std::vector<SpriteInstance *> sprites = AllSprites();

    return std::vector<EditableObject *>(sprites.begin(), sprites.end());
}

SpriteInstance *FieldScene::SelectedSprite() const {
    return static_cast<SpriteInstance *>(objectEditor.Selected());
}

// While playing, the player character can be excluded, see
// SceneOptions::selectablePlayer
std::vector<SpriteInstance *> FieldScene::SelectableSprites() {
    std::vector<SpriteInstance *> all = AllSprites();

    if (Options().selectablePlayer) {
        return all;
    }

    Character *player = Player();

    if (player == nullptr) {
        return all;
    }

    all.erase(std::remove(all.begin(), all.end(), &player->GetCharacter()), all.end());

    return all;
}

std::vector<SpriteInstance *> FieldScene::AllSprites() {
    std::vector<SpriteInstance *> all;

    for (Character &character: characters.Characters()) {
        all.push_back(&character.GetCharacter());
    }

    return all;
}

void FieldScene::Select(SpriteInstance *instance) {
    if (selected != nullptr) {
        selected->SetSelected(false);
    }

    selected = instance;

    if (selected != nullptr) {
        selected->SetSelected(true);
    }
}

Rectangle FieldScene::WorldArea() const {
    return bounds.Area();
}

// With a selection the arrow keys move the character. With a context menu
// open they rest, so the world does not move away under the menu.
bool FieldScene::UsesArrowKeys() const {
    if (confirmDialog.IsOpen()) {
        return true;
    }

    return EditsObjects() &&
           (objectEditor.Selected() != nullptr || objectEditor.IsMenuOpen() || newObjectForm.IsOpen() ||
            editObjectForm.IsOpen());
}

bool FieldScene::CapturesKeyboard() const {
    // Whoever writes a script, or its name, owns the keyboard, whatever the
    // scene is
    if (codeEditor.IsOpen() || scriptForm.IsOpen()) {
        return true;
    }

    return EditsObjects() && (newObjectForm.IsOpen() || editObjectForm.IsOpen());
}

bool FieldScene::OnEscape() {
    // The editor answers ESC itself: in the insert mode it goes back to the
    // normal one, without the Vim bindings it saves and closes. It lies above
    // everything, so the key is used up either way, even in a scene that has
    // no objects of its own.
    if (codeEditor.IsOpen()) {
        HandleScriptRequest(codeEditor.OnEscape());
        return true;
    }

    // The delete question and the script window also exist in scenes
    // without objects of their own
    if (confirmDialog.IsOpen()) {
        confirmDialog.Close();
        pendingDeleteType.clear();
        pendingDeleteScript.clear();
        pendingDeleteObject = nullptr;
        pendingSaveObject = nullptr;
        pendingSaveName.clear();
        return true;
    }

    if (scriptForm.IsOpen()) {
        if (!scriptForm.CloseList()) {
            scriptForm.Close();
        }

        return true;
    }

    if (!EditsObjects()) {
        return false;
    }

    // From the inside out: the advanced settings and group windows, then an
    // open list, then the form
    if (advancedWindow.IsOpen()) {
        advancedWindow.Close();
        return true;
    }

    if (newObjectGroup.CloseWindow() || editObjectGroup.CloseWindow()) {
        return true;
    }

    for (FormWindow *form: {&newObjectForm, &editObjectForm}) {
        if (form->IsOpen()) {
            if (!form->CloseList()) {
                form->Close();
            }

            return true;
        }
    }

    if (objectEditor.IsMenuOpen()) {
        objectEditor.CloseMenu();
        return true;
    }

    return false;
}

void FieldScene::OpenNewObjectForm(Vector2 position) {
    newObjectPosition = position;

    newObjectForm.Clear();
    newObjectForm.SetTitle("New object");
    newObjectForm.SetConfirmLabel("Create");

    // Same order as NewObjectField, the look and the health follow
    newObjectForm.AddText("Name", "", OBJECT_NAME_MAX_LENGTH);
    newObjectGroup.Add(newObjectForm, groups, ObjectGroups::ENEMY);

    // Without a sprite a new type starts as a square in the first color
    CharacterLook look;
    look.color = CharacterLookFields::Colors().front();

    newObjectLook.Add(newObjectForm, look);

    newObjectHealthField = newObjectForm.AddNumber("Health", NEW_OBJECT_HEALTH, 0, OBJECT_MAX_HEALTH);

    Renderer &renderer = engine.GetRenderer();

    newObjectForm.Open(renderer.GetWidth(), renderer.GetHeight(), engine.GetFont());
}

void FieldScene::UpdateObjectForm(FormWindow &form, GroupField &group, CharacterLookFields &look, bool edit) {
    Renderer &renderer = engine.GetRenderer();
    const FontRenderer &font = engine.GetFont();
    const MouseButtons &mouse = engine.GetMouse();

    // The advanced settings lie above the edit window. Done keeps the values
    // until the edit window is saved.
    if (edit && advancedWindow.IsOpen()) {
        AdvancedSettingsWindow::Result advanced = advancedWindow.Update(
            renderer.MouseViewportPosition(),
            mouse.IsLeftClicked(),
            font
        );

        if (advanced == AdvancedSettingsWindow::Result::Done) {
            CharacterBody body = advancedWindow.Read();

            // Values that match the type stay with the type, with its precision
            if (body == BaseBodyForEdit(look.Read(form))) {
                editObjectBody.reset();
            } else {
                editObjectBody = body;
            }
        }

        return;
    }

    // The name window of a group and its delete question lie above the form
    if (group.IsBusy()) {
        group.Update(form, renderer.MouseViewportPosition(), mouse.IsLeftClicked(), font);
        return;
    }

    FormWindow::Result result = form.Update(
        renderer.MouseViewportPosition(),
        mouse.IsLeftClicked(),
        font,
        mouse.IsRightClicked()
    );

    look.Follow(form, engine.GetAssets());

    if (group.Handle(form, result, renderer.GetWidth(), renderer.GetHeight(), font)) {
        return;
    }

    if (edit && result == FormWindow::Result::FieldButton && form.EventField() == editAdvancedField) {
        CharacterBody body = editObjectBody ? *editObjectBody : BaseBodyForEdit(look.Read(form));

        advancedWindow.Open(body, renderer.GetWidth(), renderer.GetHeight(), font);

        return;
    }

    if (result != FormWindow::Result::Confirmed) {
        return;
    }

    if (edit) {
        ApplyObjectEdit();
    } else {
        CreateObjectType();
    }
}

bool FieldScene::CreateObjectType() {
    std::string name = TrimSpaces(newObjectForm.Text(static_cast<std::size_t>(NewObjectField::Name)));

    if (name.empty()) {
        newObjectForm.SetError("Name missing");
        return false;
    }

    CharacterDefinition definition;
    definition.name = name;
    definition.maxHealth = newObjectForm.Number(newObjectHealthField);
    definition.SetLook(newObjectLook.Read(newObjectForm));

    std::string file = UniqueCharacterFile(name);

    if (!definition.Save(file)) {
        newObjectForm.SetError("Could not save");
        return false;
    }

    std::string group = newObjectGroup.Read(newObjectForm);

    newObjectForm.Close();

    // The new type now also shows up in the add menu
    LoadObjectTypes();

    auto type = std::find(objectTypes.begin(), objectTypes.end(), file);

    if (type != objectTypes.end()) {
        AddObject(static_cast<std::size_t>(type - objectTypes.begin()), newObjectPosition, group);
    }

    return true;
}

void FieldScene::OpenEditObjectForm(const SpriteInstance *instance) {
    const Character *character = characters.Find(instance);

    if (character == nullptr) {
        return;
    }

    // What the character looks like now: its type with its own changes
    CharacterDefinition current = CharacterDefinition::Load(character->DefinitionFile())
                                      .WithOverrides(character->Overrides());

    editPlacement = static_cast<std::size_t>(characters.IndexOf(instance));

    editObjectForm.Clear();
    editObjectForm.SetTitle("Edit object");
    editObjectForm.SetConfirmLabel("Save");

    // Same order as EditObjectField, the look follows
    editObjectForm.AddText("Name", character->Name(), OBJECT_NAME_MAX_LENGTH);
    editObjectGroup.Add(editObjectForm, groups, character->Group());

    // What this one character can have at most, 0 for no health or no energy
    editObjectForm.AddNumber("Health", current.maxHealth, 0, OBJECT_MAX_HEALTH);
    editObjectForm.AddNumber("Energy", current.maxEnergy, 0, OBJECT_MAX_ENERGY);

    editObjectLook.Add(editObjectForm, current.Look());

    // Shadow and hitbox get their own window
    editAdvancedField = editObjectForm.AddButton("Advanced settings");
    editObjectBody = character->Overrides().body;

    Renderer &renderer = engine.GetRenderer();

    editObjectForm.Open(renderer.GetWidth(), renderer.GetHeight(), engine.GetFont());
}

// A value only belongs to the character if it differs from its type
static std::optional<int> OverrideValue(int value, int fromType) {
    return value == fromType ? std::nullopt : std::optional<int>(value);
}

bool FieldScene::ApplyObjectEdit() {
    std::string name = TrimSpaces(editObjectForm.Text(static_cast<std::size_t>(EditObjectField::Name)));

    if (name.empty()) {
        editObjectForm.SetError("Name missing");
        return false;
    }

    editObjectForm.Close();

    std::vector<ObjectPlacement> placements = CollectPlacements();

    if (editPlacement >= placements.size()) {
        return false;
    }

    ObjectPlacement &placement = placements[editPlacement];

    // Only what differs from the type is stored with the character, so later
    // changes to the type still reach everything else about it
    CharacterDefinition type = CharacterDefinition::Load(placement.character);
    CharacterLook look = editObjectLook.Read(editObjectForm);

    placement.overrides.name = name == type.name ? "" : name;

    if (look == type.Look()) {
        placement.overrides.look.reset();
    } else {
        placement.overrides.look = look;
    }

    // Advanced settings that match the type with this look are not stored
    if (editObjectBody && *editObjectBody != BaseBodyForEdit(look)) {
        placement.overrides.body = editObjectBody;
    } else {
        placement.overrides.body.reset();
    }

    placement.group = editObjectGroup.Read(editObjectForm);

    // Only values that differ from the type belong to the character
    int maxHealth = editObjectForm.Number(static_cast<std::size_t>(EditObjectField::Health));
    int maxEnergy = editObjectForm.Number(static_cast<std::size_t>(EditObjectField::Energy));

    placement.overrides.maxHealth = OverrideValue(maxHealth, type.maxHealth);
    placement.overrides.maxEnergy = OverrideValue(maxEnergy, type.maxEnergy);

    // A character that can take less than before does not keep more health
    if (placement.health > maxHealth) {
        placement.health = maxHealth;
    }

    // The character keeps its place in the list
    ApplyPlacements(placements);

    objectEditor.Select(&characters.Characters()[editPlacement].GetCharacter());

    SaveObjects();

    return true;
}

void FieldScene::DuplicateType(std::size_t type) {
    if (type >= objectTypes.size()) {
        return;
    }

    std::string name = objectTypeNames[type] + "-Copy";
    std::string file = UniqueCharacterFile(name);

    if (!CharacterDefinition::SaveCopy(objectTypes[type], file, name)) {
        TraceLog(LOG_WARNING, "SCENE: [%s] konnte nicht kopiert werden", objectTypes[type].c_str());
        return;
    }

    LoadObjectTypes();
}

void FieldScene::AskDeleteType(std::size_t type) {
    if (type >= objectTypes.size()) {
        return;
    }

    pendingDeleteType = objectTypes[type];
    pendingDeleteObject = nullptr;
    pendingSaveObject = nullptr;

    Renderer &renderer = engine.GetRenderer();

    confirmDialog.Open(
        "Delete " + objectTypeNames[type],
        "Are you sure?",
        renderer.GetWidth(),
        renderer.GetHeight(),
        engine.GetFont(),
        FontVariant::Red
    );
}

void FieldScene::AskDeleteObject(const SpriteInstance *instance) {
    const Character *character = characters.Find(instance);

    if (character == nullptr) {
        return;
    }

    pendingDeleteType.clear();
    pendingDeleteObject = instance;
    pendingSaveObject = nullptr;

    Renderer &renderer = engine.GetRenderer();

    confirmDialog.Open(
        "Delete " + character->Name(),
        "Are you sure?",
        renderer.GetWidth(),
        renderer.GetHeight(),
        engine.GetFont(),
        FontVariant::Red
    );
}

bool FieldScene::HasObjectChanges(const SpriteInstance *instance) const {
    const Character *character = characters.Find(instance);

    // Position and group belong to the placement, they say nothing about the type
    return character != nullptr && !(character->Overrides() == CharacterOverrides{});
}

void FieldScene::AskSaveObject(const SpriteInstance *instance) {
    const Character *character = characters.Find(instance);

    if (character == nullptr || !HasObjectChanges(instance)) {
        return;
    }

    pendingDeleteType.clear();
    pendingDeleteObject = nullptr;

    pendingSaveObject = instance;
    pendingSaveName = UniqueTypeName(character->Name());

    // With a name of its own the character keeps it, an existing one gets a copy
    std::string message = pendingSaveName == character->Name()
                              ? "It joins the add menu."
                              : "Name taken, saved as\n" + pendingSaveName + ".";

    Renderer &renderer = engine.GetRenderer();

    confirmDialog.Open(
        "Save " + character->Name() + " as object",
        message,
        renderer.GetWidth(),
        renderer.GetHeight(),
        engine.GetFont()
    );
}

void FieldScene::SaveObjectType(const SpriteInstance *instance, const std::string &name) {
    const Character *character = characters.Find(instance);

    if (character == nullptr) {
        return;
    }

    // The type as this one character looks right now
    CharacterDefinition definition = CharacterDefinition::Load(character->DefinitionFile())
                                         .WithOverrides(character->Overrides());

    definition.name = name;

    std::string file = UniqueCharacterFile(name);

    if (!definition.Save(file)) {
        TraceLog(LOG_WARNING, "SCENE: [%s] konnte nicht gespeichert werden", file.c_str());
        return;
    }

    int index = characters.IndexOf(instance);

    if (index < 0) {
        return;
    }

    std::vector<ObjectPlacement> placements = CollectPlacements();

    // Its changes now live in the type, so the character is a plain instance of it
    placements[static_cast<std::size_t>(index)].character = file;
    placements[static_cast<std::size_t>(index)].overrides = {};

    ApplyPlacements(placements);

    objectEditor.Select(&characters.Characters()[static_cast<std::size_t>(index)].GetCharacter());

    SaveObjects();

    // The new type now also shows up in the add menu
    LoadObjectTypes();
}

void FieldScene::UpdateConfirmDialog(Vector2 mouse, bool clicked) {
    ConfirmDialog::Result result = confirmDialog.Update(mouse, clicked, engine.GetFont());

    if (result == ConfirmDialog::Result::Yes) {
        if (pendingSaveObject != nullptr) {
            SaveObjectType(pendingSaveObject, pendingSaveName);
        } else if (pendingDeleteObject != nullptr) {
            RemoveObject(pendingDeleteObject);
        } else if (!pendingDeleteScript.empty()) {
            // Copied: LoadScripts and the scene may look at the pending state
            std::string script = pendingDeleteScript;

            bool removed = ScriptFiles::Remove(Options().id, script);

            LoadScripts();

            if (removed) {
                ScriptRemoved(script);
            }
        } else {
            DeleteType(pendingDeleteType);
        }
    }

    if (result != ConfirmDialog::Result::None) {
        pendingDeleteType.clear();
        pendingDeleteScript.clear();
        pendingDeleteObject = nullptr;
        pendingSaveObject = nullptr;
        pendingSaveName.clear();
    }
}

void FieldScene::DeleteType(const std::string &file) {
    std::vector<ObjectPlacement> placements = CollectPlacements();
    std::vector<ObjectPlacement> remaining;

    for (const ObjectPlacement &placement: placements) {
        if (placement.character != file) {
            remaining.push_back(placement);
        }
    }

    if (remaining.size() != placements.size()) {
        ApplyPlacements(remaining);
        SaveObjects();
    }

    if (!DeleteAsset(file)) {
        TraceLog(LOG_WARNING, "SCENE: [%s] konnte nicht geloescht werden", file.c_str());
    }

    LoadObjectTypes();
}

CharacterBody FieldScene::BaseBodyForEdit(const CharacterLook &look) const {
    std::vector<ObjectPlacement> placements = CollectPlacements();

    if (editPlacement >= placements.size()) {
        return {};
    }

    CharacterOverrides changes;
    changes.look = look;

    return CharacterDefinition::Load(placements[editPlacement].character).WithOverrides(changes).Body();
}

std::string FieldScene::UniqueCharacterFile(const std::string &name) const {
    std::string base = IdFromName(name, "object");

    std::string file = "characters/" + base + ".json";

    // Existing types are never overwritten
    for (int number = 2; AssetExists(file); number++) {
        file = "characters/" + base + "_" + std::to_string(number) + ".json";
    }

    return file;
}

std::string FieldScene::UniqueTypeName(const std::string &name) const {
    if (std::find(objectTypeNames.begin(), objectTypeNames.end(), name) == objectTypeNames.end()) {
        return name;
    }

    std::string candidate = name + "-Copy";

    for (int number = 2;
         std::find(objectTypeNames.begin(), objectTypeNames.end(), candidate) != objectTypeNames.end();
         number++) {
        candidate = name + "-Copy" + std::to_string(number);
    }

    return candidate;
}

void FieldScene::Exit() {
    // Leaving the scene, e.g. into the simulation, writes an open script as
    // well: its window goes away with the scene
    if (codeEditor.IsOpen()) {
        WriteScript();
        CloseScript();
    }

    if (pendingSave) {
        SaveObjects();
    }
}

void FieldScene::Update(float dt) {
    Renderer &renderer = engine.GetRenderer();

    std::vector<SpriteInstance *> all = AllSprites();
    std::vector<SpriteInstance *> selectable = SelectableSprites();

    // Over a character the mouse cursor lights up: a click selects it
    if (Collision::FindAt(renderer.MouseWorldPosition(), selectable) != nullptr) {
        engine.RequestCursor(CursorState::Hover);
    }

    // A click into empty space clears the selection
    if (engine.GetMouse().IsLeftClicked()) {
        Select(Collision::FindAt(renderer.MouseWorldPosition(), selectable));
    }

    // Reset first, then move (a blocking reports itself), finally add the
    // remaining overlaps
    Collision::ResetFlags(all);

    Character *player = Player();

    if (player != nullptr) {
        MovePlayer(*player, dt, all);
    }

    Collision::MarkOverlaps(all);

    // After the world of this frame stands, so a script sees it as it is
    scripts.Update(dt);

    if (player != nullptr && bounds.Mode() == BoundsMode::Follow) {
        renderer.SetCameraFocus(bounds.CameraFocus(
            player->GetCharacter().Position(),
            renderer.GetWidth(),
            renderer.GetHeight()
        ));
    }

    for (Character &character: characters.Characters()) {
        character.GetCharacter().Sprite().Update(dt);
    }
}

void FieldScene::UpdateDevelopment(float) {
    // While editing nothing moves on its own, so the overlaps are checked here
    // as well: characters that lie on top of each other show it with a red
    // hitbox, right while they are being dragged
    std::vector<SpriteInstance *> all = AllSprites();

    Collision::ResetFlags(all);
    Collision::MarkOverlaps(all);

    if (UpdateScriptWindows()) {
        return;
    }

    if (!EditsObjects()) {
        return;
    }

    if (newObjectForm.IsOpen()) {
        UpdateObjectForm(newObjectForm, newObjectGroup, newObjectLook, false);

        if (newObjectForm.IsHovering() || newObjectGroup.IsHovering()) {
            engine.RequestCursor(CursorState::Hover);
        }

        return;
    }

    if (editObjectForm.IsOpen()) {
        UpdateObjectForm(editObjectForm, editObjectGroup, editObjectLook, true);

        if (editObjectForm.IsHovering() || editObjectGroup.IsHovering() || advancedWindow.IsHovering()) {
            engine.RequestCursor(CursorState::Hover);
        }

        return;
    }

    Renderer &renderer = engine.GetRenderer();

    ObjectEditor::Input input;
    input.mouseWorld = renderer.MouseWorldPosition();
    input.mouseViewport = renderer.MouseViewportPosition();
    input.leftClicked = engine.GetMouse().IsLeftClicked();
    input.leftDown = engine.GetMouse().IsLeftDown();
    input.rightClicked = engine.GetMouse().IsRightClicked();
    input.keyboardBusy = engine.GetConsole().IsOpen();
    input.viewWidth = renderer.GetWidth();
    input.viewHeight = renderer.GetHeight();

    ObjectEditor::Changes changes = objectEditor.Update(AllEditables(), input, engine.GetFont());

    ApplyEditorChanges(changes);

    engine.RequestCursor(objectEditor.Cursor(AllEditables(), input.mouseWorld));
}

bool FieldScene::UpdateScriptWindows() {
    // The code editor lies above everything and owns mouse and keyboard, in
    // every scene: it is open whenever a script is being written
    if (codeEditor.IsOpen()) {
        // It lives in the pixels of the window, so it also gets the mouse in
        // them. A window that was resized changes its place.
        codeEditor.SetLayout(
            EditorArea(),
            GetScreenWidth(),
            GetScreenHeight(),
            engine.GetRenderer().GetScale()
        );

        HandleScriptRequest(
            codeEditor.Update(GetMousePosition(), engine.GetMouse().IsLeftClicked(), engine.GetFont())
        );

        if (codeEditor.IsHovering()) {
            engine.RequestCursor(CursorState::Hover);
        }

        return true;
    }

    // The window for a new or renamed script only has its name
    if (scriptForm.IsOpen()) {
        FormWindow::Result result = scriptForm.Update(
            engine.GetRenderer().MouseViewportPosition(),
            engine.GetMouse().IsLeftClicked(),
            engine.GetFont()
        );

        if (result == FormWindow::Result::Confirmed) {
            ApplyScriptForm();
        }

        if (scriptForm.IsHovering()) {
            engine.RequestCursor(CursorState::Hover);
        }

        return true;
    }

    // It also asks about objects and types, see UpdateConfirmDialog
    if (confirmDialog.IsOpen()) {
        UpdateConfirmDialog(engine.GetRenderer().MouseViewportPosition(), engine.GetMouse().IsLeftClicked());

        if (confirmDialog.IsHovering()) {
            engine.RequestCursor(CursorState::Hover);
        }

        return true;
    }

    return false;
}

void FieldScene::ApplyEditorChanges(const ObjectEditor::Changes &changes) {
    if (changes.moved) {
        pendingSave = true;
    }

    if (changes.undoRequested || changes.redoRequested) {
        // Store a pending move as its own step first
        if (pendingSave) {
            SaveObjects();
        }

        if (changes.undoRequested) {
            UndoObjects();
        } else {
            RedoObjects();
        }

        return;
    }

    SpriteInstance *selectedObject = SelectedSprite();

    if (selectedObject != nullptr && changes.deleteRequested) {
        AskDeleteObject(selectedObject);
    }

    if (changes.addSection == static_cast<int>(AddSection::Objects) && changes.addType >= 0) {
        if (static_cast<std::size_t>(changes.addType) == objectTypes.size()) {
            OpenNewObjectForm(changes.addPosition);
        } else {
            AddObject(static_cast<std::size_t>(changes.addType), changes.addPosition);
        }
    }

    // A script opens in the editor, the extra item creates a new one
    if (changes.addSection == static_cast<int>(AddSection::Scripts) && changes.addType >= 0) {
        if (static_cast<std::size_t>(changes.addType) == scriptNames.size()) {
            OpenScriptForm("");
        } else {
            OpenScript(scriptNames[static_cast<std::size_t>(changes.addType)]);
        }
    }

    if (changes.objectAction >= 0 && changes.actionTarget != nullptr) {
        const SpriteInstance *target = static_cast<const SpriteInstance *>(changes.actionTarget);

        switch (static_cast<ObjectAction>(changes.objectAction)) {
            case ObjectAction::Edit:
                OpenEditObjectForm(target);
                break;

            case ObjectAction::Duplicate:
                DuplicateObject(target);
                break;

            case ObjectAction::Save:
                AskSaveObject(target);
                break;

            case ObjectAction::Delete:
                AskDeleteObject(target);
                break;
        }
    }

    if (changes.typeAction >= 0 && changes.actionSection == static_cast<int>(AddSection::Scripts) &&
        changes.typeIndex < scriptNames.size()) {
        const std::string &script = scriptNames[changes.typeIndex];

        switch (static_cast<ScriptAction>(changes.typeAction)) {
            case ScriptAction::Rename:
                OpenScriptForm(script);
                break;

            case ScriptAction::Delete:
                AskDeleteScript(script);
                break;
        }
    }

    if (changes.typeAction >= 0 && changes.actionSection == static_cast<int>(AddSection::Objects) &&
        changes.typeIndex < objectTypes.size()) {
        switch (static_cast<TypeAction>(changes.typeAction)) {
            case TypeAction::Duplicate:
                DuplicateType(changes.typeIndex);
                break;

            case TypeAction::Delete:
                AskDeleteType(changes.typeIndex);
                break;
        }
    }

    // While holding, not every pixel gets written individually, but only once
    // arrow keys and mouse are released again. The whole movement is therefore
    // one step in the history.
    if (pendingSave && !ObjectEditor::IsMoveKeyDown() && !objectEditor.IsDragging()) {
        SaveObjects();
    }
}

void FieldScene::Draw() {
    Renderer &renderer = engine.GetRenderer();

    DrawGround();

    for (Character &character: characters.Characters()) {
        renderer.Submit(character.GetCharacter());

        // The energy bar goes first, so it ends up below the health bar.
        // Indestructible characters have no health bar, characters without
        // energy no blue one.
        if (character.HasEnergy()) {
            renderer.SubmitEnergyBar(character.GetCharacter(), character.EnergyFraction(), character.MaxEnergy());
        }

        if (!character.IsIndestructible()) {
            renderer.SubmitHealthBar(character.GetCharacter(), character.HealthFraction(), character.MaxHealth());
        }
    }

    renderer.BeginOverlay();

    DrawOverlay();

    if (EditsObjects()) {
        objectEditor.Draw(engine.GetFont(), SelectedCaption());
    }
}

void FieldScene::DrawUI() {
    const FontRenderer &font = engine.GetFont();

    // In the Development mode the active tab shows the name. Otherwise it is shown
    // at the top left, as in the catalog, e.g. also for a copy.
    if (!engine.ShowsSceneTabs()) {
        const SceneEntry *entry = engine.GetScenes().Find(Options().id);

        font.Draw(entry != nullptr ? entry->name : name, {8, 8}, engine.GetTheme().textVariant, TextSpacing::Narrow);
    }

    // While playing, a click shows what it hit. The editor has its own label
    // for that, see ObjectEditor::SetObjectTitle.
    if (!EditsObjects() && engine.GetConfig().HasDebugTools()) {
        std::string caption = CaptionFor(selected);

        if (!caption.empty()) {
            float row = static_cast<float>(font.LetterHeight()) + 3.0f;

            font.Draw(caption, {8, 8 + row}, engine.GetTheme().textVariant, TextSpacing::Narrow);
        }
    }

    if (EditsObjects()) {
        if (engine.ShowsEditorHelp()) {
            objectEditor.DrawHelp(font, engine.GetRenderer().GetHeight(), "");
        }

        DrawEditStatus(engine, objectHistory.IsModified(), objectHistory.CanUndo(), objectHistory.CanRedo());

        // Last, so context menu and forms lie above everything, the group
        // windows above their form
        objectEditor.DrawMenu(font);
        newObjectForm.Draw(font);
        newObjectGroup.Draw(font);
        editObjectForm.Draw(font);
        editObjectGroup.Draw(font);
        advancedWindow.Draw(font);
    }

    // Also in scenes without objects of their own, see UpdateScriptWindows.
    // Closed windows draw nothing.
    scriptForm.Draw(font);
    confirmDialog.Draw(font);

}

// Above everything and in the pixels of the window, see Scene::DrawScreen
void FieldScene::DrawScreen() {
    codeEditor.Draw(engine.GetFont());
}

bool FieldScene::DrawsScreenLayer() const {
    return codeEditor.IsOpen();
}
