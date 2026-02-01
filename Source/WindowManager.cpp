#include "WindowManager.h"
#include "Scene.h" 
#include <algorithm>
#include "System/ImGuiRenderer.h" 
#include "System/Graphics.h"
#include "SceneGameBeyond.h"
#include <mutex>
#include <Framework.h>

// =========================================================
// [NEW] DISCORD/STREAMING COMPATIBILITY
// =========================================================
static bool g_IsStreamingDetected = false;
static float g_StreamDetectionTimer = 0.0f;

// Detect if Discord/OBS is hooking our windows
static void DetectStreamingHooks()
{
    // Check for common streaming software processes
    HWND discordWnd = FindWindowA("Discord", nullptr);
    HWND obsWnd = FindWindowA("OBSWindowClass", nullptr);

    g_IsStreamingDetected = (discordWnd != nullptr) || (obsWnd != nullptr);
}

void WindowManager::Update(float dt)
{
    // =========================================================
    // [FIX DISCORD] ADAPTIVE THROTTLING
    // Throttle lebih agresif jika streaming detected
    // =========================================================
    g_StreamDetectionTimer += dt;
    if (g_StreamDetectionTimer >= 5.0f) // Check setiap 5 detik
    {
        DetectStreamingHooks();
        g_StreamDetectionTimer = 0.0f;
    }

    // Adaptive throttle interval
    float throttleInterval = g_IsStreamingDetected ? 1.0f : 0.5f;

    m_priorityThrottleTimer += dt;

    if (m_dirtyPriority && m_priorityThrottleTimer >= throttleInterval)
    {
        EnforceWindowPriorities();
        m_dirtyPriority = false;
        m_priorityThrottleTimer = 0.0f;
    }

    // =========================================================
    // [FIX CRITICAL] PUMP WINDOWS MESSAGES WITH YIELD
    // =========================================================
    MSG msg;
    int msgCount = 0;
    const int maxMsgPerFrame = 50; // Limit messages per frame

    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE) && msgCount < maxMsgPerFrame)
    {
        if (msg.message == WM_QUIT)
            break;

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
        msgCount++;
    }

    // =========================================================
    // [FIX DISCORD] YIELD CPU jika terlalu banyak messages
    // Prevent tight loop yang bikin hang saat streaming
    // =========================================================
    if (msgCount >= maxMsgPerFrame)
    {
        Sleep(1); // Yield 1ms ke OS
    }
}

void WindowManager::EnforceWindowPriorities()
{
    if (windows.empty())
        return;

    std::vector<GameWindow*> sortedWindows;
    sortedWindows.reserve(windows.size());

    for (auto& win : windows)
    {
        if (win.get() != debugWindow && win->GetPriority() < 100)
        {
            sortedWindows.push_back(win.get());
        }
    }

    //if (m_lastSortedOrder == sortedWindows)
    //    return;

    //m_lastSortedOrder = sortedWindows;

    //std::sort(sortedWindows.begin(), sortedWindows.end(),
    //    [](GameWindow* a, GameWindow* b) {
    //        return a->GetPriority() < b->GetPriority();
    //    });

    std::sort(sortedWindows.begin(), sortedWindows.end(),
    [](GameWindow* a, GameWindow* b) {
        return a->GetPriority() < b->GetPriority();
    });

if (m_lastSortedOrder == sortedWindows) return;
m_lastSortedOrder = sortedWindows;

#ifdef _DEBUG
    HWND hInsertAfter = HWND_NOTOPMOST;
#else
    HWND hInsertAfter = HWND_TOPMOST;
#endif

    // =========================================================
    // [FIX DISCORD] SKIP DEFER jika streaming
    // DeferWindowPos kadang conflict dengan screen capture hooks
    // =========================================================
    if (g_IsStreamingDetected)
    {
        // Fallback to individual calls dengan extra delay
        UINT uFlags = SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE |
            SWP_NOREDRAW | SWP_ASYNCWINDOWPOS | SWP_NOCOPYBITS;

        for (size_t i = 0; i < sortedWindows.size(); ++i)
        {
            GameWindow* win = sortedWindows[i];
            SetWindowPos(win->GetHWND(), hInsertAfter, 0, 0, 0, 0, uFlags);
            hInsertAfter = win->GetHWND();

            // Small yield every 5 windows untuk prevent blocking
            if ((i + 1) % 5 == 0)
            {
                Sleep(0); // Yield timeslice
            }
        }
    }
    else
    {
        // Normal batch operation jika tidak streaming
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
    }

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

    // =========================================================
    // [FIX DISCORD] LIMIT CONCURRENT RENDERS
    // Jangan render terlalu banyak windows sekaligus saat streaming
    // =========================================================
    int renderedThisFrame = 0;
    const int maxRendersPerFrame = g_IsStreamingDetected ? 3 : 999;

    for (auto& win : windows)
    {
        if (!win->IsVisible()) continue;

        // =========================================================
        // [FIX DISCORD] SKIP EXCESS RENDERS
        // Streaming software can't handle too many window updates per frame
        // =========================================================
        if (win.get() != mainWindow)
        {
            if (!win->ShouldRender(dt))
                continue;

            if (renderedThisFrame >= maxRendersPerFrame)
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

        // =========================================================
        // [FIX DISCORD] FORCE VSYNC OFF untuk sub-windows saat streaming
        // Prevent blocking pada Present()
        // =========================================================
        int syncInterval = 0; // Always 0 for sub-windows
        if (win.get() == mainWindow && !g_IsStreamingDetected)
        {
            syncInterval = 1; // VSync hanya main window jika tidak streaming
        }

        win->EndRender(syncInterval);

        if (win.get() != mainWindow)
            renderedThisFrame++;
    }

    // =========================================================
    // [FIX DISCORD] FLUSH GPU COMMANDS
    // Prevent command buffer buildup yang bikin hang
    // =========================================================
    if (g_IsStreamingDetected && renderedThisFrame > 0)
    {
        context->Flush();
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
    m_lastSortedOrder.clear();
}