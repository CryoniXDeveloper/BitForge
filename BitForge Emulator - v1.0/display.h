#pragma once

#include <cstddef>
#include <cstdint>
#include <windows.h>

class Display {
public:
    bool init(int width, int height, int scaleFactor, const char* title);
    bool pumpEvents();
    void present(const std::uint8_t* framebufferRam) const;
    HWND window() const;
    ~Display();

private:
    static LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam,
                                       LPARAM lParam);

    HWND hwnd_ = nullptr;
    HDC memoryDC_ = nullptr;
    HBITMAP dib_ = nullptr;
    HGDIOBJ previousBitmap_ = nullptr;
    void* dibPixels_ = nullptr;
    int width_ = 0;
    int height_ = 0;
    int scale_ = 1;
    std::size_t frameBytes_ = 0;
    bool quit_ = false;
};
