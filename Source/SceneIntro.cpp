#include "SceneIntro.h"
#include "SceneTitle.h"
#include "Framework.h"
#include "System/Input.h"
#include "System/Graphics.h"

std::unique_ptr<BitmapFont> biosFont;

SceneIntro::SceneIntro()
{
    // Setup Camera 2D (Resolusi standar 1280x720)
    camera = std::make_unique<Camera>();
    camera->SetOrthographic(1280.0f, 720.0f, 0.1f, 1000.0f);

    biosFont = std::make_unique<BitmapFont>(
        "Data/Font/IBM_VGA_32px_0.png",
        "Data/Font/IBM_VGA_32px.fnt"
    );

    bgSpriteIntro = std::make_unique<Sprite>(
        Graphics::Instance().GetDevice(),
        "Data/Sprite/Placeholder/[PLACEHOLDER]Back_Boot.png"
    );


    // --- 1. BUAT BLEND STATE (Hanya sekali di awal) ---
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = FALSE;
    blendDesc.RenderTarget[0].BlendEnable = TRUE; // Aktifkan Blending!

    // Rumus Standard Alpha Blending (SrcAlpha, InvSrcAlpha)
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    auto device = Graphics::Instance().GetDevice();
    HRESULT hr = device->CreateBlendState(&blendDesc, blendState.GetAddressOf());
}

void SceneIntro::Update(float elapsedTime)
{
    // SEMENTARA: Tekan Enter untuk lanjut ke Title
    if (Input::Instance().GetKeyboard().IsTriggered(VK_RETURN))
    {
        Framework::Instance()->ChangeScene(std::make_unique<SceneTitle>());
    }
}

void SceneIntro::Render(float dt, Camera* targetCamera)
{
    auto dc = Graphics::Instance().GetDeviceContext();

    // --- 2. AKTIFKAN BLEND STATE SEBELUM GAMBAR FONT ---
        // Parameter: (BlendState, BlendFactor, SampleMask)
    float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    dc->OMSetBlendState(blendState.Get(), blendFactor, 0xFFFFFFFF);


    // PERBAIKAN: Ambil ukuran layar dari Framework -> MainWindow
    float sw = 1920;
    float sh = 1080;

        // Render Sprite
    if (bgSpriteIntro)
    {
        // PERBAIKAN: Sesuaikan argumen dengan Sprite.h yang kamu upload
        // Render(dc, x, y, z, w, h, angle, r, g, b, a)
        bgSpriteIntro->Render(
            dc,             // Device Context
            0, 0,           // Posisi X, Y (Kiri Atas)
            0.5f,              // Posisi Z (Depth)
            sw, sh,         // Lebar & Tinggi (Fullscreen)
            0.0f,           // Rotasi (Angle)
            1.0f, 1.0f, 1.0f, 0.2f // Warna (Putih/Normal)
        );
    }

    if (biosFont)
    {
        // this is for debug
        biosFont->Draw("BEYOND BREAKER - SYSTEM BOOT LOG",
            debugFontPosX, debugFontPosY,
            debugFontSize,
            debugFontColor[0], debugFontColor[1], debugFontColor[2], debugFontColor[3]);


        // Note: REFACTOR THIS LATER!!
        float fontSize = 0.417f;
        float fontColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

        biosFont->Draw("BEYOND BREAKER - SYSTEM BOOT LOG",
            271.0f, 38.5f,
            fontSize,
            fontColor[0], fontColor[1], fontColor[2], fontColor[3]);

        biosFont->Draw("BEYOND BREAKER - SYSTEM BOOT LOG",
            271.0f, 38.5f,
            fontSize,
            fontColor[0], fontColor[1], fontColor[2], fontColor[3]);


    }

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
    // Membuat window debug baru
    ImGui::Begin("Font Debugger");

    ImGui::Text("Adjust Font Transform:");

    // Slider untuk Posisi (Range 0 sampai resolusi layar)
    ImGui::SliderFloat("Pos X", &debugFontPosX, 0.0f, 1280.0f);
    ImGui::SliderFloat("Pos Y", &debugFontPosY, 0.0f, 720.0f);

    // Slider untuk Ukuran (Scale)
    ImGui::SliderFloat("Font Scale", &debugFontSize, 0.1f, 5.0f);

    ImGui::Separator();

    // Color Picker untuk Warna Font
    ImGui::Text("Font Color:");
    ImGui::ColorEdit4("Color", debugFontColor);

    // Tombol Reset jika posisi berantakan
    if (ImGui::Button("Reset Position"))
    {
        debugFontPosX = 800.0f;
        debugFontPosY = 50.0f;
        debugFontSize = 1.0f;
    }

    ImGui::End();
}