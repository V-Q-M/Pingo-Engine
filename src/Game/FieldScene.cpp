#include "FieldScene.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <system_error>
#include <utility>

#include "SceneObjects.h"
#include "Engine/AssetFile.h"
#include "Engine/Collision.h"
#include "Engine/EditStatus.h"
#include "Engine/Engine.h"

// How far a duplicated character lands next to the original
constexpr float DUPLICATE_OFFSET = 10.0f;

// Font variant of the add menu item that creates a new type
constexpr int NEW_TYPE_VARIANT = FontVariant::Grey;

// Colors for dummy objects, from the game's palette
constexpr std::array<Color, 8> DUMMY_COLORS{{
    {255, 44, 34, 255},
    {255, 229, 26, 255},
    {61, 255, 20, 255},
    {0, 249, 255, 255},
    {41, 110, 219, 255},
    {153, 113, 84, 255},
    {128, 128, 128, 255},
    {255, 255, 255, 255}
}};

FieldScene::FieldScene(Engine &engine, SceneOptions options, std::string name)
    : Scene(engine, std::move(options)),
      name(std::move(name)),
      selectorTexture(engine.GetAssets().Texture().Get("sprites/selector.png")),
      bounds({0.0f, 0.0f, WORLD_WIDTH, WORLD_HEIGHT}, BOUNDS_MODE) {
}

void FieldScene::LoadObjects(const std::string &filename) {
    objectsFile = filename;

    std::vector<ObjectPlacement> placements;
    objectsWritable = SceneObjects::Load(filename, placements);

    ApplyPlacements(placements);

    if (engine.GetConfig().IsDevelopment()) {
        // The last item in the add menu creates a new type
        objectEditor.SetAddExtra("+New", NEW_TYPE_VARIANT);
        objectEditor.SetTypeActions(
            {"Duplicate", "Delete"},
            {FontRenderer::ICON_DUPLICATE, FontRenderer::ICON_DELETE}
        );

        LoadObjectTypes();

        objectEditor.SetObjectActions(
            {"Duplicate", "Swap Team", "Delete"},
            {FontRenderer::ICON_DUPLICATE, 0, FontRenderer::ICON_DELETE}
        );

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

    heroes.Clear();
    enemies.Clear();

    Assets &assets = engine.GetAssets();

    for (const ObjectPlacement &placement: placements) {
        Team &team = placement.team == "heroes" ? heroes : enemies;

        Character &character = team.Add(assets, placement.character, placement.position);

        if (placement.health >= 0) {
            character.SetHealth(placement.health);
        }
    }
}

void FieldScene::LoadObjectTypes() {
    objectTypes.clear();

    std::error_code error;

    for (const auto &entry: std::filesystem::directory_iterator("assets/characters", error)) {
        if (entry.path().extension() == ".json") {
            objectTypes.push_back("characters/" + entry.path().filename().string());
        }
    }

    std::sort(objectTypes.begin(), objectTypes.end());

    objectTypeNames.clear();

    for (const std::string &file: objectTypes) {
        objectTypeNames.push_back(CharacterDefinition::Load(file).name);
    }

    objectEditor.SetTypeNames(objectTypeNames);
}

std::vector<ObjectPlacement> FieldScene::CollectPlacements() const {
    std::vector<ObjectPlacement> placements;

    auto collect = [&](const Team &team, const std::string &teamName) {
        for (const Character &character: team.Monsters()) {
            ObjectPlacement placement;

            placement.character = character.DefinitionFile();
            placement.team = teamName;
            placement.position = character.GetCharacter().Position();

            // Full health does not need to be in the file
            if (character.Health() != character.MaxHealth()) {
                placement.health = character.Health();
            }

            placements.push_back(placement);
        }
    };

    collect(heroes, "heroes");
    collect(enemies, "enemies");

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

    // Remember the selection by team and slot, because after the rebuild the old
    // pointers are no longer valid. If a move is undone, the same character stays
    // selected.
    SpriteInstance *previous = SelectedSprite();
    Team *team = TeamOf(previous);
    std::size_t slot = 0;

    if (team != nullptr) {
        while (&team->Monsters()[slot].GetCharacter() != previous) {
            slot++;
        }
    }

    std::vector<ObjectPlacement> placements = *state;

    ApplyPlacements(placements);

    if (team != nullptr && slot < team->Monsters().size()) {
        objectEditor.Select(&team->Monsters()[slot].GetCharacter());
    }

    WritePlacements(placements);
}

bool FieldScene::EditsObjects() const {
    return engine.GetConfig().IsDevelopment() && !objectsFile.empty();
}

Team *FieldScene::TeamOf(const SpriteInstance *instance) {
    if (heroes.Find(instance) != nullptr) {
        return &heroes;
    }

    if (enemies.Find(instance) != nullptr) {
        return &enemies;
    }

    return nullptr;
}

void FieldScene::AddObject(std::size_t type, Vector2 position) {
    if (type >= objectTypes.size()) {
        return;
    }

    // The list may move when adding, old pointers are no longer valid then
    Select(nullptr);
    objectEditor.Select(nullptr);

    Character &added = enemies.Add(engine.GetAssets(), objectTypes[type], Renderer::SnapToPixel(position));

    objectEditor.Select(&added.GetCharacter());

    SaveObjects();
}

void FieldScene::RemoveObject(const SpriteInstance *instance) {
    Team *team = TeamOf(instance);

    if (team == nullptr) {
        return;
    }

    Select(nullptr);
    objectEditor.Select(nullptr);

    team->Remove(instance);

    SaveObjects();
}

void FieldScene::SwitchTeam(const SpriteInstance *instance) {
    Team *from = TeamOf(instance);

    if (from == nullptr) {
        return;
    }

    Team &to = from == &heroes ? enemies : heroes;

    // Copy first: after removing, the pointer is no longer valid
    Character moved = *from->Find(instance);

    Select(nullptr);
    objectEditor.Select(nullptr);

    from->Remove(instance);

    Character &added = to.Add(std::move(moved));

    objectEditor.Select(&added.GetCharacter());

    SaveObjects();
}

void FieldScene::DuplicateObject(const SpriteInstance *instance) {
    Team *team = TeamOf(instance);

    if (team == nullptr) {
        return;
    }

    // Copy first: the list may move when adding
    Character copy = *team->Find(instance);

    Vector2 position = copy.GetCharacter().Position();

    copy.GetCharacter().SetPosition({position.x + DUPLICATE_OFFSET, position.y + DUPLICATE_OFFSET});
    copy.GetCharacter().SetSelector(nullptr);

    Select(nullptr);
    objectEditor.Select(nullptr);

    Character &added = team->Add(std::move(copy));

    objectEditor.Select(&added.GetCharacter());

    SaveObjects();
}

std::string FieldScene::SelectedCaption() {
    return CaptionFor(SelectedSprite());
}

std::string FieldScene::CaptionFor(const SpriteInstance *instance) {
    Team *team = TeamOf(instance);

    if (instance == nullptr || team == nullptr) {
        return "";
    }

    return team->Find(instance)->Name() + " " + (team == &heroes ? "Hero" : "Enemy");
}

void FieldScene::SetPlayer(std::size_t heroIndex) {
    playerIndex = heroIndex;
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

Character *FieldScene::Player() {
    if (playerIndex >= heroes.Monsters().size()) {
        return nullptr;
    }

    return &heroes.Monsters()[playerIndex];
}

std::vector<EditableObject *> FieldScene::AllEditables() {
    std::vector<SpriteInstance *> sprites = AllSprites();

    return std::vector<EditableObject *>(sprites.begin(), sprites.end());
}

SpriteInstance *FieldScene::SelectedSprite() const {
    return static_cast<SpriteInstance *>(objectEditor.Selected());
}

std::vector<SpriteInstance *> FieldScene::AllSprites() {
    std::vector<SpriteInstance *> all;

    for (Character &monster: heroes.Monsters()) {
        all.push_back(&monster.GetCharacter());
    }

    for (Character &monster: enemies.Monsters()) {
        all.push_back(&monster.GetCharacter());
    }

    return all;
}

void FieldScene::Select(SpriteInstance *instance) {
    if (selected != nullptr) {
        selected->SetSelector(nullptr);
    }

    selected = instance;

    if (selected != nullptr) {
        selected->SetSelector(&selectorTexture);
    }
}

Rectangle FieldScene::WorldArea() const {
    return bounds.Area();
}

// With a selection the arrow keys move the character. With a context menu
// open they rest, so the world does not move away under the menu.
bool FieldScene::UsesArrowKeys() const {
    return EditsObjects() &&
           (objectEditor.Selected() != nullptr || objectEditor.IsMenuOpen() || newObjectForm.IsOpen() ||
            confirmDialog.IsOpen());
}

bool FieldScene::CapturesKeyboard() const {
    return EditsObjects() && newObjectForm.IsOpen();
}

bool FieldScene::OnEscape() {
    if (!EditsObjects()) {
        return false;
    }

    if (confirmDialog.IsOpen()) {
        confirmDialog.Close();
        pendingDeleteType.clear();
        pendingDeleteObject = nullptr;
        return true;
    }

    if (newObjectForm.IsOpen()) {
        newObjectForm.Close();
        return true;
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

    // Same order as NewObjectField
    newObjectForm.AddText("Name", "", 12);
    newObjectForm.AddNumber("Size", 24, 4, 64);
    newObjectForm.AddNumber("Health", 100, 0, 9999);
    newObjectForm.AddColor("Color", std::vector<Color>(DUMMY_COLORS.begin(), DUMMY_COLORS.end()), 0);

    Renderer &renderer = engine.GetRenderer();

    newObjectForm.Open(renderer.GetWidth(), renderer.GetHeight(), engine.GetFont());
}

void FieldScene::UpdateNewObjectForm() {
    FormWindow::Result result = newObjectForm.Update(
        engine.GetRenderer().MouseViewportPosition(),
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT),
        engine.GetFont()
    );

    if (result == FormWindow::Result::Confirmed) {
        CreateDummyObject();
    }
}

bool FieldScene::CreateDummyObject() {
    std::string name = newObjectForm.Text(static_cast<std::size_t>(NewObjectField::Name));

    // Spaces at the edges do not count
    name.erase(0, name.find_first_not_of(' '));
    name.erase(name.find_last_not_of(' ') + 1);

    if (name.empty()) {
        newObjectForm.SetError("Name missing");
        return false;
    }

    CharacterDefinition definition = CharacterDefinition::Dummy(
        name,
        newObjectForm.Number(static_cast<std::size_t>(NewObjectField::Size)),
        newObjectForm.ColorValue(static_cast<std::size_t>(NewObjectField::Color)),
        newObjectForm.Number(static_cast<std::size_t>(NewObjectField::Health))
    );

    std::string file = UniqueCharacterFile(name);

    if (!definition.SaveDummy(file)) {
        newObjectForm.SetError("Could not save");
        return false;
    }

    newObjectForm.Close();

    // The new type now also shows up in the add menu
    LoadObjectTypes();

    auto type = std::find(objectTypes.begin(), objectTypes.end(), file);

    if (type != objectTypes.end()) {
        AddObject(static_cast<std::size_t>(type - objectTypes.begin()), newObjectPosition);
    }

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
    Team *team = TeamOf(instance);

    if (team == nullptr) {
        return;
    }

    pendingDeleteType.clear();
    pendingDeleteObject = instance;

    Renderer &renderer = engine.GetRenderer();

    confirmDialog.Open(
        "Delete " + team->Find(instance)->Name(),
        "Are you sure?",
        renderer.GetWidth(),
        renderer.GetHeight(),
        engine.GetFont(),
        FontVariant::Red
    );
}

void FieldScene::UpdateConfirmDialog(Vector2 mouse, bool clicked) {
    ConfirmDialog::Result result = confirmDialog.Update(mouse, clicked, engine.GetFont());

    if (result == ConfirmDialog::Result::Yes && pendingDeleteObject != nullptr) {
        RemoveObject(pendingDeleteObject);
    } else if (result == ConfirmDialog::Result::Yes) {
        DeleteType(pendingDeleteType);
    }

    if (result != ConfirmDialog::Result::None) {
        pendingDeleteType.clear();
        pendingDeleteObject = nullptr;
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

std::string FieldScene::UniqueCharacterFile(const std::string &name) const {
    std::string base;

    for (char character: name) {
        if (character >= 'A' && character <= 'Z') {
            base += static_cast<char>(character - 'A' + 'a');
        } else if ((character >= 'a' && character <= 'z') || (character >= '0' && character <= '9')) {
            base += character;
        } else if ((character == ' ' || character == '-') && !base.empty() && base.back() != '_') {
            base += '_';
        }
    }

    while (!base.empty() && base.back() == '_') {
        base.pop_back();
    }

    if (base.empty()) {
        base = "object";
    }

    std::string file = "characters/" + base + ".json";

    // Existing types are never overwritten
    for (int number = 2; AssetExists(file); number++) {
        file = "characters/" + base + "_" + std::to_string(number) + ".json";
    }

    return file;
}

void FieldScene::Exit() {
    if (pendingSave) {
        SaveObjects();
    }
}

void FieldScene::Update(float dt) {
    Renderer &renderer = engine.GetRenderer();

    std::vector<SpriteInstance *> all = AllSprites();

    // Over a character the mouse cursor lights up: a click selects it
    if (ObjectEditor::FindAt(renderer.MouseWorldPosition(), AllEditables()) != nullptr) {
        engine.RequestCursor(CursorState::Hover);
    }

    // A click into empty space clears the selection
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Select(Collision::FindAt(renderer.MouseWorldPosition(), all));
    }

    // Reset first, then move (a blocking reports itself), finally add the
    // remaining overlaps
    Collision::ResetFlags(all);

    Character *player = Player();

    if (player != nullptr) {
        MovePlayer(*player, dt, all);
    }

    Collision::MarkOverlaps(all);

    if (player != nullptr && bounds.Mode() == BoundsMode::Follow) {
        renderer.SetCameraFocus(bounds.CameraFocus(
            player->GetCharacter().Position(),
            renderer.GetWidth(),
            renderer.GetHeight()
        ));
    }

    for (Character &monster: heroes.Monsters()) {
        monster.GetCharacter().Sprite().Update(dt);
    }

    for (Character &monster: enemies.Monsters()) {
        monster.GetCharacter().Sprite().Update(dt);
    }
}

void FieldScene::UpdateDevelopment(float) {
    if (!EditsObjects()) {
        return;
    }

    if (newObjectForm.IsOpen()) {
        UpdateNewObjectForm();

        if (newObjectForm.IsHovering()) {
            engine.RequestCursor(CursorState::Hover);
        }

        return;
    }

    if (confirmDialog.IsOpen()) {
        UpdateConfirmDialog(engine.GetRenderer().MouseViewportPosition(), IsMouseButtonPressed(MOUSE_BUTTON_LEFT));

        if (confirmDialog.IsHovering()) {
            engine.RequestCursor(CursorState::Hover);
        }

        return;
    }

    Renderer &renderer = engine.GetRenderer();

    ObjectEditor::Input input;
    input.mouseWorld = renderer.MouseWorldPosition();
    input.mouseViewport = renderer.MouseViewportPosition();
    input.leftClicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    input.leftDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    input.rightClicked = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
    input.viewWidth = renderer.GetWidth();
    input.viewHeight = renderer.GetHeight();

    ObjectEditor::Changes changes = objectEditor.Update(AllEditables(), input, engine.GetFont());

    ApplyEditorChanges(changes);

    engine.RequestCursor(objectEditor.Cursor(AllEditables(), input.mouseWorld));
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

    if (changes.addType >= 0 && static_cast<std::size_t>(changes.addType) == objectTypes.size()) {
        OpenNewObjectForm(changes.addPosition);
    } else if (changes.addType >= 0) {
        AddObject(static_cast<std::size_t>(changes.addType), changes.addPosition);
    }

    if (changes.objectAction >= 0 && changes.actionTarget != nullptr) {
        const SpriteInstance *target = static_cast<const SpriteInstance *>(changes.actionTarget);

        switch (static_cast<ObjectAction>(changes.objectAction)) {
            case ObjectAction::Duplicate:
                DuplicateObject(target);
                break;

            case ObjectAction::SwapTeam:
                SwitchTeam(target);
                break;

            case ObjectAction::Delete:
                AskDeleteObject(target);
                break;
        }
    }

    if (changes.typeAction >= 0 && changes.typeIndex < objectTypes.size()) {
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

    for (Character &monster: heroes.Monsters()) {
        renderer.Submit(monster.GetCharacter());
        // Indestructible characters have no health bar
        if (!monster.IsIndestructible()) {
            renderer.SubmitHealthBar(monster.GetCharacter(), monster.HealthFraction());
        }
    }

    for (Character &monster: enemies.Monsters()) {
        renderer.Submit(monster.GetCharacter());
        // Indestructible characters have no health bar
        if (!monster.IsIndestructible()) {
            renderer.SubmitHealthBar(monster.GetCharacter(), monster.HealthFraction());
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

    if (EditsObjects()) {
        if (engine.ShowsEditorHelp()) {
            objectEditor.DrawHelp(font, engine.GetRenderer().GetHeight(), "");
        }

        DrawEditStatus(engine, objectHistory.IsModified(), objectHistory.CanUndo(), objectHistory.CanRedo());

        // Last, so context menu and form lie above everything
        objectEditor.DrawMenu(font);
        newObjectForm.Draw(font);
        confirmDialog.Draw(font);
    }
}
