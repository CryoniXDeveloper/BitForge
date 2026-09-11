#include "motherboard.h"
#include "cpu.h"
#include "ram.h"
#include "display.h"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <thread>

int main() {
    Motherboard motherboard;

    Display display;
    if (!display.init(Motherboard::FB_WIDTH, Motherboard::FB_HEIGHT, 2, "BitForge Display")) {
        return 1;
    }

    const auto framebufferOffset = static_cast<std::size_t>(
        Motherboard::FRAMEBUFFER_START - Motherboard::RAM_START);
    const std::uint8_t* const framebufferRam = memory.memory.data() + framebufferOffset;

    std::thread cpuThread([&motherboard] {
        motherboard.run();
    });

    auto fpsTime = std::chrono::steady_clock::now();
    unsigned int frames = 0;
    bool renderReported = false;
    double renderMilliseconds = 0.0;
    double megapixelsPerSecond = 0.0;

    while (display.pumpEvents()) {
        display.present(framebufferRam);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        if (!renderReported && motherboard.cpu->finished.load(std::memory_order_acquire)) {
            const double renderSeconds = motherboard.cpu->CPURunTime;
            renderMilliseconds = renderSeconds * 1000.0;
            megapixelsPerSecond = renderSeconds > 0.0
                ? static_cast<double>(Motherboard::FB_WIDTH * Motherboard::FB_HEIGHT) /
                    renderSeconds / 1'000'000.0
                : 0.0;
            renderReported = true;
        }

        ++frames;
        const auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration<double>(now - fpsTime).count() >= 1.0) {
            char title[160];
            if (renderReported) {
                std::snprintf(title, sizeof(title),
                              "BitForge - Render: %.2f ms | %.2f MPix/s | Present: %u FPS",
                              renderMilliseconds, megapixelsPerSecond, frames);
            } else {
                std::snprintf(title, sizeof(title), "BitForge - Rendering | Present: %u FPS", frames);
            }
            SetWindowTextA(display.window(), title);
            frames = 0;
            fpsTime = now;
        }
    }

    motherboard.cpu->running = false;
    cpuThread.join();
    return 0;
}
