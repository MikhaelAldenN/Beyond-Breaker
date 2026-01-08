#include "Graphics.h"
#include "Misc.h" // Assuming HRTrace is here

void Graphics::Initialize()
{
    HRESULT hr = S_OK;

    UINT createDeviceFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0,
    };
    D3D_FEATURE_LEVEL featureLevel;

    // 1. Create Device ONLY (No SwapChain yet)
    hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        createDeviceFlags,
        featureLevels,
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,
        device.GetAddressOf(),
        &featureLevel,
        immediateContext.GetAddressOf()
    );
    _ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

    // 2. Retrieve DXGI Factory (We need this to create SwapChains later)
    {
        Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
        hr = device.As(&dxgiDevice);

        Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
        hr = dxgiDevice->GetAdapter(adapter.GetAddressOf());

        hr = adapter->GetParent(__uuidof(IDXGIFactory), (void**)dxgiFactory.GetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
    }

    // 3. Initialize Shared Renderers
    renderState = std::make_unique<RenderState>(device.Get());
    primitiveRenderer = std::make_unique<PrimitiveRenderer>(device.Get());
    shapeRenderer = std::make_unique<ShapeRenderer>(device.Get());
    modelRenderer = std::make_unique<ModelRenderer>(device.Get());
}

void Graphics::CreateSwapChain(HWND hWnd, int width, int height, IDXGISwapChain** outSwapChain)
{
    if (!dxgiFactory) return;

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 2; // Double buffering
    sd.OutputWindow = hWnd;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // Modern standard
    sd.Flags = 0;

    dxgiFactory->CreateSwapChain(device.Get(), &sd, outSwapChain);
}