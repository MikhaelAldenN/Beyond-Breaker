#include "SceneIntro.h"
#include "SceneTitle.h"
#include "Framework.h"
#include "System/Input.h"
#include "System/Graphics.h"

SceneIntro::SceneIntro()
{
    // Setup Camera 2D (Resolusi standar 1280x720)
    camera = std::make_unique<Camera>();
    camera->SetOrthographic(1280.0f, 720.0f, 0.1f, 1000.0f);
}

void SceneIntro::Update(float elapsedTime)
{
    // CONTOH LOGIC:
    // Nanti di sini kamu bisa update timer, misal:
    // bootTimer += elapsedTime;
    // if (bootTimer > 5.0f) { PindahScene(); }

    // SEMENTARA: Tekan Enter untuk lanjut ke Title
    if (Input::Instance().GetKeyboard().IsTriggered(VK_RETURN))
    {
        Framework::Instance()->ChangeScene(std::make_unique<SceneTitle>());
    }
}

void SceneIntro::Render(float dt, Camera* targetCamera)
{
    // Area ini untuk render gambar/teks boot nanti.
    // Sekarang biarkan kosong, layar akan mengikuti warna Clear di Framework (biasanya hitam/abu).

    // Contoh cara ambil ukuran layar (persiapan untuk nanti):
    /*
    auto dc = Graphics::Instance().GetDeviceContext();
    float sw = 1280.0f;
    float sh = 720.0f;
    if (auto window = Framework::Instance()->GetMainWindow())
    {
        sw = static_cast<float>(window->GetWidth());
        sh = static_cast<float>(window->GetHeight());
    }
    */
}

void SceneIntro::OnResize(int width, int height)
{
    if (camera && height > 0)
    {
        camera->SetAspectRatio((float)width / (float)height);
    }
}

void SceneIntro::DrawGUI()
{
    // Kosong dulu
}