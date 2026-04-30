#pragma once
#include <windows.h>
#include <d2d1.h>

class Renderer {
public:
    ~Renderer() { Destroy(); }

    bool Initialize(HWND hwnd, int width, int height);
    void Destroy();

    bool BeginFrame();
    void EndFrame();

    ID2D1DCRenderTarget* RT()      const { return m_renderTarget; }
    ID2D1Factory*        Factory() const { return m_factory; }
    ID2D1SolidColorBrush* WhiteBrush() const { return m_whiteBrush; }
    int Width()  const { return m_width; }
    int Height() const { return m_height; }

private:
    HWND m_hwnd   = nullptr;
    int  m_width  = 0;
    int  m_height = 0;

    ID2D1Factory*        m_factory       = nullptr;
    ID2D1DCRenderTarget* m_renderTarget  = nullptr;
    ID2D1SolidColorBrush* m_whiteBrush   = nullptr;

    HDC    m_memDC  = nullptr;
    HBITMAP m_dib  = nullptr;
    HBITMAP m_oldBmp = nullptr;
    void*  m_bits   = nullptr;
};
