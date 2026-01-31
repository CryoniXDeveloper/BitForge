#pragma once
#include "rom.h"
#include "ram.h"

class CPU;

extern RAM ram;
extern ROM rom;

class Motherboard {
public:
    CPU* cpu;

    static constexpr uint64_t STACK_START = 0x0000FFFF;
    static constexpr uint64_t ROM_START = 0x00010000;
    static constexpr uint64_t ROM_END   = 0x07FFFFFF;
    static constexpr uint64_t RAM_START = 0x08000000;
    uint64_t RAM_END;

    Motherboard();
    ~Motherboard();
    void run();
};

