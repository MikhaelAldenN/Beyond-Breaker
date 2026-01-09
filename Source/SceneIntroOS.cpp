#include "SceneIntroOS.h"
#include "SceneTitle.h" // Tujuan akhir
#include "Framework.h"
#include "System/Graphics.h"
#include "System/Input.h"

std::unique_ptr<BitmapFont> osFont;

SceneIntroOS::SceneIntroOS()
{
    // Setup Camera 1920x1080
    camera = std::make_unique<Camera>();
    camera->SetOrthographic(1920.0f, 1080.0f, 0.1f, 1000.0f);

    osFont = std::make_unique<BitmapFont>(
        "Data/Font/IBM_VGA_32px_0.png",
        "Data/Font/IBM_VGA_32px.fnt"
    );

    spriteOSLogo = std::make_unique<Sprite>(
        Graphics::Instance().GetDevice(),
        "Data/Sprite/sprite_Logo_BRICK-DOS.png"
    );

    primitiveBatch = std::make_unique<Primitive>(Graphics::Instance().GetDevice());

    // --- 1. SETUP BLEND STATE (Agar PNG Transparan tidak jadi Putih) ---
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = FALSE;
    blendDesc.RenderTarget[0].BlendEnable = TRUE; // <--- WAJIB TRUE
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    Graphics::Instance().GetDevice()->CreateBlendState(&blendDesc, blendState.GetAddressOf());


    // --- SETUP ANIMASI TABRAKAN ---
    float screenWidth = 1920.0f;
    float logoWidth = 850.0f;

    // Target X (Tengah Horizontal)
    logoTargetX = (screenWidth * 0.5f) - (logoWidth * 0.5f);

    // --- SETUP LOGO 1 (ATAS - DARI KIRI) ---
    logo1_StartX = -logoWidth;             // Mulai dari luar kiri
    logo1_CurrentX = logo1_StartX;

    // Posisi Y: Kita taruh agak ke atas dari tengah (370 - offset)
    // Misal kita geser ke atas 60 pixel
    logo1_Y = 370.0f;

    // --- SETUP LOGO 2 (BAWAH - DARI KANAN) ---
    logo2_StartX = screenWidth;            // Mulai dari luar kanan
    logo2_CurrentX = logo2_StartX;

    // Posisi Y: Kita taruh agak ke bawah dari tengah (370 + offset)
    // Misal kita geser ke bawah 60 pixel
    logo2_Y = 370.0f + 3.0f;
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

    // --- LOGIC ANIMASI CRASH ---
    if (animTimer < ANIM_DURATION)
    {
        animTimer += elapsedTime;
        float t = animTimer / ANIM_DURATION;
        if (t > 1.0f) t = 1.0f;

        // Hanya update X (Horizontal Movement)
        logo1_CurrentX = Lerp(logo1_StartX, logoTargetX, t);
        logo2_CurrentX = Lerp(logo2_StartX, logoTargetX, t);
    }
    else
    {
        // Kunci posisi di tengah
        logo1_CurrentX = logoTargetX;
        logo2_CurrentX = logoTargetX;
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

    float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    dc->OMSetBlendState(blendState.Get(), blendFactor, 0xFFFFFFFF);

    if (primitiveBatch)
    {

        // UKURAN BARU: 1312 x 984 di Tengah Layar (304, 48)
        primitiveBatch->Rect(
            304.0f, 48.0f,    // Posisi X, Y (Kiri Atas)
            1312.0f, 984.0f,  // Ukuran Lebar, Tinggi
            0.0f, 0.0f,       // Pivot Center
            0.0f,             // Rotasi
            0.96f, 0.80f, 0.23f, 1.0f     // Warna
        );

        primitiveBatch->Render(dc);
    }

    // Render Logo di tengah layar
    if (spriteOSLogo && isVisible)
    {
        // LOGO 1 (Jalur Atas)
        spriteOSLogo->Render(
            dc,
            logo1_CurrentX, logo1_Y, // Pakai Y khusus logo 1
            0.0f, 850, 105,
            0.0f,
            1.0f, 1.0f, 1.0f, 1.0f
        );

        // LOGO 2 (Jalur Bawah)
        // Note: Kita HAPUS kondisi 'if timer < duration' 
        // karena sekarang mereka posisinya beda, jadi dua-duanya harus selalu digambar.
        spriteOSLogo->Render(
            dc,
            logo2_CurrentX, logo2_Y, // Pakai Y khusus logo 2
            0.0f, 850, 105,
            0.0f,
            1.0f, 1.0f, 1.0f, 1.0f
        );
    }


    if (osFont)
    {
        float fontSize = 0.625f;
        float fontColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

        //osFont->Draw("Copyright (c) Comreba Corporation, 1987. All Rights Reserved.",
        //debugFontPosX, debugFontPosY,
        //fontSize,
        //debugFontColor[0], debugFontColor[1], debugFontColor[2], debugFontColor[3]);

        osFont->Draw("BRICK-DOS Opereating System\n        Version 1.2",
            733.5f, 593.0f,
            fontSize,
            fontColor[0], fontColor[1], fontColor[2], fontColor[3]);
        
        osFont->Draw("Copyright (c) Comreba Corporation, 1987. All Rights Reserved.\n    BRICK-DOS is a registered trademark of Comreba Corp.",
            450.0f, 903.0f,
            fontSize,
            fontColor[0], fontColor[1], fontColor[2], fontColor[3]);


    }
}

void SceneIntroOS::DrawGUI()
{
    //// Membuat window debug baru
    //ImGui::Begin("Font Debugger");

    //ImGui::Text("Adjust Font Transform:");

    //// Slider untuk Posisi (Range 0 sampai resolusi layar)
    //ImGui::SliderFloat("Pos X", &debugFontPosX, 0.0f, 1280.0f);
    //ImGui::SliderFloat("Pos Y", &debugFontPosY, 0.0f, 1080.0f);

    //// Slider untuk Ukuran (Scale)
    //ImGui::SliderFloat("Font Scale", &debugFontSize, 0.1f, 5.0f);

    //ImGui::Separator();

    //// Color Picker untuk Warna Font
    //ImGui::Text("Font Color:");
    //ImGui::ColorEdit4("Color", debugFontColor);

    //// Tombol Reset jika posisi berantakan
    //if (ImGui::Button("Reset Position"))
    //{
    //    debugFontPosX = 800.0f;
    //    debugFontPosY = 50.0f;
    //    debugFontSize = 1.0f;
    //}

    //ImGui::End();
}

void SceneIntroOS::OnResize(int width, int height)
{
    if (camera) camera->SetAspectRatio((float)width / (float)height);
}