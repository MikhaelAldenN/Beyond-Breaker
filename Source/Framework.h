#pragma once

#include <windows.h>
#include <memory>
#include <vector>

#include "System/HighResolutionTimer.h"
#include "GameWindow.h"
#include "Scene.h"

class Framework
{
public:
    Framework();
    ~Framework();

    static Framework* Instance();

    // --- Core Loop ---
    void Update(float elapsedTime);
    void Render(float elapsedTime);
    void ForceUpdateRender(); // Handling for blocking operations (e.g. Resize/Drag)

    // --- Message Handling ---
    LRESULT CALLBACK HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    GameWindow* GetMainWindow() const { return mainWindow.get(); }

private:
    void CalculateFrameStats(float dt);

    static Framework* pInstance;
    HighResolutionTimer timer;
    std::unique_ptr<GameWindow> mainWindow;
    std::unique_ptr<Scene> scene;
};