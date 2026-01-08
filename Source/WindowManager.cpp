#include "WindowManager.h"
#include "Scene.h" 

void WindowManager::Update(float dt)
{
    // Di sini nanti bisa tambah logika animasi window (misal window bergetar saat kena damage)
}

void WindowManager::RenderAll(float dt, Scene* scene)
{
    if (!scene) return;

    // Loop semua window di gudang
    for (auto& win : windows)
    {
        // 1. Siapkan Layar Window Ini
        //    Kita kasih warna background agak merah biar beda sama Main Window
        win->BeginRender(0.5f, 0.5f, 0.5f);

        // 2. Resize Aspect Ratio Scene
        //    PENTING: Agar gambar tidak gepeng di window kecil
        scene->OnResize(win->GetWidth(), win->GetHeight());

        // 3. Render Scene
        //    Menggunakan Kamera yang dipegang oleh Window tersebut
        scene->Render(dt, win->GetCamera());

        // 4. Tampilkan
        win->EndRender();
    }
}

void WindowManager::HandleResize(HWND hWnd, int width, int height)
{
    // Cari window mana yang punya HWND ini
    for (auto& win : windows)
    {
        if (win->GetHWND() == hWnd)
        {
            win->Resize(width, height);
            return; // Ketemu, selesai
        }
    }
}

GameWindow* WindowManager::CreateGameWindow(const char* title, int width, int height)
{
    // 1. Buat Window Baru
    auto newWindow = std::make_unique<GameWindow>(title, width, height);

    // 2. Ambil Raw Pointer untuk dikembalikan ke peminta (SceneGame)
    GameWindow* ptr = newWindow.get();

    // 3. Simpan ke vector (ownership pindah ke Manager)
    windows.push_back(std::move(newWindow));

    return ptr;
}

void WindowManager::DestroyWindow(GameWindow* targetWindow)
{
    // Hapus window dari vector (Otomatis men-trigger destructor GameWindow -> Close Window)
    windows.erase(
        std::remove_if(windows.begin(), windows.end(),
            [targetWindow](const std::unique_ptr<GameWindow>& p) {
                return p.get() == targetWindow;
            }),
        windows.end());
}

void WindowManager::ClearAll()
{
    windows.clear();
}