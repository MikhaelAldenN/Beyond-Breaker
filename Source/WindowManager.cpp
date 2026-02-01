#include "WindowManager.h"
#include "Scene.h" 
#include <algorithm>
#include "System/ImGuiRenderer.h" 
#include "System/Graphics.h"
#include "SceneGameBeyond.h"
#include <mutex>
#include <Framework.h>

void WindowManager::Update(float dt)
{
    static float priorityTimer = 0.0f;
    priorityTimer += dt;

    // HANYA enforce priority setiap 0.5 detik, bukan setiap frame!
    if (m_dirtyPriority && priorityTimer >= 0.5f)
    {
        EnforceWindowPriorities();
        m_dirtyPriority = false;
        priorityTimer = 0.0f;
    }
}

void WindowManager::EnforceWindowPriorities()
{
    // =========================================================
    // [OPTIMISASI KRUSIAL] EARLY EXIT
    // Jangan process jika tidak ada windows
    // =========================================================
    if (windows.empty())
        return;

    std::vector<GameWindow*> sortedWindows;
    sortedWindows.reserve(windows.size());

    // 1. Filter valid game windows (SINGLE PASS)
    for (auto& win : windows)
    {
        if (win.get() != debugWindow && win->GetPriority() < 100)
        {
            sortedWindows.push_back(win.get());
        }
    }

    // =========================================================
    // [OPTIMISASI 2] JIKA SORTED TIDAK BERUBAH, SKIP SETWINDOWPOS
    // Bandingkan dengan cache sebelumnya
    // =========================================================
    if (m_lastSortedOrder == sortedWindows)
        return; // Tidak ada perubahan, skip SetWindowPos

    m_lastSortedOrder = sortedWindows; // Update cache

    // 2. Sort by priority
    std::sort(sortedWindows.begin(), sortedWindows.end(),
        [](GameWindow* a, GameWindow* b) {
            return a->GetPriority() < b->GetPriority();
        });

#ifdef _DEBUG
    HWND hInsertAfter = HWND_NOTOPMOST;
#else
    HWND hInsertAfter = HWND_TOPMOST;
#endif

    // =========================================================
    // [OPTIMISASI 3] BATCH SETWINDOWPOS CALLS
    // Gunakan DeferWindowPos untuk batch multiple SetWindowPos sekaligus
    // =========================================================
    HDWP hDWP = BeginDeferWindowPos(sortedWindows.size());

    if (hDWP)
    {
        UINT uFlags = SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREDRAW;

        for (GameWindow* win : sortedWindows)
        {
            hDWP = DeferWindowPos(hDWP, win->GetHWND(), hInsertAfter, 0, 0, 0, 0, uFlags);
            hInsertAfter = win->GetHWND();
        }

        EndDeferWindowPos(hDWP);
    }
    else
    {
        // Fallback jika DeferWindowPos gagal
        UINT uFlags = SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREDRAW;
        for (GameWindow* win : sortedWindows)
        {
            SetWindowPos(win->GetHWND(), hInsertAfter, 0, 0, 0, 0, uFlags);
            hInsertAfter = win->GetHWND();
        }
    }

    if (debugWindow && debugWindow->IsVisible())
    {
        SetWindowPos(debugWindow->GetHWND(), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREDRAW);
    }
}

void WindowManager::RenderAll(float dt, Scene* scene)
{
    if (!scene) return;

    scene->DrawGUI();

    bool isBeyondScene = (dynamic_cast<SceneGameBeyond*>(scene) != nullptr);
    auto context = Graphics::Instance().GetDeviceContext();
    auto mainWindow = Framework::Instance()->GetMainWindow();

    // =========================================================
    // [OPTIMISASI 1] CACHE MAIN WINDOW POINTER
    // =========================================================
    for (auto& win : windows)
    {
        if (!win->IsVisible()) continue;

        if (win.get() != mainWindow && !win->ShouldRender(dt))
        {
            continue;
        }

        if (isBeyondScene) {
            win->BeginRender(0.1f, 0.1f, 0.15f);
        }
        else {
            win->BeginRender(0.0f, 0.0f, 0.0f);
        }

        scene->OnResize(win->GetWidth(), win->GetHeight());
        scene->Render(dt, win->GetCamera());

        // Render ImGui HANYA di Main Window
        if (win.get() == mainWindow)
        {
            ImGuiRenderer::Render(context);
        }

        int syncInterval = 0;
        win->EndRender(syncInterval);
    }
}

void WindowManager::HandleResize(HWND hWnd, int width, int height)
{
    for (auto& win : windows)
    {
        if (win->GetHWND() == hWnd)
        {
            win->Resize(width, height);
            return;
        }
    }
}

GameWindow* WindowManager::CreateGameWindow(const char* title, int width, int height)
{
    std::lock_guard<std::mutex> lock(m_windowsMutex);

    auto newWindow = std::make_unique<GameWindow>(title, width, height);
    GameWindow* ptr = newWindow.get();
    windows.push_back(std::move(newWindow));
    return ptr;
}

void WindowManager::DestroyWindow(GameWindow* targetWindow)
{
    std::lock_guard<std::mutex> lock(m_windowsMutex);

    windows.erase(
        std::remove_if(windows.begin(), windows.end(),
            [targetWindow](const std::unique_ptr<GameWindow>& p) {
                return p.get() == targetWindow;
            }),
        windows.end());

    MarkPriorityDirty();
}

void WindowManager::ClearAll()
{
    std::lock_guard<std::mutex> lock(m_windowsMutex);
    windows.clear();
    m_lastSortedOrder.clear(); // [OPTIMISASI] Clear cache juga
}