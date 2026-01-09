#include "SceneTitle.h"
#include "SceneGameBreaker.h"
#include "Framework.h"       // PENTING: Untuk akses ukuran layar (Window)
#include "System/Input.h"
#include "System/Graphics.h" // PENTING: Untuk akses Device & Context

SceneTitle::SceneTitle()
{
    // 1. Setup Camera (Dummy)
    camera = std::make_unique<Camera>();
    camera->SetOrthographic(1280.0f, 720.0f, 0.1f, 1000.0f);

    // 2. Load Sprite
    // PERBAIKAN: Sprite butuh 'Device' sebagai parameter pertama
    bgSprite = std::make_unique<Sprite>(
        Graphics::Instance().GetDevice(),
        "Data/Sprite/TitleAlternate.png"
    );
}

void SceneTitle::Update(float elapsedTime)
{
    // Logika pindah scene
    if (Input::Instance().GetKeyboard().IsTriggered(VK_RETURN))
    {
        Framework::Instance()->ChangeScene(std::make_unique<SceneGameBreaker>());
    }
}

void SceneTitle::Render(float dt, Camera* targetCamera)
{
    // Ambil DeviceContext dari Graphics
    auto dc = Graphics::Instance().GetDeviceContext();

    // PERBAIKAN: Ambil ukuran layar dari Framework -> MainWindow
    float sw = 1280.0f;
    float sh = 720.0f;

    // Cek apakah window ada, untuk update ukuran dinamis
    if (auto window = Framework::Instance()->GetMainWindow())
    {
        sw = static_cast<float>(window->GetWidth());
        sh = static_cast<float>(window->GetHeight());
    }

    // Render Sprite
    if (bgSprite)
    {
        // PERBAIKAN: Sesuaikan argumen dengan Sprite.h yang kamu upload
        // Render(dc, x, y, z, w, h, angle, r, g, b, a)
        bgSprite->Render(
            dc,             // Device Context
            0, 0,           // Posisi X, Y (Kiri Atas)
            0,              // Posisi Z (Depth)
            sw, sh,         // Lebar & Tinggi (Fullscreen)
            0.0f,           // Rotasi (Angle)
            1.0f, 1.0f, 1.0f, 1.0f // Warna (Putih/Normal)
        );
    }
}

void SceneTitle::OnResize(int width, int height)
{
    if (camera && height > 0)
    {
        camera->SetAspectRatio((float)width / (float)height);
    }
}

void SceneTitle::DrawGUI()
{
    // Biarkan kosong dulu tidak apa-apa jika belum ada UI (ImGui)
    // Fungsi ini harus tetap ada untuk memenuhi syarat virtual function
}