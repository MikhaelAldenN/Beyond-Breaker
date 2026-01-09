#pragma once
#include "Scene.h"
#include "Camera.h"
#include "System/Sprite.h" 
#include <memory>

class SceneIntroOS : public Scene
{
public:
    SceneIntroOS();
    ~SceneIntroOS() override = default;

    void Update(float elapsedTime) override;
    void Render(float dt, Camera* camera = nullptr) override;
    void OnResize(int width, int height) override;

private:
    std::unique_ptr<Camera> camera;
    std::unique_ptr<Sprite> spriteOSLogo; // Logo OS Besarnya

    float timer = 0.0f;
    const float LOADING_DURATION = 5.0f; // Logo muncul selama 3 detik

    // Opsional: Buat efek kedip-kedip biar kayak loading beneran
    float blinkTimer = 0.0f;
    bool isVisible = true;
};