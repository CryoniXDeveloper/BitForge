#include "display.h"

#include <cstring>

bool Display::init(int width, int height, int scaleFactor, const char* title) {
    width_ = width;
    height_ = height;
    scale_ = scaleFactor;
    frameBytes_ = static_cast<std::size_t>(width) * height * 4;

    WNDCLASSA windowClass{};
    windowClass.lpfnWndProc = windowProc;
    windowClass.hInstance = GetModuleHandleA(nullptr);
    windowClass.lpszClassName = "VMDisplayWindowClass";
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassA(&windowClass);

    RECT rect{0, 0, width * scaleFactor, height * scaleFactor};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    hwnd_ = CreateWindowExA(
        0, windowClass.lpszClassName, title, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top,
        nullptr, nullptr, windowClass.hInstance, this);
    if (!hwnd_) return false;

    BITMAPINFO bitmapInfo{};
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = width;
    bitmapInfo.bmiHeader.biHeight = -height;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    HDC windowDC = GetDC(hwnd_);
    memoryDC_ = CreateCompatibleDC(windowDC);
    dib_ = CreateDIBSection(windowDC, &bitmapInfo, DIB_RGB_COLORS,
                            &dibPixels_, nullptr, 0);
    ReleaseDC(hwnd_, windowDC);
    if (!memoryDC_ || !dib_ || !dibPixels_) return false;

    previousBitmap_ = SelectObject(memoryDC_, dib_);
    ShowWindow(hwnd_, SW_SHOW);
    return true;
}

bool Display::pumpEvents() {
    MSG message;
    while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE)) {
        if (message.message == WM_QUIT) quit_ = true;
        TranslateMessage(&message);
        DispatchMessage(&message);
    }
    return !quit_;
}

void Display::present(const std::uint8_t* framebufferRam) const {
    std::memcpy(dibPixels_, framebufferRam, frameBytes_);

    HDC windowDC = GetDC(hwnd_);
    SetStretchBltMode(windowDC, COLORONCOLOR);
    StretchBlt(windowDC, 0, 0, width_ * scale_, height_ * scale_,
               memoryDC_, 0, 0, width_, height_, SRCCOPY);
    ReleaseDC(hwnd_, windowDC);
}

HWND Display::window() const {
    return hwnd_;
}

Display::~Display() {
    if (memoryDC_ && previousBitmap_) SelectObject(memoryDC_, previousBitmap_);
    if (dib_) DeleteObject(dib_);
    if (memoryDC_) DeleteDC(memoryDC_);
    if (hwnd_) DestroyWindow(hwnd_);
}

LRESULT CALLBACK Display::windowProc(HWND hwnd, UINT message, WPARAM wParam,
                                     LPARAM lParam) {
    auto* self = reinterpret_cast<Display*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));

    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<const CREATESTRUCTA*>(lParam);
        self = static_cast<Display*>(create->lpCreateParams);
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }

    switch (message) {
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE && self) self->quit_ = true;
        return 0;
    case WM_DESTROY:
        if (self) self->quit_ = true;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, message, wParam, lParam);
}
