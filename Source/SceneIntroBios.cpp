#include "SceneIntroBios.h"
#include "SceneIntroOS.h"
#include "SceneTitle.h"
#include "Framework.h"
#include "System/Input.h"
#include "System/Graphics.h"

std::unique_ptr<BitmapFont> biosFont;

SceneIntroBios::SceneIntroBios()
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

    spriteLogoBoot = std::make_unique<Sprite>(
        Graphics::Instance().GetDevice(),
        "Data/Sprite/Scene Intro/Sprite_OrangSoloItu.png"
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

    // --- SETUP TYPEWRITER ---
    biosLogSystem = std::make_unique<Typewriter>();

    // Definisikan warna & ukuran font di sini biar rapi
    float fSize = 0.625f;
    float yellow[4] = { 0.96f, 0.80f, 0.23f, 1.0f };
    float red[4] = { 1.0f, 0.2f, 0.2f, 1.0f }; // Untuk error/warning

    // Masukkan data text yang tadi ada di Render
    biosLogSystem->AddLine(
        "BEYOND BREAKER - SYSTEM BOOT LOG \n\nBEYOND Modular BIOS v9.70BB, An Energy Star Ally \nCopyright (C) 1987-2026, Beyond Breaker Software, Inc. ",
        406.5f, 57.4f, fSize, yellow
    );

    biosLogSystem->AddLine(
        "Version BB-4TH-WALL\nNEURAL-LINK CPU at 444MHz",
        298.0f, 175.74f, fSize, yellow
    );

    // INI DIA BINTANG UTAMANYA:
    // AddMemoryTestLine(Prefix, Start, End, Suffix, X, Y, Size, Color)
    biosLogSystem->AddMemoryTestLine(
        "Memory Test : ",
        0,
        262144,
        3.0f,              // <--- Parameter DURASI (Detik)
        "KB OK",
        740.0f, 175.74f + 23.5f,
        fSize,
        yellow
    );

    // Lanjut baris berikutnya
    biosLogSystem->AddLine(
        "(Reference: \"640K ought to be enough for anybody\")",
        298.0f, 235.0f, fSize, yellow // Sesuaikan Y lagi
    );

    biosLogSystem->AddLine(
        "Beyond Plug and Play BIOS Extension v1.0A\nCopyright (C) 2026, Beyond Breaker Software, Inc.",
        298.0f, 296.79f, fSize, yellow
    );

    biosLogSystem->AddLine("Detecting Primary Master    ... BEYOND_OS_HD", 334.7f, 344.31f, fSize, yellow);
    biosLogSystem->AddLine("Detecting Primary Slave     ... NONE", 334.7f, 368.5f, fSize, yellow);
    biosLogSystem->AddLine("Detecting Secondary Master  ... BLOCK_DATABASE", 334.7f, 392.5f, fSize, yellow);

    // Contoh beda warna
    biosLogSystem->AddLine("Detecting Secondary Slave   ... [!] WARNING: ANOMALY_DETECTED", 334.7f, 416.5f, fSize, yellow);

    biosLogSystem->AddLine(
        "[CRITICAL ALERT] Sector 0x004 is bleeding.\nThe blocks are no longer static. The Wall is thinner than you think.",
        298.7f, 560.0f, fSize, yellow // Pakai warna MERAH
    );

    //biosLogSystem->AddLine(
    //    "Press DEL to enter SETUP Press F1 to ESCAPE THE GRID\n09/01/2026-BB-586B-W877-2A5LEF09C-00",
    //    298.7f, 973.5f, fSize, yellow
    //);
}

void SceneIntroBios::Update(float elapsedTime)
{
    // 1. Jika sedang fase Black Screen (Exiting)
    if (isExiting)
    {
        exitTimer += elapsedTime;
        if (exitTimer >= EXIT_DELAY)
        {
            // Waktu habis, pindah ke OS Scene
            Framework::Instance()->ChangeScene(std::make_unique<SceneIntroOS>());
        }
        return; // Stop update logic lainnya
    }

    // 2. Logic Normal (BIOS Text berjalan)
    if (biosLogSystem)
    {
        biosLogSystem->Update(elapsedTime);
    }

    // 3. Logic Input (Enter) atau Auto-Finish
    // Cek apakah user tekan Enter
    if (Input::Instance().GetKeyboard().IsTriggered(VK_RETURN))
    {
        // Jika teks BIOS belum selesai -> Percepat/Skip Text
        if (biosLogSystem && !biosLogSystem->IsFinished())
        {
            biosLogSystem->SkipCurrentLine();
        }
        // Jika teks SUDAH selesai -> Masuk fase Black Screen
        else
        {
            isExiting = true;
        }
    }

    // Opsi Tambahan: Kalau teks sudah selesai sendiri tanpa ditekan Enter
    // Tambahkan delay dikit biar gak kaget, lalu masuk black screen
    /*
    if (biosLogSystem && biosLogSystem->IsFinished()) {
        // logic auto exit disini
    }
    */
}

void SceneIntroBios::Render(float dt, Camera* targetCamera)
{
    auto dc = Graphics::Instance().GetDeviceContext();

    if (isExiting) return;

    // --- 2. AKTIFKAN BLEND STATE SEBELUM GAMBAR FONT ---
        // Parameter: (BlendState, BlendFactor, SampleMask)
    float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    dc->OMSetBlendState(blendState.Get(), blendFactor, 0xFFFFFFFF);


    // PERBAIKAN: Ambil ukuran layar dari Framework -> MainWindow
    float sw = 1920;
    float sh = 1080;

    //    // Render Sprite
    //if (bgSpriteIntro)
    //{
    //    // PERBAIKAN: Sesuaikan argumen dengan Sprite.h yang kamu upload
    //    // Render(dc, x, y, z, w, h, angle, r, g, b, a)
    //    bgSpriteIntro->Render(
    //        dc,             // Device Context
    //        0, 0,           // Posisi X, Y (Kiri Atas)
    //        0.5f,              // Posisi Z (Depth)
    //        sw, sh,         // Lebar & Tinggi (Fullscreen)
    //        0.0f,           // Rotasi (Angle)
    //        1.0f, 1.0f, 1.0f, 0.2f // Warna (Putih/Normal)
    //    );
    //}

    if (spriteLogoBoot)
    {
		spriteLogoBoot->Render(
			dc,             // Device Context
            298.0f, 57.4f,           // Posisi X, Y (Kiri Atas)
			0.0f,              // Posisi Z (Depth)
			93, 93,         // Lebar & Tinggi (Fullscreen)
			0.0f,           // Rotasi (Angle)
			1.0f, 1.0f, 1.0f, 1.0f // Warna (Putih/Normal)
		);
    }

    //if (biosFont)
    //{
    //    // Note: REFACTOR THIS LATER!!
    //    float fontSize = 0.625f;
    //    float fontColor[4] = { 0.96f, 0.80f, 0.23f, 1.0f };


    //    biosFont->Draw("BEYOND BREAKER - SYSTEM BOOT LOG \n\nBEYOND Modular BIOS v9.70BB, An Energy Star Ally \nCopyright (C) 1987-2026, Beyond Breaker Software, Inc. ",
    //        406.5f, 57.4f,
    //        fontSize,
    //        fontColor[0], fontColor[1], fontColor[2], fontColor[3]);

    //    biosFont->Draw("Version BB-4TH-WALL\n\nNEURAL-LINK CPU at 444MHz Memory Test : 640K OK\n(Reference: \"640K ought to be enough for anybody\")",
    //        298.0f, 175.74f,
    //        fontSize,
    //        fontColor[0], fontColor[1], fontColor[2], fontColor[3]);

    //    biosFont->Draw("Beyond Plug and Play BIOS Extension v1.0A\nCopyright (C) 2026, Beyond Breaker Software, Inc.",
    //        298.0f, 296.79f,
    //        fontSize,
    //        fontColor[0], fontColor[1], fontColor[2], fontColor[3]);

    //    biosFont->Draw("Detecting Primary Master    ... BEYOND_OS_HD",
    //        334.7f, 344.31f,
    //        fontSize,
    //        fontColor[0], fontColor[1], fontColor[2], fontColor[3]);

    //    biosFont->Draw("Detecting Primary Slave     ... NONE",
    //        334.7f, 368.5f,
    //        fontSize,
    //        fontColor[0], fontColor[1], fontColor[2], fontColor[3]);

    //    biosFont->Draw("Detecting Secondary Master  ... BLOCK_DATABASE",
    //        334.7f, 392.5f,
    //        fontSize,
    //        fontColor[0], fontColor[1], fontColor[2], fontColor[3]);

    //    biosFont->Draw("Detecting Secondary Slave   ... [!] WARNING: ANOMALY_DETECTED",
    //        334.7f, 416.5f,
    //        fontSize,
    //        fontColor[0], fontColor[1], fontColor[2], fontColor[3]);

    //    biosFont->Draw("[CRITICAL ALERT] Sector 0x004 is bleeding.\nThe blocks are no longer static. The Wall is thinner than you think.",
    //        298.7f, 560.0f,
    //        fontSize,
    //        fontColor[0], fontColor[1], fontColor[2], fontColor[3]);

    //    biosFont->Draw("Press DEL to enter SETUP Press F1 to ESCAPE THE GRID\n09/01/2026-BB-586B-W877-2A5LEF09C-00",
    //        298.7f, 973.5f,
    //        fontSize,
    //        fontColor[0], fontColor[1], fontColor[2], fontColor[3]);
    //}

    if (biosFont)
    {
            float fontSize = 0.625f;
            float fontColor[4] = { 0.96f, 0.80f, 0.23f, 1.0f };

            biosFont->Draw("Press DEL to enter SETUP Press F1 to ESCAPE THE GRID\n09/01/2026-BB-586B-W877-2A5LEF09C-00",
            298.7f, 973.5f,
            fontSize,
            fontColor[0], fontColor[1], fontColor[2], fontColor[3]);

    }

    if (biosFont && biosLogSystem)
    {
        //float fontSize = 0.625f;
        //float fontColor[4] = { 0.96f, 0.80f, 0.23f, 1.0f };

        //// this is for debug
        //biosFont->Draw("Press DEL",
        //debugFontPosX, debugFontPosY,
        //fontSize,
        //debugFontColor[0], debugFontColor[1], debugFontColor[2], debugFontColor[3]);

        // GANTI SEMUA LOGIC TEXT LAMA DENGAN INI:
        biosLogSystem->Render(biosFont.get());
    }

}

void SceneIntroBios::OnResize(int width, int height)
{
    if (camera && height > 0)
    {
        camera->SetAspectRatio((float)width / (float)height);
    }
}

void SceneIntroBios::DrawGUI()
{
    // Membuat window debug baru
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