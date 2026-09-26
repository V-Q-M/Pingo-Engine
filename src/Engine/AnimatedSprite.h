#pragma once

#include "raylib.h"
#include <chrono>
#include "Anchor.h"

class AnimatedSprite {
public:
    AnimatedSprite(Texture2D &texture,
                   int frameWidth,
                   int frameHeight,
                   int frameCount,
                   float shadowWidth,
                   float shadowHeight,
                   float shadowOffsetY,
                   float shadowOffsetX = 0.0f
    );

    void Update(float dt);

    void SetAnimationSpeed(std::chrono::milliseconds speed);

    void SetFrame(int frame);

    // Row of the sprite sheet the frames are taken from, 0 is the first one.
    // A character in an effect animates a different row, see ObjectEffects.
    void SetRow(int row);

    int Row() const;

    // How many rows of this frame size the sheet has
    int RowCount() const;

    int Width() const;

    int Height() const;

    float ShadowWidth() const;

    float ShadowHeight() const;

    float ShadowOffsetY() const;

    // Moves the shadow sideways, negative to the left
    float ShadowOffsetX() const;

    // How many pixels narrower the shadow gets while the character is up in
    // its animation. 0 keeps it the same size the whole time.
    void SetShadowBounce(float pixels);

    float ShadowBounce() const;

    // Where in its animation the character is, from 0.0 to 1.0. The shadow
    // follows it, so both keep the same rhythm.
    float AnimationPhase() const;

    Vector2 GetAnchor(Anchor anchor) const;

    void RandomizeAnimation();

    Rectangle GetFrame() const;

    Texture2D &GetTexture() const;

private:
    Texture2D &texture;

    int frameWidth;
    int frameHeight;
    int frameCount;
    int columns;
    float shadowWidth;
    float shadowHeight;
    float shadowOffsetY;
    float shadowOffsetX;
    float shadowBounce = 0.0f;

    int currentFrame = 0;
    int row = 0;

    float timer = 0.0f;

    std::chrono::milliseconds frameTime{150};
};
