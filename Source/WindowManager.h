#pragma once

#include <vector>
#include <memory>
#include <algorithm>
#include <windows.h>
#include <mutex>
#include "GameWindow.h"

class Scene;

class WindowManager
{
public:
    static WindowManager& Instance()
    {
        static WindowManager instance;
        return instance;
    }

    void Update(float dt);
    void RenderAll(float dt, Scene* scene);
    void HandleResize(HWND hWnd, int width, int height);
    void ClearAll();

    GameWindow* CreateGameWindow(const char* title, int width, int height);
    void DestroyWindow(GameWindow* targetWindow);
    void EnforceWindowPriorities();
    void MarkPriorityDirty() { m_dirtyPriority = true; }

    void SetDebugWindow(GameWindow* win) { debugWindow = win; }
    GameWindow* GetDebugWindow() const { return debugWindow; }

    bool HasWindows() const { return !windows.empty(); }

    GameWindow* GetWindowByIndex(size_t index)
    {
        if (index < windows.size()) return windows[index].get();
        return nullptr;
    }

private:
    WindowManager() = default;
    ~WindowManager() = default;
    WindowManager(const WindowManager&) = delete;
    void operator=(const WindowManager&) = delete;

private:
    std::vector<std::unique_ptr<GameWindow>> windows;
    GameWindow* debugWindow = nullptr;

    bool m_dirtyPriority = false;
    mutable std::mutex m_windowsMutex;

    // =========================================================
    // [OPTIMISASI] Cache untuk sorted order
    // Jika tidak berubah, skip SetWindowPos
    // =========================================================
    std::vector<GameWindow*> m_lastSortedOrder;

    // =========================================================
    // [FIX CRITICAL] Throttle timer untuk priority enforcement
    // Prevent blocking calls setiap frame
    // =========================================================
    float m_priorityThrottleTimer = 0.0f;
};