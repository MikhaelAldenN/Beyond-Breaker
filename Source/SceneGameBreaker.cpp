#include <imgui.h>
#include <SDL3/SDL.h>
#include "System/Graphics.h"
#include "System/Input.h"
#include "SceneGameBreaker.h"
#include "WindowManager.h"

SceneGameBreaker::SceneGameBreaker()
{
    float screenW = 1280.0f;
    float screenH = 720.0f;

    // 1. Initialize Main Assets
    mainCamera = new Camera();
    mainCamera->SetPerspectiveFov(DirectX::XMConvertToRadians(45), screenW / screenH, 0.1f, 1000.0f);
    auto& camCtrl = CameraController::Instance();
    camCtrl.SetActiveCamera(mainCamera);

    camCtrl.SetControlMode(CameraControlMode::FixedStatic);
    camCtrl.SetFixedSetting({ 0.0f, 18.0f, 0.0f });
    camCtrl.SetTarget({ 0.0f, 0.0f, 0.0f });

    player = new Player();

    paddle = new Paddle();
}

SceneGameBreaker::~SceneGameBreaker()
{
    if (mainCamera) delete mainCamera;
    if (player) delete player;
    if (paddle) delete paddle;
}

void SceneGameBreaker::Update(float elapsedTime)
{
    // Update Player & Camera Controller
    Camera* activeCam = CameraController::Instance().GetActiveCamera();
    if (paddle)
    {
        paddle->Update(elapsedTime, activeCam);
    }

    CameraController::Instance().Update(elapsedTime);
}

void SceneGameBreaker::Render(float elapsedTime, Camera* camera)
{
    Camera* targetCam = camera ? camera : mainCamera;

    // Setup Render State
    auto dc = Graphics::Instance().GetDeviceContext();
    auto rs = Graphics::Instance().GetRenderState();

    dc->OMSetBlendState(rs->GetBlendState(BlendState::Opaque), nullptr, 0xFFFFFFFF);
    dc->OMSetDepthStencilState(rs->GetDepthStencilState(DepthState::TestAndWrite), 0);
    dc->RSSetState(rs->GetRasterizerState(RasterizerState::SolidCullBack));

    RenderScene(elapsedTime, targetCam);

    // Render Debug Shapes (Main Camera Only)
    if (targetCam == mainCamera)
    {
        Graphics::Instance().GetShapeRenderer()->Render(dc, targetCam->GetView(), targetCam->GetProjection());
    }
}

void SceneGameBreaker::RenderScene(float elapsedTime, Camera* camera)
{
    if (!camera) return;

    auto dc = Graphics::Instance().GetDeviceContext();
    auto primRenderer = Graphics::Instance().GetPrimitiveRenderer();
    auto modelRenderer = Graphics::Instance().GetModelRenderer();
    RenderContext rc{ dc, Graphics::Instance().GetRenderState(), camera, nullptr };

    // Draw Grid
    primRenderer->DrawGrid(20, 1);
    primRenderer->Render(dc, camera->GetView(), camera->GetProjection(), D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

    // Draw Player
    if (player)
    {
        player->Render(modelRenderer);
        modelRenderer->Render(rc);
    }

    // Draw Paddle
    if (paddle)
    {
        paddle->Render(modelRenderer);
        modelRenderer->Render(rc);
    }
}

void SceneGameBreaker::DrawGUI()
{
    CameraController::Instance().DrawDebugGUI();
    ImGui::Begin("Scene Info");
    ImGui::Text("WASD to Move");
    ImGui::End();
}

void SceneGameBreaker::OnResize(int width, int height)
{
    if (height == 0) height = 1;
    if (mainCamera)
    {
        mainCamera->SetAspectRatio((float)width / (float)height);
    }
}