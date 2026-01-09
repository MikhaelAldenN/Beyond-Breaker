#pragma once

#include <memory>
#include "Scene.h"
#include "Camera.h"
#include "System/Sprite.h" 
#include "BitmapFont.h"
#include "imgui.h"

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

    // --- Variabel Debug Font ---
    float debugFontPosX = 271.0f;
    float debugFontPosY = 38.5f;
    float debugFontSize = 0.417f;
    float debugFontColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f }; // Default Kuning (RGBA)
};