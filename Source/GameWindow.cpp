#include "GameWindow.h"
#include "System/Graphics.h"
#include "Framework.h"
#include "WindowManager.h"
#include <map>
#include <iostream>

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static std::map<HWND, WNDPROC> g_WindowProcMap;

LRESULT CALLBACK UnifiedWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    GameWindow* pWindow = (GameWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    // =========================================================
    // Cegat drag pada window yang non-draggable
    // =========================================================
    if (msg == WM_SYSCOMMAND)
    {
        if ((wParam & 0xFFF0) == SC_MOVE)
        {
            if (pWindow && !pWindow->IsDraggable())
            {
                return 0;
            }
        }
    }

    // ImGui
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
        // =========================================================
        // [FIX HANG] HAPUS SetTimer 1ms di WM_ENTERSIZEMOVE.
        //
        // Sebelumnya: WM_ENTERSIZEMOVE Å® SetTimer(1ms) Å® setiap tick
        // panggil ForceUpdateRender() Å® yang itu trigger render loop
        // Å® yang itu Present() Å® yang itu generate lebih banyak messages.
        //
        // Dengan 10+ window yang posisinya berubah setiap frame,
        // timer 1ms ini menciptakan feedback loop yang floods the
        // message queue sampai PeekMessageW di WindowManager::Update()
        // tidak pernah selesai drain-nya.
        //
        // WM_SIZE sudah cukup untuk handle resize. ForceUpdateRender
        // tidak perlu dipanggil dari sini.
        // =========================================================
    case WM_ENTERSIZEMOVE:
        return 0;

    case WM_EXITSIZEMOVE:
        return 0;

    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED)
        {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
            WindowManager::Instance().HandleResize(hWnd, width, height);
        }
        break;
    }

    // Chain call ke SDL's original WndProc
    if (g_WindowProcMap.find(hWnd) != g_WindowProcMap.end())
    {
        return CallWindowProc(g_WindowProcMap[hWnd], hWnd, msg, wParam, lParam);
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

GameWindow::GameWindow(const char* title, int w, int h)
    : width(w), height(h)
{
    std::cout << "[GameWindow] Creating Window: " << title << "..." << std::endl;
    sdlWindow = SDL_CreateWindow(title, w, h, SDL_WINDOW_RESIZABLE);
    hWnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(sdlWindow), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);

    SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)this);

    Graphics::Instance().CreateSwapChain(hWnd, width, height, swapChain.GetAddressOf());
    CreateBuffers(width, height);

    WNDPROC oldProc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)UnifiedWindowProc);
    if (oldProc) g_WindowProcMap[hWnd] = oldProc;
}

GameWindow::~GameWindow()
{
    if (hWnd) g_WindowProcMap.erase(hWnd);
    if (sdlWindow) SDL_DestroyWindow(sdlWindow);
}

void GameWindow::CreateBuffers(int w, int h)
{
    ID3D11Device* device = Graphics::Instance().GetDevice();
    renderTargetView.Reset();
    depthStencilView.Reset();

    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
    swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)backBuffer.GetAddressOf());
    device->CreateRenderTargetView(backBuffer.Get(), nullptr, renderTargetView.GetAddressOf());

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
    float color[] = { r, g, b, 1.0f };
    context->ClearRenderTargetView(renderTargetView.Get(), color);
    context->ClearDepthStencilView(depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    context->OMSetRenderTargets(1, renderTargetView.GetAddressOf(), depthStencilView.Get());
    context->RSSetViewports(1, &viewport);
}

void GameWindow::EndRender(int syncInterval)
{
    if (swapChain) swapChain->Present(syncInterval, 0);
}

void GameWindow::Resize(int w, int h)
{
    if (w <= 0 || h <= 0) return;
    if (w == width && h == height) return;

    width = w;
    height = h;

    auto context = Graphics::Instance().GetDeviceContext();
    context->OMSetRenderTargets(0, nullptr, nullptr);
    renderTargetView.Reset();
    depthStencilView.Reset();
    context->Flush();

    swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    CreateBuffers(width, height);
}

void GameWindow::SetVisible(bool visible)
{
    isVisible = visible;
    if (visible) SDL_ShowWindow(sdlWindow);
    else SDL_HideWindow(sdlWindow);
}

void GameWindow::SetTitle(const char* title)
{
    if (sdlWindow)
    {
        SDL_SetWindowTitle(sdlWindow, title);
    }
}