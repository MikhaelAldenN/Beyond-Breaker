#pragma once

#include <d3d11.h>
#include <wrl.h>
#include <memory>
#include "RenderState.h"
#include "PrimitiveRenderer.h"
#include "ShapeRenderer.h"
#include "ModelRenderer.h"

class Graphics
{
private:
    Graphics() = default;
    ~Graphics() = default;

public:
    static Graphics& Instance() { static Graphics i; return i; }

    // Initialize Device Only (No Window creation here)
    void Initialize();

    // Helper to create a SwapChain for a specific window
    void CreateSwapChain(HWND hWnd, int width, int height, IDXGISwapChain** outSwapChain);

    ID3D11Device* GetDevice() { return device.Get(); }
    ID3D11DeviceContext* GetDeviceContext() { return immediateContext.Get(); }

    // Shared Renderers (They don't hold window data, so they stay here)
    RenderState* GetRenderState() { return renderState.get(); }
    PrimitiveRenderer* GetPrimitiveRenderer() const { return primitiveRenderer.get(); }
    ShapeRenderer* GetShapeRenderer() const { return shapeRenderer.get(); }
    ModelRenderer* GetModelRenderer() const { return modelRenderer.get(); }

private:
    Microsoft::WRL::ComPtr<ID3D11Device>        device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> immediateContext;
    Microsoft::WRL::ComPtr<IDXGIFactory>        dxgiFactory; // Important for creating multiple swapchains

    std::unique_ptr<RenderState>       renderState;
    std::unique_ptr<PrimitiveRenderer> primitiveRenderer;
    std::unique_ptr<ShapeRenderer>     shapeRenderer;
    std::unique_ptr<ModelRenderer>     modelRenderer;
};