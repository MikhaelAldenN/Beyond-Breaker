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
    void SetVelocity(const DirectX::XMFLOAT3& v) { velocity = v; }
    void SetActive(bool active) { isActive = active; }

    CharacterMovement* GetMovement() const { return movement; }
    DirectX::XMFLOAT3 GetVelocity() const { return velocity; }

    float GetRadius() const { return radius; }

    bool IsActive() const { return isActive; }

private:
    DirectX::XMFLOAT3 velocity = { 0, 0, 0 };
    float speed = 7.0f;             // Speed of the ball
    float radius = 0.5f;            // Approximate size for collision

    // Arena Boundaries 
    float xLimit = 8.1f;            // Left/Right Walls
    float zLimitTop = 6.0f;         // Top Wall
    float zLimitBottom = -6.0f;     // Bottom (Game Over trigger)

    bool isActive = true;
};