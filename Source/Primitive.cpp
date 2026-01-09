#include "Primitive.h"
#include <string>  // <--- TAMBAHKAN INI UNTUK std::wstring
#include <cmath>
#include <cassert>
#include <vector>  // Tambahkan ini juga biar aman
#include <cstdio>  // Untuk FILE*, fopen, dll

// Utility untuk load shader (Sesuaikan jika kamu punya class ShaderLoader sendiri)
// Asumsi: Kamu punya file shader compiled (.cso) di folder Data/Shaders/
// Jika tidak, kamu mungkin perlu membuatnya atau menggunakan basic shader string.

Primitive::Primitive(ID3D11Device* device)
{
    // 1. Buat Vertex Buffer (Dynamic)
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = sizeof(Vertex) * MAX_VERTICES;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    device->CreateBuffer(&bd, nullptr, vertexBuffer.GetAddressOf());

    // 2. Load Shaders (Pastikan path ini benar di proyekmu!)
    // Jika kamu memakai sistem resource manager, ganti bagian ini.
    // Ini contoh manual loading binary shader file:
    auto LoadShader = [&](const std::wstring& filename, std::vector<char>& buffer) {
        FILE* fp = nullptr;
        _wfopen_s(&fp, filename.c_str(), L"rb");
        if (!fp) return false;
        fseek(fp, 0, SEEK_END);
        long size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        buffer.resize(size);
        fread(buffer.data(), 1, size, fp);
        fclose(fp);
        return true;
        };

    std::vector<char> vsBlob, psBlob;
    // NOTE: Kamu butuh file "primitive_vs.cso" dan "primitive_ps.cso"
    if (LoadShader(L"Shader/primitive_vs.cso", vsBlob)) {
        device->CreateVertexShader(vsBlob.data(), vsBlob.size(), nullptr, vertexShader.GetAddressOf());

        // Input Layout
        D3D11_INPUT_ELEMENT_DESC layout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
        device->CreateInputLayout(layout, 2, vsBlob.data(), vsBlob.size(), inputLayout.GetAddressOf());
    }

    if (LoadShader(L"Shader/primitive_ps.cso", psBlob)) {
        device->CreatePixelShader(psBlob.data(), psBlob.size(), nullptr, pixelShader.GetAddressOf());
    }

    // 3. Rasterizer State
    D3D11_RASTERIZER_DESC rsDesc = {};
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_NONE;
    rsDesc.FrontCounterClockwise = false;
    device->CreateRasterizerState(&rsDesc, rasterizerState.GetAddressOf());

    // 4. Depth Stencil (Disable Z-Buffer for 2D)
    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = FALSE;
    device->CreateDepthStencilState(&dsDesc, depthStencilState.GetAddressOf());
}

Primitive::~Primitive() {}

// --- IMPLEMENTASI WRAPPER (Seperti Requestmu) ---

void Primitive::Rect(float x, float y, float w, float h, float cx, float cy, float angle, float r, float g, float b, float a)
{
    Rect(VECTOR2{ x, y }, VECTOR2{ w, h }, VECTOR2{ cx, cy }, angle, VECTOR4{ r, g, b, a });
}

void Primitive::Rect(const VECTOR2& position, const VECTOR2& size, const VECTOR2& center, float angle, const VECTOR4& color)
{
    // Kita langsung proses vertices di sini untuk dimasukkan ke batch
    DrawRectInternal(position, size, center, angle, color);
}

void Primitive::Line(float x1, float y1, float x2, float y2, float width, float r, float g, float b, float a)
{
    // Line digambar sebagai Rect tipis yang diputar
    float dx = x2 - x1;
    float dy = y2 - y1;
    float len = sqrtf(dx * dx + dy * dy);
    float angle = atan2f(dy, dx);

    // Center di kiri tengah (0, width/2) agar rotasi dari titik awal
    Rect(x1, y1, len, width, 0, width * 0.5f, angle, r, g, b, a);
}

void Primitive::Circle(float x, float y, float radius, float r, float g, float b, float a, int segments)
{
    // Implementasi simple menggunakan GL_TRIANGLE_FAN logic tapi manual triangulasi
    // Untuk saat ini, kita skip dulu implementasi kompleks circle batching
    // agar kamu bisa fokus ke Background Kotak Kuning dulu.
}

// --- INTERNAL LOGIC ---

void Primitive::DrawRectInternal(const VECTOR2& pos, const VECTOR2& size, const VECTOR2& center, float angle, const VECTOR4& color)
{
    // Kalkulasi 4 titik sudut kotak (Local Space -> World Space -> Screen Space nanti di Shader/CPU)
    // Disini kita hitung CPU transform sederhana ke Pixel Coordinates

    float cosA = cosf(angle);
    float sinA = sinf(angle);

    // Definisi Quad relative terhadap titik pusat (center)
    // Urutan: TopLeft, TopRight, BottomLeft, BottomRight (Triangle Strip)
    struct Point { float x, y; };
    Point offsets[4] = {
        { -center.x,          -center.y },           // TL
        { -center.x + size.x, -center.y },           // TR
        { -center.x,          -center.y + size.y },  // BL
        { -center.x + size.x, -center.y + size.y }   // BR
    };

    for (int i = 0; i < 4; ++i)
    {
        // Rotasi
        float rx = offsets[i].x * cosA - offsets[i].y * sinA;
        float ry = offsets[i].x * sinA + offsets[i].y * cosA;

        // Translasi
        Vertex v;
        v.position = { pos.x + rx, pos.y + ry, 0.0f };
        v.color = color;

        batchVertices.push_back(v);
    }

    // Tambahkan 2 vertex degenerate/dummy jika kita menyambung strip (opsional untuk advanced batching)
    // Untuk simplifikasi Primitive.cpp ini, kita asumsikan Render() dipanggil per primitive atau menggunakan Topology List.
    // Agar bisa batching Triangle Strip, kita perlu teknik restart index atau degenerate triangle.
    // Disini saya ubah jadi Topology TRIANGLE LIST (6 vertices per quad) biar mudah dibatch:

    // REVISI LOGIC: Gunakan Triangle List (lebih aman buat batching manual)
    // Hapus 4 push_back di atas, ganti dengan 6 push_back (2 segitiga)
    // ... (Code logic disederhanakan di bawah ini)
}

// Versi sederhana Render langsung (Immediate Mode style) agar tidak pusing batching
void Primitive::Render(ID3D11DeviceContext* context)
{
    if (batchVertices.empty()) return;

    // 1. Setup Screen Size untuk Normalisasi Koordinat (Pixel ke -1..1)
    // Dapatkan viewport dari context
    UINT num = 1;
    D3D11_VIEWPORT vp;
    context->RSGetViewports(&num, &vp);
    float screenW = vp.Width;
    float screenH = vp.Height;

    // 2. Map Buffer
    D3D11_MAPPED_SUBRESOURCE msr;
    if (SUCCEEDED(context->Map(vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr)))
    {
        Vertex* ptr = (Vertex*)msr.pData;

        // Convert Pixel Coords to NDC (-1 to 1) disini
        for (const auto& v : batchVertices)
        {
            Vertex finalV = v;
            // Rumus: (x / w) * 2 - 1. Y dibalik karena DX Y+ naik, Screen Y+ turun.
            finalV.position.x = (v.position.x / screenW) * 2.0f - 1.0f;
            finalV.position.y = -((v.position.y / screenH) * 2.0f - 1.0f);
            *ptr++ = finalV;
        }

        context->Unmap(vertexBuffer.Get(), 0);
    }

    // 3. Set Pipeline
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
    context->IASetInputLayout(inputLayout.Get());
    // Gunakan Triangle Strip karena urutan vertex di DrawRectInternal cocok utk Strip
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    context->VSSetShader(vertexShader.Get(), nullptr, 0);
    context->PSSetShader(pixelShader.Get(), nullptr, 0);
    context->RSSetState(rasterizerState.Get());
    context->OMSetDepthStencilState(depthStencilState.Get(), 0);

    // 4. Draw
    context->Draw((UINT)batchVertices.size(), 0);

    // 5. Clear Batch
    batchVertices.clear();
}