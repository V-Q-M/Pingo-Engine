#pragma once

#include <cstddef>
#include <string>

#include "raylib.h"

#include "ObjectGroups.h"
#include "Engine/ConfirmDialog.h"
#include "Engine/FontRenderer.h"
#include "Engine/FormWindow.h"

// The group of a character as a dropdown in a FormWindow, e.g. in the windows
// for new and edited objects.
//
// The list ends with "+New", which asks for a name in a small window and
// creates the group. The list stays open, so the new group can be chosen right
// away. A right click on a group renames or deletes it. ally and enemy allow
// neither, and a group can only be deleted once no character in any scene
// belongs to it anymore.
//
// Changes to the groups are saved right away.
class GroupField {
public:
    // Same order as in the menu a right click on a group opens
    enum class Action {
        Rename,
        Delete
    };

    // Appends the dropdown to the form with the group groupId chosen. groups
    // must live as long as the form.
    void Add(FormWindow &form, ObjectGroups &groups, const std::string &groupId);

    // Is the name window or the delete question open? Then Update gets the
    // input instead of the form.
    bool IsBusy() const;

    // Handles the name window or the delete question, the mouse in viewport
    // coordinates
    void Update(FormWindow &form, Vector2 mouse, bool clicked, const FontRenderer &font);

    // Call with every result of FormWindow::Update. true if the result belonged
    // to the group dropdown.
    bool Handle(const FormWindow &form,
                FormWindow::Result result,
                int viewWidth,
                int viewHeight,
                const FontRenderer &font);

    bool IsHovering() const;

    // Closes the name window or the delete question. true if one was open.
    bool CloseWindow();

    // Belongs above the form
    void Draw(const FontRenderer &font) const;

    // The id of the chosen group
    std::string Read(const FormWindow &form) const;

private:
    // What the name window is for
    enum class Pending {
        None,
        Create,
        Rename
    };

    // Rebuilds the options from the groups and chooses the group with this id,
    // enemy if it is gone
    void RefreshOptions(FormWindow &form, const std::string &groupId) const;

    void OpenNameWindow(const std::string &title,
                        const std::string &confirmLabel,
                        const std::string &name,
                        int viewWidth,
                        int viewHeight,
                        const FontRenderer &font);

    ObjectGroups *groups = nullptr;

    std::size_t field = 0;

    FormWindow nameWindow;

    ConfirmDialog deleteDialog;

    Pending pending = Pending::None;

    // The group the name window or the delete question is about
    std::string targetId;
};
