#include "SceneTitle.h"
#include "SceneGameBreaker.h"
#include "Framework.h"
#include "System/Input.h"
#include "System/Graphics.h"
#include <imgui.h>

SceneTitle::SceneTitle()
{
    camera = std::make_unique<Camera>();
    camera->SetOrthographic(1920.0f, 1080.0f, 0.1f, 1000.0f);
    camera->SetPosition(0.0f, 0.0f, -10.0f);
    camera->SetRotation(0.0f, 0.0f, 0.0f);

    // Load Sprite
    bgSprite = std::make_unique<Sprite>(
        Graphics::Instance().GetDevice(),
        "Data/Sprite/Scene Title/Sprite_BorderBrickDos.png"
    );
}

void SceneTitle::Update(float elapsedTime)
{
    // Switch Scene
    if (Input::Instance().GetKeyboard().IsTriggered(VK_RETURN))
    {
        Framework::Instance()->ChangeScene(std::make_unique<SceneGameBreaker>());
    }
}

void SceneTitle::Render(float dt, Camera* targetCamera)
{
    auto dc = Graphics::Instance().GetDeviceContext();
    auto rs = Graphics::Instance().GetRenderState();

    dc->OMSetBlendState(Graphics::Instance().GetAlphaBlendState(), nullptr, 0xFFFFFFFF);
    dc->OMSetDepthStencilState(rs->GetDepthStencilState(DepthState::TestAndWrite), 0);
    dc->RSSetState(rs->GetRasterizerState(RasterizerState::SolidCullNone));

    float width = 1920.0f;
    float height = 1080.0f;

    if (bgSprite)
    {
        bgSprite->Render(
            dc,
            camera.get(),
            0, 0, 0,        // Posisi Tengah (0,0,0)
            width, height,  // Ukuran Full
            0.0f, 0.0f, 0.0f, // ROTASI 0 SEMUA (2D Mode)
            1.0f, 1.0f, 1.0f, 1.0f
        );
    }
}

void SceneTitle::OnResize(int width, int height)
{
    // Opsional: Jika window di-resize, kita bisa update ukuran Ortho
    // Tapi untuk game pixel art/retro, biasanya kita kunci di 1920x1080 biar aspect rationya tetap.
    // Jadi fungsi ini boleh dikosongkan atau biarkan default.
}

void SceneTitle::DrawGUI()
{
    ImGui::Begin("Scene Title Debug (2D Mode)");

    if (camera)
    {
        ImGui::Text("Camera Pos (Z doesn't zoom in Ortho!)");

        DirectX::XMFLOAT3 pos = camera->GetPosition();
        float p[3] = { pos.x, pos.y, pos.z };
        if (ImGui::DragFloat3("Pos", p, 1.0f))
        {
            camera->SetPosition(p[0], p[1], p[2]);
        }

        // Reset Button
        if (ImGui::Button("Reset 2D View"))
        {
            camera->SetPosition(0.0f, 0.0f, -10.0f);
            camera->SetRotation(0.0f, 0.0f, 0.0f);
        }
    }
    ImGui::End();
}