#include "SceneTitle.h"
#include "SceneGameBreaker.h"
#include "Framework.h"       // PENTING: Untuk akses ukuran layar (Window)
#include "System/Input.h"
#include "System/Graphics.h" // PENTING: Untuk akses Device & Context

SceneTitle::SceneTitle()
{
    // 1. Setup Camera
    camera = std::make_unique<Camera>();
    camera->SetPerspectiveFov(DirectX::XMConvertToRadians(45), 1920.0f / 1080.0f, 0.1f, 1000.0f);

    // [FIX] Pindah ke ATAS (Y=20), bukan ke BELAKANG (Z=-20)
    // Posisi mata: (0, 20, 0.1) -> Kasih sedikit Z (0.1) biar matematika LookAt-nya gak error (Divide by Zero)
    camera->SetPosition(0.0f, 20.0f, 0.0f);

    // [FIX] Suruh kamera melihat ke TITIK NOL (Lantai)
    // Parameter: Posisi Mata, Posisi Target, Up Vector
    // PENTING: Kalau kita nunduk 90 derajat, 'Atas' kepala kita menghadap ke Z Positif.
    camera->SetLookAt(
        { 0.0f, 20.0f, 0.0f },
        { 0.0f,  0.0f, 0.0f },
        { 0.0f,  0.0f, 0.0f }  // Up Vector jadi Z+ karena kita nunduk
    );

    // 2. Load Sprite
    bgSprite = std::make_unique<Sprite>(
        Graphics::Instance().GetDevice(),
        "Data/Sprite/Placeholder/[PLACEHOLDER]Back_Title.png"
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
    // 1. Ambil Context & RenderState
    auto dc = Graphics::Instance().GetDeviceContext();
    auto rs = Graphics::Instance().GetRenderState();

    // 2. SETUP RENDER STATE
    // [FIX] Gunakan fungsi helper yang ada di Graphics.h untuk transparansi
    dc->OMSetBlendState(Graphics::Instance().GetAlphaBlendState(), nullptr, 0xFFFFFFFF);

    // [FIX] Gunakan enum yang PASTI ADA (dari file SceneGameBreaker yang kamu upload sebelumnya)
    // Jangan menebak "DepthRead", pakai TestAndWrite dulu agar compile sukses.
    dc->OMSetDepthStencilState(rs->GetDepthStencilState(DepthState::TestAndWrite), 0);

    // [FIX] Gunakan enum yang PASTI ADA.
    // Jika sprite tidak muncul (hilang), berarti kita butuh "CullNone", 
    // tapi kamu harus cek file RenderState.h untuk nama aslinya.
    dc->RSSetState(rs->GetRasterizerState(RasterizerState::SolidCullNone));

    // --- RUMUS SKALA OTOMATIS ---
        // Kita ingin gambar pas layar pada jarak kamera saat ini.
        // Rumus Trigonometri: Tinggi = 2 * Jarak * tan(FOV / 2)

    float distance = 20.0f; // Jarak target (sesuai posisi awal kamera)

    // Pastikan ini sama dengan settingan FOV di Constructor (45 derajat)
    float fov = DirectX::XMConvertToRadians(45.0f);

    float worldH = 2.0f * distance * tanf(fov / 2.0f);
    float worldW = worldH * (1920.0f / 1080.0f); // Kalikan Aspect Ratio

    if (bgSprite)
    {
        // [FIX] Rotasi Sprite agar TIDUR di lantai
        // Pitch = 90 Derajat (XM_PIDIV2)
        // Yaw   = 0
        // Roll  = 0
        // (Atau kalau gambarnya kebalik, coba Pitch = -90 atau -XM_PIDIV2)

        float pitch = DirectX::XM_PIDIV2;

        bgSprite->Render(
            dc,
            camera.get(),
            0, 0, 0,
            worldW, worldH,
            pitch, 0.0f, 0.0f, // ROTASI DI SINI
            1.0f, 1.0f, 1.0f, 1.0f
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
    // Buka Window ImGui
    ImGui::Begin("Scene Title Debug");

    if (camera)
    {
        ImGui::Text("Camera Controls");
        ImGui::Separator();

        // --- 1. POSISI (POSITION) ---
        // Ambil data posisi saat ini
        DirectX::XMFLOAT3 pos = camera->GetPosition();
        float p[3] = { pos.x, pos.y, pos.z };

        // Tampilkan Drag Float (Geser angka untuk ubah nilai)
        // Parameter: Label, ArrayFloat, Kecepatan Geser
        if (ImGui::DragFloat3("Position (X,Y,Z)", p, 0.1f))
        {
            // Jika user menggeser, update posisi kamera
            camera->SetPosition(p[0], p[1], p[2]);
        }

        // --- 2. ROTASI (ROTATION) ---
        // Rotasi di kamera disimpan dalam RADIANS.
        // Kita ubah ke DEGREES supaya enak dibaca manusia.
        DirectX::XMFLOAT3 rot = camera->GetRotation();
        float rDeg[3] = {
            DirectX::XMConvertToDegrees(rot.x),
            DirectX::XMConvertToDegrees(rot.y),
            DirectX::XMConvertToDegrees(rot.z)
        };

        if (ImGui::DragFloat3("Rotation (Deg)", rDeg, 1.0f))
        {
            // Kembalikan ke RADIANS saat dikirim ke kamera
            camera->SetRotation(
                DirectX::XMConvertToRadians(rDeg[0]),
                DirectX::XMConvertToRadians(rDeg[1]),
                DirectX::XMConvertToRadians(rDeg[2])
            );
        }

        ImGui::Separator();

        // --- 3. RESET BUTTON (Opsional) ---
        if (ImGui::Button("Reset Camera"))
        {
            camera->SetPosition(0.0f, 0.0f, -20.0f);
            camera->SetRotation(0.0f, 0.0f, 0.0f);
        }
    }
    else
    {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error: Camera is Null!");
    }

    ImGui::End();
}