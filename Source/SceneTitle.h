#pragma once

#include <memory>
#include "Scene.h"
#include "System/Sprite.h" 
#include "Camera.h"

class SceneTitle : public Scene
{
public:
    SceneTitle();
    ~SceneTitle() override = default; // Destructor default sudah cukup karena pakai unique_ptr

    void Update(float elapsedTime) override;
    void Render(float dt, Camera* camera = nullptr) override;
    void DrawGUI() override;
    void OnResize(int width, int height) override;

    // Getter camera untuk dipasang ke MainWindow
    Camera* GetCamera() const { return camera.get(); }

private:
    std::unique_ptr<Sprite> bgSprite;
    std::unique_ptr<Camera> camera;
};