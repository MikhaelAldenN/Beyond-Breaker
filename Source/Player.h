#pragma once
#include "Character.h"
#include <memory>

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

private:
    StateMachine* stateMachine;
    AnimationController* animator;
    Camera* activeCamera = nullptr;
    bool isInputEnabled = true;
};