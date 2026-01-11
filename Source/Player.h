#pragma once
#include "Character.h"
#include <memory>
#include <DirectXMath.h>

// Forward Declaration (Biar hemat compile time dan cegah circular dependency)
class StateMachine;
class AnimationController;
class Camera;

class Player : public Character
{
public:
    Player();
    ~Player() override;

    void Update(float elapsedTime, Camera* camera) override;

    // --- Public Helpers untuk State ---
    StateMachine* GetStateMachine() const { return stateMachine; }
    CharacterMovement* GetMovement() const { return movement; }
    AnimationController* GetAnimator() const { return animator; }
    std::shared_ptr<Model> GetModel() const { return model; }

    // --- Input Helpers (Dipanggil oleh State) ---
    // Hapus parameter Camera* karena kita pakai activeCamera
    void HandleMovementInput();

    // Fungsi ini wajib ada implementasinya di cpp
    bool CheckJumpInput();

    // Control Switch 
    void SetInputEnabled(bool enable) { isInputEnabled = enable; }

    // Set Camera reference
    void SetCamera(Camera* cam) { activeCamera = cam; }

    // Breakout Mechanic
    void SetBreakoutMode(bool enable);

    struct BreakoutSettings
    {
        float shakeGain = 0.1f;        // How much shake is added per SPACE press
        float shakeDecay = 1.2f;       // How fast the shake calms down per second
        float maxShake = 0.8f;         // The maximum violence of the shake
        float shakeFrequency = 20.0f;  // Speed of vibration
    };

    BreakoutSettings breakoutSettings;

private:
    void UpdateBreakoutLogic(float elapsedTime);

    StateMachine* stateMachine;
    AnimationController* animator;
    Camera* activeCamera = nullptr;
    bool isInputEnabled = true;
    // --- Breakout State ---
    bool isBreakoutActive = false;
    bool wasSpacePressed = false;                               
    float currentShakeIntensity = 0.0f;
    DirectX::XMFLOAT3 originalPosition = { 0.0f, 0.0f, 0.0f }; 
};