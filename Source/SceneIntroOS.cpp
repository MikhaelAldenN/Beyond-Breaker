#include "SceneIntroOS.h"
#include "SceneTitle.h" // Tujuan akhir
#include "Framework.h"
#include "System/Graphics.h"
#include "System/Input.h"

SceneIntroOS::SceneIntroOS()
{
    // Setup Camera
    camera = std::make_unique<Camera>();
    camera->SetOrthographic(1280.0f, 720.0f, 0.1f, 1000.0f);

    // Load Gambar Logo OS
    spriteOSLogo = std::make_unique<Sprite>(
        Graphics::Instance().GetDevice(),
        "Data/Sprite/Scene Intro/Sprite_KuterOS.png" // Pastikan file ini ada
    );

    primitiveBatch = std::make_unique<Primitive>(Graphics::Instance().GetDevice());
}

void SceneIntroOS::Update(float elapsedTime)
{
    timer += elapsedTime;

    // --- LOGIC LOADING ---
    // Contoh: Logo kedip-kedip dikit (Indicator loading)
    blinkTimer += elapsedTime;
    if (blinkTimer > 0.1f) // Kedip cepat ala HDD led
    {
        blinkTimer = 0;
        // isVisible = !isVisible; // Uncomment kalau mau efek strobo
    }

    // Jika sudah 3 detik ATAU user tekan Enter (Skip)
    if (timer >= LOADING_DURATION || Input::Instance().GetKeyboard().IsTriggered(VK_RETURN))
    {
        // Pindah ke Title Screen yang sebenarnya
        Framework::Instance()->ChangeScene(std::make_unique<SceneTitle>());
    }
}

void SceneIntroOS::Render(float dt, Camera* targetCamera)
{
    auto dc = Graphics::Instance().GetDeviceContext();

    if (primitiveBatch)
    {
        // Warna Kuning BRICK-DOS
        float r = 245.0f / 255.0f;
        float g = 204.0f / 255.0f;
        float b = 59.0f / 255.0f;

        // UKURAN BARU: 1312 x 984 di Tengah Layar (304, 48)
        primitiveBatch->Rect(
            304.0f, 48.0f,    // Posisi X, Y (Kiri Atas)
            1312.0f, 984.0f,  // Ukuran Lebar, Tinggi
            0.0f, 0.0f,       // Pivot Center
            0.0f,             // Rotasi
            r, g, b, 1.0f     // Warna
        );

        primitiveBatch->Render(dc);
    }

    // Render Logo di tengah layar
    //if (spriteOSLogo && isVisible)
    //{
    //    // Asumsi posisi tengah (Center of screen 1280x720 is 640, 360)
    //    // Sesuaikan ukuran W dan H dengan ukuran asli gambarmu
    //    spriteOSLogo->Render(
    //        dc,
    //        0, 0,
    //        0.0f,
    //        1920, 1080,         // Ukuran Gambar
    //        0.0f,
    //        1.0f, 1.0f, 1.0f, 1.0f
    //    );
    //}
}

void SceneIntroOS::OnResize(int width, int height)
{
    if (camera) camera->SetAspectRatio((float)width / (float)height);
}