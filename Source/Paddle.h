#pragma once
#include "Character.h"
#include "Ball.h"

class Ball;
class Paddle : public Character
{
public:
    Paddle();
    ~Paddle() override;

    void Update(float elapsedTime, Camera* camera) override;
    void CheckCollision(Ball* ball);

    CharacterMovement* GetMovement() const { return movement; }

private:
    void HandleInput();

    // Settings
    float paddleSpeed = 10.0f;
    float xLimit = 9.1f; // How far left/right it can go
};