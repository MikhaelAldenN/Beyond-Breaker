#pragma once

#include <memory>
#include "Scene.h"
#include "Camera.h"
#include "System/Sprite.h" 
#include "BitmapFont.h"

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
    std::unique_ptr<BitmapFont> biosFont;
    Microsoft::WRL::ComPtr<ID3D11BlendState> blendState;

    std::unique_ptr<Sprite> bgSpriteIntro;

    // Vars here
    // float bootTimer = 0.0f;
};