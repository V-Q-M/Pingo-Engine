#include "GroupField.h"

#include <vector>

#include "SceneObjects.h"
#include "Engine/Names.h"

constexpr std::size_t GROUP_NAME_MAX_LENGTH = 12;

// The last item in the list creates a group
constexpr const char *NEW_GROUP_LABEL = "+New";

void GroupField::Add(FormWindow &form, ObjectGroups &groups, const std::string &groupId) {
    this->groups = &groups;

    pending = Pending::None;
    nameWindow.Close();
    deleteDialog.Close();

    field = form.AddDropdown("Group", {}, 0);

    RefreshOptions(form, groupId);

    form.SetDropdownExtra(field, NEW_GROUP_LABEL);
    form.SetOptionActions(field, {"Rename", "Delete"}, {FontRenderer::ICON_EDIT, FontRenderer::ICON_DELETE});

    form.SetOptionActionFilter(field, [this](std::size_t option, std::size_t action) {
        const std::string &id = this->groups->All().at(option).id;

        if (ObjectGroups::IsDefault(id)) {
            return false;
        }

        // As long as a character in any scene belongs to the group, it stays
        if (static_cast<Action>(action) == Action::Delete) {
            return SceneObjects::UsedGroups().count(id) == 0;
        }

        return true;
    });
}

bool GroupField::IsBusy() const {
    return nameWindow.IsOpen() || deleteDialog.IsOpen();
}

void GroupField::Update(FormWindow &form, Vector2 mouse, bool clicked, const FontRenderer &font) {
    if (deleteDialog.IsOpen()) {
        if (deleteDialog.Update(mouse, clicked, font) == ConfirmDialog::Result::Yes) {
            std::string chosen = Read(form);

            groups->Remove(targetId);
            groups->Save();

            RefreshOptions(form, chosen);
        }

        return;
    }

    if (!nameWindow.IsOpen()) {
        return;
    }

    FormWindow::Result result = nameWindow.Update(mouse, clicked, font);

    if (result == FormWindow::Result::Cancelled) {
        pending = Pending::None;
        return;
    }

    if (result != FormWindow::Result::Confirmed) {
        return;
    }

    std::string name = TrimSpaces(nameWindow.Text(0));

    if (name.empty()) {
        nameWindow.SetError("Name missing");
        return;
    }

    if (groups->IsNameTaken(name, pending == Pending::Rename ? targetId : "")) {
        nameWindow.SetError("Name taken");
        return;
    }

    // The chosen group stays chosen, the new one is only offered
    std::string chosen = Read(form);

    if (pending == Pending::Create) {
        groups->Create(name);
    } else {
        groups->Rename(targetId, name);
    }

    if (!groups->Save()) {
        nameWindow.SetError("Could not save");
        return;
    }

    nameWindow.Close();
    pending = Pending::None;

    RefreshOptions(form, chosen);
}

bool GroupField::Handle(const FormWindow &form,
                        FormWindow::Result result,
                        int viewWidth,
                        int viewHeight,
                        const FontRenderer &font) {
    bool listEvent = result == FormWindow::Result::ListExtra || result == FormWindow::Result::OptionAction;

    if (!listEvent || form.EventField() != field) {
        return false;
    }

    if (result == FormWindow::Result::ListExtra) {
        pending = Pending::Create;
        targetId.clear();

        OpenNameWindow("New group", "Create", "", viewWidth, viewHeight, font);

        return true;
    }

    const ObjectGroup &group = groups->All().at(form.EventOption());

    targetId = group.id;

    switch (static_cast<Action>(form.EventAction())) {
        case Action::Rename:
            pending = Pending::Rename;
            OpenNameWindow("Rename group", "Save", group.name, viewWidth, viewHeight, font);
            break;

        case Action::Delete:
            pending = Pending::None;
            deleteDialog.Open("Delete " + group.name, "Are you sure?", viewWidth, viewHeight, font, FontVariant::Red);
            break;
    }

    return true;
}

bool GroupField::IsHovering() const {
    return nameWindow.IsHovering() || deleteDialog.IsHovering();
}

bool GroupField::CloseWindow() {
    if (deleteDialog.IsOpen()) {
        deleteDialog.Close();
        return true;
    }

    if (nameWindow.IsOpen()) {
        nameWindow.Close();
        pending = Pending::None;
        return true;
    }

    return false;
}

void GroupField::Draw(const FontRenderer &font) const {
    nameWindow.Draw(font);
    deleteDialog.Draw(font);
}

std::string GroupField::Read(const FormWindow &form) const {
    std::size_t choice = form.Choice(field);

    if (groups == nullptr || choice >= groups->All().size()) {
        return ObjectGroups::ENEMY;
    }

    return groups->All()[choice].id;
}

void GroupField::RefreshOptions(FormWindow &form, const std::string &groupId) const {
    std::vector<std::string> names;

    for (const ObjectGroup &group: groups->All()) {
        names.push_back(group.name);
    }

    int index = groups->IndexOf(groupId);

    if (index < 0) {
        index = groups->IndexOf(ObjectGroups::ENEMY);
    }

    form.SetOptions(field, names, static_cast<std::size_t>(index));
}

void GroupField::OpenNameWindow(const std::string &title,
                                const std::string &confirmLabel,
                                const std::string &name,
                                int viewWidth,
                                int viewHeight,
                                const FontRenderer &font) {
    nameWindow.Clear();
    nameWindow.SetTitle(title);
    nameWindow.SetConfirmLabel(confirmLabel);
    nameWindow.AddText("Name", name, GROUP_NAME_MAX_LENGTH);
    nameWindow.Open(viewWidth, viewHeight, font);
}
