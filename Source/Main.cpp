#include <windows.h>
#include <memory>
#include <SDL3/SDL.h> 

#include "Framework.h"

// --- GLOBAL VARIABLES ---
Framework* g_frameworkPtr = nullptr;
WNDPROC g_originalWndProc = nullptr;
const UINT_PTR IDT_GAME_LOOP = 1;

// Waktu Global
Uint64 g_lastTime = 0;
Uint64 g_frequency = 0;

// --- FUNGSI HELPER: FORCE RENDER (SAAT RESIZE) ---
void ForceGameLoop()
{
    if (g_frameworkPtr)
    {
        if (g_lastTime == 0) g_lastTime = SDL_GetPerformanceCounter();

        Uint64 currentTime = SDL_GetPerformanceCounter();
        float elapsedTime = (float)(currentTime - g_lastTime) / (float)g_frequency;
        g_lastTime = currentTime;

        // Safety cap
        if (elapsedTime > 0.1f) elapsedTime = 0.016f;
        if (elapsedTime < 0.0001f) return;

        // Cukup panggil Render utama. 
        // Framework sekarang otomatis merender Window Utama DAN Sub Window sekaligus.
        g_frameworkPtr->Update(elapsedTime);
        g_frameworkPtr->Render(elapsedTime);
    }
}

// --- WINDOW PROCEDURE HOOK ---
LRESULT CALLBACK WndProcHook(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // 1. Serahkan pesan ke Framework dulu (untuk ImGui & Resize internal)
    if (g_frameworkPtr)
    {
        g_frameworkPtr->HandleMessage(hWnd, msg, wParam, lParam);
    }

    switch (msg)
    {
        // ---------------------------------------------------------
        // LOGIKA RESIZE MULUS (Tetap Pertahankan)
        // ---------------------------------------------------------
    case WM_ENTERSIZEMOVE:
        SetTimer(hWnd, IDT_GAME_LOOP, 1, NULL);
        return 0;

    case WM_EXITSIZEMOVE:
        KillTimer(hWnd, IDT_GAME_LOOP);
        return 0;

    case WM_TIMER:
        if (wParam == IDT_GAME_LOOP)
        {
            ForceGameLoop();
            return 0;
        }
        break;

    case WM_GETMINMAXINFO:
    {
        MINMAXINFO* mmi = (MINMAXINFO*)lParam;
        mmi->ptMinTrackSize.x = 640;
        mmi->ptMinTrackSize.y = 480;
        return 0;
    }

    case WM_SIZE:
    {
        if (wParam == SIZE_MINIMIZED) return 0;

        // Tidak perlu panggil OnResize manual lagi, HandleMessage di atas sudah mengurusnya.
        // Kita hanya perlu ForceGameLoop agar visualnya update realtime saat ditarik.
        if (g_frameworkPtr)
        {
            ForceGameLoop();
        }
        return 0;
    }
    }

    return CallWindowProc(g_originalWndProc, hWnd, msg, wParam, lParam);
}

// --- MAIN ENTRY POINT ---
int main(int argc, char* argv[])
{
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("Gagal Init SDL: %s", SDL_GetError());
        return -1;
    }
    g_frequency = SDL_GetPerformanceFrequency();

    // 1. BUAT FRAMEWORK (Window dibuat otomatis di dalam sini)
    //    Tidak perlu SDL_CreateWindow manual lagi!
    auto framework = std::make_unique<Framework>();
    g_frameworkPtr = framework.get();

    // 2. SETUP HOOK KE MAIN WINDOW
    //    Ambil HWND dari framework yang baru dibuat
    GameWindow* mainWindow = framework->GetMainWindow();
    if (mainWindow)
    {
        HWND hWnd = mainWindow->GetHWND();
        g_originalWndProc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)WndProcHook);
    }

    // 3. GAME LOOP
    bool running = true;
    g_lastTime = SDL_GetPerformanceCounter();

    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            // Input::Instance().HandleSDLEvent(&event); // Opsional
            if (event.type == SDL_EVENT_QUIT)
                running = false;
        }

        // Time Step
        Uint64 currentTime = SDL_GetPerformanceCounter();
        float elapsedTime = (float)(currentTime - g_lastTime) / (float)g_frequency;
        g_lastTime = currentTime;

        if (elapsedTime > 0.05f) elapsedTime = 0.05f;

        // Update & Render (Otomatis handle semua window)
        framework->Update(elapsedTime);
        framework->Render(elapsedTime);
    }

    // Restore WndProc sebelum keluar (Good practice)
    if (mainWindow && g_originalWndProc)
    {
        SetWindowLongPtr(mainWindow->GetHWND(), GWLP_WNDPROC, (LONG_PTR)g_originalWndProc);
    }

    framework.reset();
    SDL_Quit();

    return 0;
}