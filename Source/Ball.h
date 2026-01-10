#pragma once
#include "Character.h"

class Ball : public Character
{
public:
    Ball();
    ~Ball() override;

    void Update(float elapsedTime, Camera* camera) override;

    void Launch();

    void Reset();

    CharacterMovement* GetMovement() const { return movement; }
    DirectX::XMFLOAT3 GetVelocity() const { return velocity; }
    void SetVelocity(const DirectX::XMFLOAT3& v) { velocity = v; }
    float GetRadius() const { return radius; }
    bool IsActive() const { return isActive; }

private:
    DirectX::XMFLOAT3 velocity = { 0, 0, 0 };
    float speed = 6.0f;             // Speed of the ball
    float radius = 0.5f;            // Approximate size for collision

    // Arena Boundaries 
    float xLimit = 9.5f;            // Left/Right Walls
    float zLimitTop = 7.0f;         // Top Wall
    float zLimitBottom = -6.3f;     // Bottom (Game Over trigger)

    bool isActive = false;
};