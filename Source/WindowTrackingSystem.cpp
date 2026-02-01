#include "WindowTrackingSystem.h"
#include "WindowManager.h"
#include <SDL3/SDL.h>
#include <windows.h>
#include <cmath>

using namespace DirectX;

// =========================================================
// [FIX HANG] HAPUS IsStreamingActive() SEPENUHNYA.
//
// Fungsi ini punya bug fatal: `cacheTimer` static local
// NEVER diincrement (tidak ada `cacheTimer += dt` di manapun).
// Akibatnya:
//   1. Pertama kali masuk: cached=false  jalankan FindWindowA
//   2. Set cached=true, cacheTimer=0
//   3. Frame berikutnya: cached=true, cacheTimer=0
//       condition `!cached || cacheTimer > 5.0f` = false
//       SELALU return cached result dari frame pertama
//
// Kalau Discord running di background saat game start,
// result = true SELAMANYA. Semua codepath "if streaming"
// (Sleep calls, render caps, individual SetWindowPos)
// akan PERMANENTLY aktif.
// =========================================================

WindowTrackingSystem::WindowTrackingSystem()
{
}

WindowTrackingSystem::~WindowTrackingSystem()
{
    ClearAll();
}

void WindowTrackingSystem::ClearAll()
{
    for (auto& tracked : m_trackedWindows)
    {
        if (tracked->window)
        {
            WindowManager::Instance().DestroyWindow(tracked->window);
        }
    }
    m_trackedWindows.clear();
    m_windowLookup.clear();
}

bool WindowTrackingSystem::AddTrackedWindow(
    const TrackedWindowConfig& config,
    std::function<DirectX::XMFLOAT3()> getTargetPos,
    std::function<DirectX::XMFLOAT2()> getTargetSize)
{
    GameWindow* window = WindowManager::Instance().CreateGameWindow(
        config.title.c_str(),
        config.width,
        config.height
    );

    if (!window) return false;

    window->SetPriority(config.priority);
    WindowManager::Instance().MarkPriorityDirty();

    if (config.fpsLimit > 0.0f)
    {
        window->SetTargetFPS(config.fpsLimit);
    }

    if (config.name == "player") window->SetDraggable(false);

    auto camera = std::make_shared<Camera>();
    window->SetCamera(camera.get());

    auto tracked = std::make_unique<TrackedWindow>();
    tracked->name = config.name;
    tracked->window = window;
    tracked->camera = camera;
    tracked->trackingOffset = config.trackingOffset;
    tracked->getTargetPositionFunc = getTargetPos;
    tracked->getTargetSizeFunc = getTargetSize;

    tracked->state.targetW = (float)config.width;
    tracked->state.targetH = (float)config.height;
    tracked->state.actualW = config.width;
    tracked->state.actualH = config.height;

    if (getTargetPos)
    {
        XMFLOAT3 initialPos = getTargetPos();
        float screenX, screenY;
        WorldToScreenPos(initialPos, screenX, screenY);

        tracked->state.targetX = screenX - (window->GetWidth() * 0.5f);
        tracked->state.targetY = screenY - (window->GetHeight() * 0.5f);
        tracked->state.actualX = static_cast<int>(roundf(tracked->state.targetX));
        tracked->state.actualY = static_cast<int>(roundf(tracked->state.targetY));

        SDL_SetWindowPosition(window->GetSDLWindow(), tracked->state.actualX, tracked->state.actualY);
    }

    m_windowLookup[config.name] = tracked.get();
    m_trackedWindows.push_back(std::move(tracked));

    return true;
}

TrackedWindow* WindowTrackingSystem::GetTrackedWindow(const std::string& name)
{
    auto it = m_windowLookup.find(name);
    if (it != m_windowLookup.end()) return it->second;
    return nullptr;
}

void WindowTrackingSystem::Update(float dt)
{
    // =========================================================
    // Cache screen dimensions  refresh setiap 1 detik
    // =========================================================
    m_cacheUpdateTimer += dt;
    if (m_cacheUpdateTimer >= 1.0f)
    {
        m_cachedScreenWidth = GetSystemMetrics(SM_CXSCREEN);
        m_cachedScreenHeight = GetSystemMetrics(SM_CYSCREEN);
        m_cacheUpdateTimer = 0.0f;
    }

    // =========================================================
    // [FIX] Update SEMUA window setiap frame.
    //
    // Sebelumnya ada `maxUpdatesPerFrame = 5` yang aktif saat
    // streaming detected (yang permanently true kalau Discord
    // running). Itu bikin sebagian windows tidak pernah di-update
    // posisinya, dan SwapChain mereka out-of-sync.
    // =========================================================
    for (auto& tracked : m_trackedWindows)
    {
        if (!tracked->window || !tracked->window->IsVisible())
            continue;

        UpdateSingleWindow(dt, *tracked);
    }
}

void WindowTrackingSystem::UpdateSingleWindow(float dt, TrackedWindow& tracked)
{
    if (!tracked.window || !tracked.camera || !tracked.getTargetPositionFunc)
        return;

    // Threshold yang balanced untuk smooth tracking tanpa terlalu banyak SDL calls
    const int posThreshold = 8;   // Medium threshold
    const int sizeThreshold = 8;  // Medium threshold

    // =========================================================
    // SIZE UPDATE
    // =========================================================
    if (tracked.getTargetSizeFunc)
    {
        XMFLOAT2 desiredSize = tracked.getTargetSizeFunc();

        float tSize = min(m_followSpeed * dt, 1.0f);
        tracked.state.targetW += (desiredSize.x - tracked.state.targetW) * tSize;
        tracked.state.targetH += (desiredSize.y - tracked.state.targetH) * tSize;

        int newW = static_cast<int>(roundf(tracked.state.targetW));
        int newH = static_cast<int>(roundf(tracked.state.targetH));

        int deltaW = abs(newW - tracked.state.actualW);
        int deltaH = abs(newH - tracked.state.actualH);

        if (deltaW >= sizeThreshold || deltaH >= sizeThreshold)
        {
            newW = max(10, newW);
            newH = max(10, newH);

            SDL_SetWindowSize(tracked.window->GetSDLWindow(), newW, newH);
            tracked.state.actualW = newW;
            tracked.state.actualH = newH;
        }
    }

    // =========================================================
    // POSITION UPDATE
    // =========================================================
    XMFLOAT3 targetWorldPos = tracked.getTargetPositionFunc();
    targetWorldPos.x += tracked.trackingOffset.x;
    targetWorldPos.y += tracked.trackingOffset.y;
    targetWorldPos.z += tracked.trackingOffset.z;

    float targetScreenX, targetScreenY;
    WorldToScreenPos(targetWorldPos, targetScreenX, targetScreenY);

    float destX = targetScreenX - (tracked.window->GetWidth() * 0.5f);
    float destY = targetScreenY - (tracked.window->GetHeight() * 0.5f);

    float t = min(m_followSpeed * dt, 1.0f);
    tracked.state.targetX += (destX - tracked.state.targetX) * t;
    tracked.state.targetY += (destY - tracked.state.targetY) * t;

    int newX = static_cast<int>(roundf(tracked.state.targetX));
    int newY = static_cast<int>(roundf(tracked.state.targetY));

    int deltaX = abs(newX - tracked.state.actualX);
    int deltaY = abs(newY - tracked.state.actualY);

    if (deltaX >= posThreshold || deltaY >= posThreshold)
    {
        SDL_SetWindowPosition(tracked.window->GetSDLWindow(), newX, newY);
        tracked.state.actualX = newX;
        tracked.state.actualY = newY;

        // =========================================================
        // [FIX HANG] HAPUS Sleep(0) yang ada di sini.
        //
        // Sleep() di single-threaded game loop = yield ke OS.
        // OS kemudian process pending messages. Tapi message queue
        // sudah flooded dari SDL_SetWindowPosition calls.
        // Itu creates circular wait:
        //   Sleep  pump messages  lebih banyak position updates
        //    lebih banyak messages  Sleep lagi  ...
        // =========================================================
    }

    // Update Camera Projection
    UpdateOffCenterProjection(tracked.camera.get(), tracked.window, GetUnifiedCameraHeight());
}

void WindowTrackingSystem::GetScreenDimensions(int& outWidth, int& outHeight)
{
    if (m_cachedScreenWidth > 0) {
        outWidth = m_cachedScreenWidth;
        outHeight = m_cachedScreenHeight;
    }
    else {
        outWidth = GetSystemMetrics(SM_CXSCREEN);
        outHeight = GetSystemMetrics(SM_CYSCREEN);
        m_cachedScreenWidth = outWidth;
        m_cachedScreenHeight = outHeight;
    }
}

void WindowTrackingSystem::WorldToScreenPos(const DirectX::XMFLOAT3& worldPos, float& outScreenX, float& outScreenY)
{
    int screenW, screenH;
    GetScreenDimensions(screenW, screenH);

    outScreenX = (screenW * 0.5f) + (worldPos.x * m_pixelToUnitRatio);
    outScreenY = (screenH * 0.5f) - (worldPos.z * m_pixelToUnitRatio);
}

float WindowTrackingSystem::GetUnifiedCameraHeight()
{
    int screenW, screenH;
    GetScreenDimensions(screenW, screenH);
    float halfFovTan = tanf(XMConvertToRadians(m_fov) * 0.5f);
    return (screenH * 0.5f) / (m_pixelToUnitRatio * halfFovTan);
}

void WindowTrackingSystem::UpdateOffCenterProjection(Camera* targetCam, GameWindow* targetWin, float camHeight)
{
    int screenW, screenH;
    GetScreenDimensions(screenW, screenH);

    targetCam->SetPosition(0.0f, camHeight, 0.0f);
    targetCam->LookAt({ 0.0f, 0.0f, 0.0f });

    int winX, winY, winW, winH;
    SDL_GetWindowPosition(targetWin->GetSDLWindow(), &winX, &winY);
    SDL_GetWindowSize(targetWin->GetSDLWindow(), &winW, &winH);

    float nearZ = 0.1f;
    float farZ = 1000.0f;
    float halfFovTan = tanf(XMConvertToRadians(m_fov) * 0.5f);

    float halfHeight = nearZ * halfFovTan;
    float halfWidth = halfHeight * ((float)screenW / screenH);

    double screenWd = (double)screenW;
    double screenHd = (double)screenH;

    float l = (float)((winX / screenWd) * 2.0 - 1.0);
    float r = (float)(((winX + winW) / screenWd) * 2.0 - 1.0);
    float t = (float)(1.0 - (winY / screenHd) * 2.0);
    float b = (float)(1.0 - ((winY + winH) / screenHd) * 2.0);

    targetCam->SetOffCenterProjection(
        l * halfWidth, r * halfWidth,
        b * halfHeight, t * halfHeight,
        nearZ, farZ
    );
}

void WindowTrackingSystem::RemoveTrackedWindow(const std::string& name)
{
    auto it = m_windowLookup.find(name);
    if (it == m_windowLookup.end()) return;

    TrackedWindow* trackedInfo = it->second;

    if (trackedInfo && trackedInfo->window)
    {
        WindowManager::Instance().DestroyWindow(trackedInfo->window);
    }

    m_windowLookup.erase(it);

    m_trackedWindows.erase(
        std::remove_if(m_trackedWindows.begin(), m_trackedWindows.end(),
            [&name](const std::unique_ptr<TrackedWindow>& ptr) {
                return ptr->name == name;
            }),
        m_trackedWindows.end());
}

bool WindowTrackingSystem::AddPooledTrackedWindow(
    const TrackedWindowConfig& config,
    std::function<DirectX::XMFLOAT3()> getTargetPos,
    std::function<DirectX::XMFLOAT2()> getTargetSize)
{
    if (!m_windowPool.empty())
    {
        auto recycled = std::move(m_windowPool.back());
        m_windowPool.pop_back();

        recycled->name = config.name;
        recycled->trackingOffset = config.trackingOffset;
        recycled->getTargetPositionFunc = getTargetPos;
        recycled->getTargetSizeFunc = getTargetSize;

        recycled->window->SetTitle(config.title.c_str());

        if (config.fpsLimit > 0.0f) recycled->window->SetTargetFPS(config.fpsLimit);

        SDL_ShowWindow(recycled->window->GetSDLWindow());

        m_windowLookup[config.name] = recycled.get();
        m_trackedWindows.push_back(std::move(recycled));

        return true;
    }

    return AddTrackedWindow(config, getTargetPos, getTargetSize);
}

void WindowTrackingSystem::ReleasePooledWindow(const std::string& name)
{
    auto it = m_windowLookup.find(name);
    if (it == m_windowLookup.end()) return;

    TrackedWindow* ptr = it->second;

    auto vecIt = std::find_if(m_trackedWindows.begin(), m_trackedWindows.end(),
        [&name](const std::unique_ptr<TrackedWindow>& p) {
            return p->name == name;
        });

    if (vecIt != m_trackedWindows.end())
    {
        SDL_HideWindow(ptr->window->GetSDLWindow());
        m_windowPool.push_back(std::move(*vecIt));
        m_trackedWindows.erase(vecIt);
        m_windowLookup.erase(it);
    }
}