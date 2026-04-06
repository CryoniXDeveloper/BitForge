#pragma once
#include "rom.h"
#include "ram.h"

class CPU;

extern RAM memory;
extern ROM rom;

class Motherboard {
public:
    CPU* cpu;
    
    static constexpr size_t RAM_SIZE = 128ull * 1024 * 1024;
    static constexpr size_t ROM_SIZE = 32ull * 1024;
    static constexpr size_t STACK_SIZE = 1024ull * 1024;
    static constexpr size_t IVT_SIZE = 2048ull;

    static constexpr uint64_t ROM_START = 0x00000000;
    static constexpr uint64_t ROM_END   = ROM_START + ROM_SIZE - 1;

    static constexpr uint64_t RAM_START = ROM_END + 1;
    static constexpr uint64_t RAM_END   = RAM_START + RAM_SIZE - 1;

    static constexpr uint64_t STACK_START = RAM_END;
    static constexpr uint64_t STACK_END   = STACK_START - STACK_SIZE + 1;

    static constexpr uint64_t IVT_END   = STACK_END - 1;
    static constexpr uint64_t IVT_START = IVT_END - IVT_SIZE + 1;

    static constexpr size_t RAM_RESERVED = STACK_SIZE + IVT_SIZE;
    static constexpr size_t RAM_USABLE   = RAM_SIZE - RAM_RESERVED;

    static constexpr uint64_t RAM_USABLE_START = RAM_START;
    static constexpr uint64_t RAM_USABLE_END   = IVT_START - 1;

    Motherboard();
    ~Motherboard();
    void run();
};