#pragma once

#include <memory>
#include "Scene.h"
#include "Camera.h"
#include "System/Sprite.h" 

class SceneIntro : public Scene
{
public:
    SceneIntro();
    ~SceneIntro() override = default;

    void Update(float elapsedTime) override;
    void Render(float dt, Camera* camera = nullptr) override;
    void DrawGUI() override;
    void OnResize(int width, int height) override;

    Camera* GetCamera() const { return camera.get(); }

private:
    std::unique_ptr<Camera> camera;

    // Vars here
    // float bootTimer = 0.0f;
};