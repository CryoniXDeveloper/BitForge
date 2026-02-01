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

    static constexpr uint64_t STACK_END = 0x00000000;
    static constexpr uint64_t STACK_START = STACK_END + STACK_SIZE - 1;
    static constexpr uint64_t ROM_START = STACK_START + 1;
    static constexpr uint64_t ROM_END   = ROM_START + ROM_SIZE - 1;
    static constexpr uint64_t RAM_START = ROM_END + 1;
    static constexpr uint64_t RAM_END = RAM_START + RAM_SIZE - 1;

    Motherboard();
    ~Motherboard();
    void run();
};