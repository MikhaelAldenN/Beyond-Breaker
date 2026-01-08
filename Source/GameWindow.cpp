#include "GameWindow.h"
#include "System/Graphics.h"
#include <stdio.h>
#include "Framework.h" // <--- PASTIKAN INCLUDE FRAMEWORK (sesuaikan path jika perlu, misal "../Framework.h")

// --- VARIABEL GLOBAL UNTUK HOOK ---
static WNDPROC g_SDLWndProc = nullptr;
static const UINT_PTR IDT_SUB_TIMER = 99;

// --- PROSEDUR "SATPAM" WINDOW (HOOK) ---
LRESULT CALLBACK SubWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        // Saat user mulai klik tahan title bar atau resize
    case WM_ENTERSIZEMOVE:
        SetTimer(hWnd, IDT_SUB_TIMER, 1, NULL); // Nyalakan timer 1ms
        return 0;

        // Saat user melepas klik
    case WM_EXITSIZEMOVE:
        KillTimer(hWnd, IDT_SUB_TIMER); // Matikan timer
        return 0;

        // Timer berdetak terus saat dragging (Jantung Buatan)
    case WM_TIMER:
        if (wParam == IDT_SUB_TIMER)
        {
            // PANGGIL FRAMEWORK UNTUK RENDER
            if (Framework::Instance())
            {
                Framework::Instance()->ForceUpdateRender();
            }
            return 0;
        }
        break;
    }

    // Kembalikan pesan lain ke prosedur asli SDL
    return CallWindowProc(g_SDLWndProc, hWnd, msg, wParam, lParam);
}

GameWindow::GameWindow(const char* title, int w, int h)
    : width(w), height(h)
{
    // 1. Create Physical Window (SDL)
    sdlWindow = SDL_CreateWindow(title, w, h, SDL_WINDOW_RESIZABLE);

    // Get HWND
    hWnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(sdlWindow), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);

    // 2. Setup SwapChain
    Graphics::Instance().CreateSwapChain(hWnd, width, height, swapChain.GetAddressOf());

    // 3. Create Buffers
    CreateBuffers(width, height);

    // --- 4. PASANG HOOK DISINI ---
    // Kita ganti prosedur penanganan pesan window ini dengan punya kita (SubWindowProc)
    WNDPROC oldProc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)SubWindowProc);

    // Simpan prosedur asli SDL (cukup sekali saja karena semua window SDL class-nya sama)
    if (g_SDLWndProc == nullptr) g_SDLWndProc = oldProc;
}

GameWindow::~GameWindow()
{
    if (sdlWindow) SDL_DestroyWindow(sdlWindow);
}

void GameWindow::CreateBuffers(int w, int h)
{
    ID3D11Device* device = Graphics::Instance().GetDevice();
    HRESULT hr;

    // --- RENDER TARGET VIEW ---
    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
    swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)backBuffer.GetAddressOf());
    device->CreateRenderTargetView(backBuffer.Get(), nullptr, renderTargetView.GetAddressOf());

    // --- DEPTH STENCIL VIEW ---
    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = w;
    depthDesc.Height = h;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> depthTex;
    device->CreateTexture2D(&depthDesc, nullptr, depthTex.GetAddressOf());
    device->CreateDepthStencilView(depthTex.Get(), nullptr, depthStencilView.GetAddressOf());

    // --- VIEWPORT ---
    viewport.Width = (float)w;
    viewport.Height = (float)h;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
}

void GameWindow::BeginRender(float r, float g, float b)
{
    auto context = Graphics::Instance().GetDeviceContext();

    // 1. Clear Screen
    float color[] = { r, g, b, 1.0f };
    context->ClearRenderTargetView(renderTargetView.Get(), color);
    context->ClearDepthStencilView(depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    // 2. SET RENDER TARGET (CRITICAL STEP)
    // This tells the GPU: "Draw everything to THIS window now"
    context->OMSetRenderTargets(1, renderTargetView.GetAddressOf(), depthStencilView.Get());
    context->RSSetViewports(1, &viewport);
}

void GameWindow::EndRender(int syncInterval)
{
    if (swapChain)
    {
        swapChain->Present(syncInterval, 0);
    }
}

void GameWindow::Resize(int w, int h)
{
    if (w <= 0 || h <= 0) return;
    if (w == width && h == height) return;

    width = w;
    height = h;

    auto context = Graphics::Instance().GetDeviceContext();

    // Unbind
    context->OMSetRenderTargets(0, nullptr, nullptr);
    renderTargetView.Reset();
    depthStencilView.Reset();
    context->Flush();

    // Resize SwapChain
    swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);

    // Recreate Views
    CreateBuffers(width, height);
}