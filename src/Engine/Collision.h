#pragma once

#include <vector>

#include "SpriteInstance.h"

class Collision {
public:
    static bool Overlaps(const SpriteInstance &a, const SpriteInstance &b);

    // Clears the collision flag of all characters. Belongs at the start
    // of the frame, before any movement.
    static void ResetFlags(const std::vector<SpriteInstance *> &instances);

    // Marks every pair that overlaps. Deliberately resets nothing, so
    // that a blocking reported earlier is kept.
    static void MarkOverlaps(const std::vector<SpriteInstance *> &instances);

    // The character under a point, otherwise nullptr. If several overlap,
    // the frontmost one wins: the one that is also drawn on top.
    static SpriteInstance *FindAt(Vector2 point,
                                  const std::vector<SpriteInstance *> &instances);

    // Moves the character by delta, but does not let it walk into a solid
    // character. It is only blocked if both sides are solid.
    // Being blocked counts as a collision and marks both involved.
    static void MoveWithCollision(SpriteInstance &mover,
                                  Vector2 delta,
                                  const std::vector<SpriteInstance *> &others);

private:
    // The solid character currently in mover's way, otherwise nullptr
    static SpriteInstance *FindBlocker(const SpriteInstance &mover,
                                       const std::vector<SpriteInstance *> &others);
};
