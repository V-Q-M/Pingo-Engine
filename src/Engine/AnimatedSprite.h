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
                   float shadowOffsetY
    );

    void Update(float dt);

    void SetAnimationSpeed(std::chrono::milliseconds speed);

    void SetFrame(int frame);

    int Width() const;

    int Height() const;

    float ShadowWidth() const;

    float ShadowHeight() const;

    float ShadowOffsetY() const;

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

    int currentFrame = 0;

    float timer = 0.0f;

    std::chrono::milliseconds frameTime{150};
};
