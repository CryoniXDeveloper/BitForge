#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <fstream>
#include <vector>
#include <cstdint>

int main() {
    std::ifstream f("rom.bin", std::ios::binary);
    std::vector<uint8_t> rom(10000, 0);
    if (f) f.read(reinterpret_cast<char*>(rom.data()), 10000);

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("BitForge ROM Viewer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1000, 1000, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);

    bool running = true;
    SDL_Event e;
    while (running) {
        while (SDL_PollEvent(&e))
            if (e.type == SDL_QUIT) running = false;

        for (int y = 0; y < 100; y++) {
            for (int x = 0; x < 100; x++) {
                uint8_t b = rom[y * 100 + x];
                uint8_t r = (b & 0b11100000) >> 5 * 36;
                uint8_t g = (b & 0b00011100) >> 2 * 36;
                uint8_t bl = (b & 0b00000011) * 85;
                SDL_SetRenderDrawColor(renderer, r, g, bl, 255);
                SDL_Rect rect = { x * 10, y * 10, 10, 10 };
                SDL_RenderFillRect(renderer, &rect);
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}