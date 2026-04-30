#include "Renderer.h"
#include <cstdlib>

// DIBSection pixel (32-bit ARGB)
#pragma pack(push, 1)
struct Pixel { unsigned char b, g, r, a; };
#pragma pack(pop)

bool Renderer::Initialize(HWND hwnd, int width, int height) {
    m_hwnd   = hwnd;
    m_width  = width;
    m_height = height;

    // Create D2D factory
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_factory);
    if (FAILED(hr)) return false;

    // Create a 32-bit ARGB DIBSection for per-pixel alpha
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = width;
    bmi.bmiHeader.biHeight      = -height;  // top-down
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC screenDC = GetDC(nullptr);
    m_memDC = CreateCompatibleDC(screenDC);
    m_dib   = CreateDIBSection(screenDC, &bmi, DIB_RGB_COLORS, &m_bits, nullptr, 0);
    ReleaseDC(nullptr, screenDC);

    if (!m_dib || !m_memDC) {
        Destroy();
        return false;
    }
    m_oldBmp = (HBITMAP)SelectObject(m_memDC, m_dib);

    // Create D2D render target bound to the memory DC
    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
    );

    hr = m_factory->CreateDCRenderTarget(&props, &m_renderTarget);
    if (FAILED(hr)) {
        Destroy();
        return false;
    }

    // Pre-create shared white brush
    hr = m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 1), &m_whiteBrush);
    if (FAILED(hr)) { Destroy(); return false; }

    // Initialize DIB with zero alpha
    DWORD* p = (DWORD*)m_bits;
    for (int i = 0; i < width * height; ++i)
        p[i] = 0;

    // Present initial transparent frame
    BLENDFUNCTION blend = {};
    blend.BlendOp             = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat         = AC_SRC_ALPHA;

    POINT ptZero = {0, 0};
    SIZE  sz     = {width, height};
    UpdateLayeredWindow(m_hwnd, nullptr, nullptr, &sz, m_memDC, &ptZero, 0, &blend, ULW_ALPHA);

    return true;
}

void Renderer::Destroy() {
    if (m_whiteBrush)   { m_whiteBrush->Release();   m_whiteBrush   = nullptr; }
    if (m_renderTarget) { m_renderTarget->Release(); m_renderTarget = nullptr; }
    if (m_factory)      { m_factory->Release();      m_factory = nullptr; }

    if (m_memDC && m_oldBmp) SelectObject(m_memDC, m_oldBmp);
    if (m_dib)   { DeleteObject(m_dib);   m_dib   = nullptr; }
    if (m_memDC) { DeleteDC(m_memDC);     m_memDC = nullptr; }

    m_bits = nullptr;
    m_oldBmp = nullptr;
}

bool Renderer::BeginFrame() {
    if (!m_renderTarget) return false;

    RECT rect = {0, 0, m_width, m_height};
    m_renderTarget->BindDC(m_memDC, &rect);
    m_renderTarget->BeginDraw();
    return true;
}

void Renderer::EndFrame() {
    if (!m_renderTarget) return;

    m_renderTarget->EndDraw();

    // Present via UpdateLayeredWindow
    BLENDFUNCTION blend = {};
    blend.BlendOp             = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat         = AC_SRC_ALPHA;

    POINT ptZero = {0, 0};
    SIZE  sz     = {m_width, m_height};
    UpdateLayeredWindow(m_hwnd, nullptr, nullptr, &sz, m_memDC, &ptZero, 0, &blend, ULW_ALPHA);
}
