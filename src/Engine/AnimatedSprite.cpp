
#include "AnimatedSprite.h"
#include <cassert>

AnimatedSprite::AnimatedSprite(Texture2D &texture,
                               int frameWidth,
                               int frameHeight,
                               int frameCount,
                               float shadowWidth,
                               float shadowHeight,
                               float shadowOffsetY)
    : texture(texture),
      frameWidth(frameWidth),
      frameHeight(frameHeight),
      frameCount(frameCount),
      shadowWidth(shadowWidth),
      shadowHeight(shadowHeight),
      shadowOffsetY(shadowOffsetY) {
    columns = texture.width / frameWidth;
}

void AnimatedSprite::Update(float dt) {
    timer += dt;

    if (timer >= frameTime.count() / 1000.0f) {
        timer -= frameTime.count() / 1000.0f;

        currentFrame++;

        if (currentFrame >= frameCount) {
            currentFrame = 0;
        }
    }
}

void AnimatedSprite::SetAnimationSpeed(std::chrono::milliseconds speed) {
    frameTime = speed;
}


void AnimatedSprite::SetFrame(int frame) {
    assert(frame >= 0 && frame < frameCount);
    currentFrame = frame;
}

int AnimatedSprite::Width() const {
    return frameWidth;
}

int AnimatedSprite::Height() const {
    return frameHeight;
}

float AnimatedSprite::ShadowWidth() const {
    return shadowWidth;
}

float AnimatedSprite::ShadowHeight() const {
    return shadowHeight;
}

float AnimatedSprite::ShadowOffsetY() const {
    return shadowOffsetY;
}

Rectangle AnimatedSprite::GetFrame() const {
    int x = currentFrame % columns;
    int y = currentFrame / columns;

    return {
        static_cast<float>(x * frameWidth),
        static_cast<float>(y * frameHeight),
        static_cast<float>(frameWidth),
        static_cast<float>(frameHeight)
    };
}

Texture2D &AnimatedSprite::GetTexture() const {
    return texture;
}

Vector2 AnimatedSprite::GetAnchor(Anchor anchor) const {
    switch (anchor) {
        case Anchor::Feet:
            return {frameWidth / 2.0f, (float) frameHeight};
        case Anchor::Center:
            return {frameWidth / 2.0f, frameHeight / 2.0f};
        case Anchor::Head:
            return {frameWidth / 2.0f, 0.0f};
        case Anchor::HealthBar:
            return {frameWidth / 2.0f, frameHeight + 8.0f};
        case Anchor::DamageText:
            return {frameWidth * 0.75f, 8.0f};
        case Anchor::Selection:
            return {frameWidth / 2.0f, frameHeight / 2.0f};
    }
    return {};
}

void AnimatedSprite::RandomizeAnimation() {
    timer = GetRandomValue(
                0,
                frameTime.count()
            ) / 1000.0f;

    currentFrame = GetRandomValue(
        0,
        frameCount - 1
    );
}
