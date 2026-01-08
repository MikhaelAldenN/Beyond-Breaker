#include <memory>
#include <sstream>
#include <imgui.h>
#include <SDL3/SDL.h>

#include "Framework.h"
#include "System/Graphics.h"
#include "System/ImGuiRenderer.h"
#include "SceneGame.h"
#include "System/Input.h"
#include "WindowManager.h"

Framework* Framework::pInstance = nullptr;

Framework::Framework()
{
    pInstance = this;

    Graphics::Instance().Initialize();
    mainWindow = std::make_unique<GameWindow>("Main Game Window", 1280, 720);

    Input::Instance().Initialize(mainWindow->GetHWND());
    ImGuiRenderer::Initialize(mainWindow->GetHWND(), Graphics::Instance().GetDevice(), Graphics::Instance().GetDeviceContext());

    scene = std::make_unique<SceneGame>();

    if (auto gameScene = dynamic_cast<SceneGame*>(scene.get()))
    {
        mainWindow->SetCamera(gameScene->GetMainCamera());
    }
}

Framework::~Framework()
{
    scene.reset();
    ImGuiRenderer::Finalize();
    mainWindow.reset();
    pInstance = nullptr;
}

Framework* Framework::Instance()
{
    return pInstance;
}

void Framework::Update(float elapsedTime)
{
    CalculateFrameStats(elapsedTime);
    Input::Instance().Update();
    ImGuiRenderer::NewFrame();

    if (scene)
    {
        scene->Update(elapsedTime);
    }
}

void Framework::Render(float elapsedTime)
{
    if (mainWindow)
    {
        mainWindow->BeginRender(0.5f, 0.5f, 0.5f);

        if (scene)
        {
            scene->OnResize(mainWindow->GetWidth(), mainWindow->GetHeight());
            scene->Render(elapsedTime, mainWindow->GetCamera());
            scene->DrawGUI();
        }

        ImGuiRenderer::Render(Graphics::Instance().GetDeviceContext());
        mainWindow->EndRender(1); // Sync interval 1
    }

    WindowManager::Instance().RenderAll(elapsedTime, scene.get());
}

void Framework::ForceUpdateRender()
{
    static Uint64 lastTime = 0;
    if (lastTime == 0) lastTime = SDL_GetPerformanceCounter();

    Uint64 currentTime = SDL_GetPerformanceCounter();
    float dt = (float)(currentTime - lastTime) / (float)SDL_GetPerformanceFrequency();
    lastTime = currentTime;

    if (dt > 0.05f) dt = 0.05f;

    Update(dt);
    Render(dt);
}

void Framework::CalculateFrameStats(float dt)
{
    static int frames = 0;
    static float timeAccumulator = 0.0f;

    frames++;
    timeAccumulator += dt;

    if (timeAccumulator >= 1.0f)
    {
        float fps = static_cast<float>(frames);
        std::ostringstream outs;
        outs.precision(6);
        outs << "Main Game Window - FPS: " << fps << " / Frame Time: " << (1000.0f / fps) << " ms";

        if (mainWindow)
        {
            SetWindowTextA(mainWindow->GetHWND(), outs.str().c_str());
        }

        frames = 0;
        timeAccumulator -= 1.0f;
    }
}

LRESULT CALLBACK Framework::HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (mainWindow && hWnd == mainWindow->GetHWND())
    {
        if (ImGuiRenderer::HandleMessage(hWnd, msg, wParam, lParam))
            return true;
    }

    switch (msg)
    {
    case WM_SIZE:
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);

        if (mainWindow && hWnd == mainWindow->GetHWND())
            mainWindow->Resize(width, height);
        else
            WindowManager::Instance().HandleResize(hWnd, width, height);
        return 0;
    }

    return 0;
}