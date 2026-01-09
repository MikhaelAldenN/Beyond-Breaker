#pragma once
#include "Scene.h"
#include "Camera.h"
#include "System/Sprite.h" 
#include <memory>
#include "Primitive.h"
#include "BitmapFont.h"
#include "imgui.h"

class SceneIntroOS : public Scene
{
public:
    SceneIntroOS();
    ~SceneIntroOS() override = default;

    void Update(float elapsedTime) override;
    void Render(float dt, Camera* camera = nullptr) override;
    void DrawGUI() override;
    void OnResize(int width, int height) override;

    Camera* GetCamera() const { return camera.get(); }


private:
    std::unique_ptr<Camera> camera;
    std::unique_ptr<Sprite> spriteOSLogo; // Logo OS Besarnya
    std::unique_ptr<Primitive> primitiveBatch;
    Microsoft::WRL::ComPtr<ID3D11BlendState> blendState;

    float timer = 0.0f;
    const float LOADING_DURATION = 5.0f; // Logo muncul selama 3 detik

    // Opsional: Buat efek kedip-kedip biar kayak loading beneran
    float blinkTimer = 0.0f;
    bool isVisible = true;

    // --- Variabel Debug Font ---
    float debugFontPosX = 298.0f;
    float debugFontPosY = 175.74f;
    float debugFontSize = 0.417f;
    float debugFontColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // Default Kuning (RGBA)

    // --- ANIMATION VARIABLES ---
    float logoTargetX;       // Target X (Tengah)

    // LOGO 1 (Misal: Datang dari Kiri, Posisi Lebih Atas)
    float logo1_StartX;
    float logo1_CurrentX;
    float logo1_Y;           // <--- Y tetap untuk logo 1

    // LOGO 2 (Misal: Datang dari Kanan, Posisi Lebih Bawah)
    float logo2_StartX;
    float logo2_CurrentX;
    float logo2_Y;           // <--- Y tetap untuk logo 2

    float animTimer = 0.0f;
    const float ANIM_DURATION = 1.0f;

    // --- HELPER FUNCTION (REUSABLE) ---
    // Fungsi ini bisa kamu pindahkan ke file MathUtils.h nanti jika mau dipakai global
    template <typename T>
    T Lerp(T a, T b, float t)
    {
        return a + (b - a) * t;
    }
};