#pragma once

#include <SDL3/SDL.h>
#include <d3d11.h>
#include <wrl.h>
#include "Camera.h"

class GameWindow
{
public:
    GameWindow(const char* title, int width, int height);
    ~GameWindow();

    void SetCamera(Camera* cam) { this->targetCamera = cam; }
    Camera* GetCamera() const { return targetCamera; }

    void BeginRender(float r = 0.0f, float g = 0.0f, float b = 0.0f);
    void EndRender(int syncInterval = 1);
    void Resize(int width, int height);

    SDL_Window* GetSDLWindow() { return sdlWindow; }
    HWND GetHWND() { return hWnd; }
    void* GetHandle() const { return (void*)hWnd; }
    int GetWidth() const { return width; }
    int GetHeight() const { return height; }

    void SetPriority(int p) { priority = p; }
    int GetPriority() const { return priority; }

    void SetVisible(bool visible);
    bool IsVisible() const { return isVisible; }

    void SetDraggable(bool enable) { isDraggable = enable; }
    bool IsDraggable() const { return isDraggable; }
    void SetTitle(const char* title);

    void SetTargetFPS(float fps)
    {
        if (fps > 0.0f) m_renderInterval = 1.0f / fps;
        else m_renderInterval = 0.0f;
    }

    bool ShouldRender(float dt)
    {
        if (m_renderInterval <= 0.0f) return true;

        m_renderTimer += dt;
        if (m_renderTimer >= m_renderInterval)
        {
            m_renderTimer = 0.0f;
            return true;
        }
        return false;
    }

    // =========================================================
    // Sentinel value: window dengan priority ini TIDAK ikut
    // z-order sorting di EnforceWindowPriorities.
    // Main window dan debug window pakai nilai ini.
    // =========================================================
    static constexpr int PRIORITY_SENTINEL = 9999;

private:
    void CreateBuffers(int w, int h);

private:
    SDL_Window* sdlWindow = nullptr;
    HWND hWnd = nullptr;
    int width = 0;
    int height = 0;

    Camera* targetCamera = nullptr;

    Microsoft::WRL::ComPtr<IDXGISwapChain>          swapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>  renderTargetView;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView>  depthStencilView;
    D3D11_VIEWPORT                                  viewport = {};

    // =========================================================
    // [FIX] Default priority = SENTINEL.
    // Sebelumnya 100, dan filter di EnforceWindowPriorities pakai
    // `< 100` untuk exclude main window. Itu fragile: setiap
    // sub-window yang dikasih priority >= 100 secara tidak sengaja
    // akan ikut-ikutan di-exclude.
    //
    // Sekarang: default = 9999 (sentinel). Main window yang tidak
    // pernah di-SetPriority akan tetap di 9999 dan otomatis
    // ter-exclude. Sub-window yang di-SetPriority ke nilai
    // apapun < 9999 akan ikut sorting.
    // =========================================================
    int priority = PRIORITY_SENTINEL;

    bool isVisible = true;
    bool isDraggable = true;

    float m_renderInterval = 0.0f;
    float m_renderTimer = 0.0f;
};