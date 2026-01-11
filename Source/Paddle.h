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
    void UpdateAI(float elapsedTime, Ball* ball);
    void CheckCollision(Ball* ball);
    void SetAIEnabled(bool enable) { isAIEnabled = enable; }

    CharacterMovement* GetMovement() const { return movement; }

private:
    void HandleInput();

    // Settings
    float paddleSpeed = 1.5f;
    float xLimit = 7.3f;        // How far left/right it can go
    float launchTimer = 0.0f;   // Timer for AI launch the ball
	bool isAIEnabled = false;
};