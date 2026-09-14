#pragma once

#include <cstddef>
#include <utility>
#include <vector>

// Undo history made of snapshots, for developer tools.
//
// After every change the whole state is stored, undo and redo jump back
// and forth in this list. A new change after an undo discards the steps
// that came after it.
//
// Storing whole states needs more memory than only remembering the changes.
// In return every step is consistent on its own, even after deleting or
// adding, and no separate inverse is needed per action. For a few dozen
// objects that is a few kilobytes.
//
// State needs operator==. The history only lives in memory.
template <typename State>
class EditHistory {
public:
    explicit EditHistory(std::size_t limit = 100);

    // New starting point, e.g. after loading. The history starts over.
    void Reset(State initial);

    // Stores a new state. If it equals the current one, nothing happens.
    void Push(State state);

    bool CanUndo() const;

    bool CanRedo() const;

    // The state that was jumped back to, otherwise nullptr. The pointer is valid
    // until the next Push.
    const State *Undo();

    const State *Redo();

    // Does the current state differ from the starting point?
    bool IsModified() const;

private:
    std::vector<State> states;

    std::size_t index = 0;
    std::size_t limit;

    State original{};
};

template <typename State>
EditHistory<State>::EditHistory(std::size_t limit)
    : limit(limit) {
}

template <typename State>
void EditHistory<State>::Reset(State initial) {
    original = initial;

    states.clear();
    states.push_back(std::move(initial));

    index = 0;
}

template <typename State>
void EditHistory<State>::Push(State state) {
    if (states.empty()) {
        Reset(std::move(state));
        return;
    }

    if (states[index] == state) {
        return;
    }

    states.erase(states.begin() + static_cast<std::ptrdiff_t>(index) + 1, states.end());
    states.push_back(std::move(state));

    index = states.size() - 1;

    // The oldest steps get dropped. The starting point for IsModified
    // is kept anyway.
    if (states.size() > limit + 1) {
        states.erase(states.begin());
        index--;
    }
}

template <typename State>
bool EditHistory<State>::CanUndo() const {
    return index > 0;
}

template <typename State>
bool EditHistory<State>::CanRedo() const {
    return index + 1 < states.size();
}

template <typename State>
const State *EditHistory<State>::Undo() {
    if (!CanUndo()) {
        return nullptr;
    }

    index--;

    return &states[index];
}

template <typename State>
const State *EditHistory<State>::Redo() {
    if (!CanRedo()) {
        return nullptr;
    }

    index++;

    return &states[index];
}

template <typename State>
bool EditHistory<State>::IsModified() const {
    return !states.empty() && !(states[index] == original);
}
