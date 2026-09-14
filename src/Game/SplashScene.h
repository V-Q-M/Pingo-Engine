#pragma once

#include "Character.h"
#include "Engine/Scene.h"

// Splash screen like Unity's: Pingo appears on black with its idle
// animation, "Made with Pingo Engine" below it and the engine version at the
// bottom right.
//
// It is not part of any scene catalog and runs before the entry point in the
// Release and Debugging builds. It fades in and out again, a key or a click
// skips it.
class SplashScene : public Scene {
public:
    explicit SplashScene(Engine &engine);

    void Enter() override;

    void Update(float dt) override;

    void Draw() override;

    void DrawUI() override;

    // Jumps into the fade out, starting at the brightness the image currently has
    void Skip();

    // How dark the overlay currently is: 1 completely black, 0 clear
    float Shade() const;

    // Has it faded out? Then the change to the entry point is scheduled.
    bool IsFinished() const;

private:
    Character pingo;

    // Seconds since the start
    float time = 0.0f;

    bool finished = false;
};
