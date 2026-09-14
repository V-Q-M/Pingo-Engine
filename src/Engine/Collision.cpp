#include "Collision.h"

bool Collision::Overlaps(const SpriteInstance &a, const SpriteInstance &b) {
    return CheckCollisionRecs(a.Bounds(), b.Bounds());
}

void Collision::ResetFlags(const std::vector<SpriteInstance *> &instances) {
    for (SpriteInstance *instance: instances) {
        instance->SetColliding(false);
    }
}

void Collision::MarkOverlaps(const std::vector<SpriteInstance *> &instances) {
    // Check every pair exactly once: j starts after i
    for (std::size_t i = 0; i < instances.size(); i++) {
        for (std::size_t j = i + 1; j < instances.size(); j++) {
            if (!Overlaps(*instances[i], *instances[j])) {
                continue;
            }

            instances[i]->SetColliding(true);
            instances[j]->SetColliding(true);
        }
    }
}

SpriteInstance *Collision::FindAt(Vector2 point,
                                  const std::vector<SpriteInstance *> &instances) {
    SpriteInstance *found = nullptr;

    for (SpriteInstance *instance: instances) {
        if (!CheckCollisionPointRec(point, instance->Bounds())) {
            continue;
        }

        if (found == nullptr || instance->Z() >= found->Z()) {
            found = instance;
        }
    }

    return found;
}

SpriteInstance *Collision::FindBlocker(const SpriteInstance &mover,
                                       const std::vector<SpriteInstance *> &others) {
    if (!mover.IsSolid()) {
        return nullptr;
    }

    for (SpriteInstance *other: others) {
        // The character itself is part of the list and must not
        // block itself
        if (other == &mover || !other->IsSolid()) {
            continue;
        }

        if (Overlaps(mover, *other)) {
            return other;
        }
    }

    return nullptr;
}

void Collision::MoveWithCollision(SpriteInstance &mover,
                                  Vector2 delta,
                                  const std::vector<SpriteInstance *> &others) {
    // Check the axes one after another: if one direction is blocked, the
    // other one stays usable. This way you slide along an obstacle
    // instead of getting stuck in front of it.
    Vector2 start = mover.Position();

    mover.SetPosition({start.x + delta.x, start.y});

    if (SpriteInstance *blocker = FindBlocker(mover, others)) {
        mover.SetPosition(start);

        mover.SetColliding(true);
        blocker->SetColliding(true);
    }

    Vector2 afterX = mover.Position();

    mover.SetPosition({afterX.x, afterX.y + delta.y});

    if (SpriteInstance *blocker = FindBlocker(mover, others)) {
        mover.SetPosition(afterX);

        mover.SetColliding(true);
        blocker->SetColliding(true);
    }
}
