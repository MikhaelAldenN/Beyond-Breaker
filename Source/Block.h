#pragma once
#include "Character.h"

class Block : public Character
{
public:
    Block();
    ~Block() override;

    void Update(float elapsedTime, Camera* camera) override;
    void OnHit();
    bool IsActive() const { return isActive; }

    CharacterMovement* GetMovement() const { return movement; }

private:
    bool isActive = true;
};