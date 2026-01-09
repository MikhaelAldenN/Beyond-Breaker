#pragma once
#include "Character.h"

class Paddle : public Character
{
public:
    Paddle();
    ~Paddle() override;

    void Update(float elapsedTime, Camera* camera) override;

private:
    void HandleInput();

    // Settings
    float paddleSpeed = 10.0f;
    float xLimit = 6.0f; // How far left/right it can go
};