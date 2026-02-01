#include "WindowManager.h"
#include "Scene.h"
#include <algorithm>
#include "System/ImGuiRenderer.h"
#include "System/Graphics.h"
#include "SceneGameBeyond.h"
#include <Framework.h>

void WindowManager::Update(float dt)
{
    float throttleInterval = 0.5f;

    m_priorityThrottleTimer += dt;

    if (m_dirtyPriority && m_priorityThrottleTimer >= throttleInterval)
    {
        EnforceWindowPriorities();
        m_dirtyPriority = false;
        m_priorityThrottleTimer = 0.0f;
    }

    MSG msg;
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
            break;

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void WindowManager::EnforceWindowPriorities()
{
    if (windows.empty())
        return;

    // =========================================================
    // STEP 1: Kumpulkan semua window yang ikut sorting.
    //
    // Filter: priority < PRIORITY_SENTINEL
    // Main window dan debug window punya priority = PRIORITY_SENTINEL
    // (9999) sehingga otomatis ter-exclude.
    // =========================================================
    std::vector<GameWindow*> sortedWindows;
    sortedWindows.reserve(windows.size());

    for (auto& win : windows)
    {
        if (win.get() != debugWindow && win->GetPriority() < GameWindow::PRIORITY_SENTINEL)
        {
            sortedWindows.push_back(win.get());
        }
    }

    // =========================================================
    // STEP 2: Sort DULU.
    //
    // Sebelumnya: compare �� cache �� sort
    // Itu berarti cache menyimpan UNSORTED vector, dan compare
    // juga terhadap UNSORTED vector. Hasilnya: cache hanya
    // mendeteksi "apakah SET window berubah?" bukan "apakah
    // z-ORDER berubah?". Setiap kali enemy/item/projectile
    // spawn atau mati, set berubah, sort ulang, dan karena
    // std::sort tidak stable untuk equal keys, urutan window
    // dengan priority yang sama bisa flip-flop setiap frame.
    //
    // Fix: sort dulu, BARU compare ke cache yang juga menyimpan
    // hasil SORTED. Kalau sorted result sama persis dengan cache,
    // z-order tidak berubah �� skip SetWindowPos sepenuhnya.
    // =========================================================
    std::sort(sortedWindows.begin(), sortedWindows.end(),
        [](GameWindow* a, GameWindow* b) {
            return a->GetPriority() < b->GetPriority();
        });

    // =========================================================
    // STEP 3: Compare SORTED result ke cache.
    // Kalau sama, z-order tidak akan berubah �� early return.
    // =========================================================
    if (m_lastSortedOrder == sortedWindows)
        return;

    // Update cache dengan sorted result yang baru
    m_lastSortedOrder = sortedWindows;

    // =========================================================
    // STEP 4: Jalankan SetWindowPos chain via DeferWindowPos.
    // =========================================================
#ifdef _DEBUG
    HWND hInsertAfter = HWND_NOTOPMOST;
#else
    HWND hInsertAfter = HWND_TOPMOST;
#endif

    HDWP hDWP = BeginDeferWindowPos(static_cast<int>(sortedWindows.size()));

    if (hDWP)
    {
        UINT uFlags = SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE |
            SWP_NOREDRAW | SWP_ASYNCWINDOWPOS;

        for (GameWindow* win : sortedWindows)
        {
            hDWP = DeferWindowPos(hDWP, win->GetHWND(), hInsertAfter, 0, 0, 0, 0, uFlags);
            if (!hDWP) break;
            hInsertAfter = win->GetHWND();
        }

        if (hDWP)
            EndDeferWindowPos(hDWP);
    }

    // Debug window selalu paling atas
    if (debugWindow && debugWindow->IsVisible())
    {
        SetWindowPos(debugWindow->GetHWND(), HWND_TOPMOST, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREDRAW | SWP_ASYNCWINDOWPOS);
    }
}

void WindowManager::RenderAll(float dt, Scene* scene)
{
    if (!scene) return;

    scene->DrawGUI();

    bool isBeyondScene = (dynamic_cast<SceneGameBeyond*>(scene) != nullptr);
    auto context = Graphics::Instance().GetDeviceContext();
    auto mainWindow = Framework::Instance()->GetMainWindow();

    for (auto& win : windows)
    {
        if (!win->IsVisible()) continue;

        if (win.get() != mainWindow)
        {
            if (!win->ShouldRender(dt))
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

        if (win.get() == mainWindow)
        {
            ImGuiRenderer::Render(context);
        }

        int syncInterval = (win.get() == mainWindow) ? 1 : 0;
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
    auto newWindow = std::make_unique<GameWindow>(title, width, height);
    GameWindow* ptr = newWindow.get();
    windows.push_back(std::move(newWindow));
    return ptr;
}

void WindowManager::DestroyWindow(GameWindow* targetWindow)
{
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
    windows.clear();
    m_lastSortedOrder.clear();
}
