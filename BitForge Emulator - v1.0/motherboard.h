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

    static constexpr uint32_t IO_PORT_COUNT = 65536;
    uint8_t ioPorts[IO_PORT_COUNT]{};

    uint8_t  readPort8 (uint16_t port);
    uint16_t readPort16(uint16_t port);
    uint32_t readPort32(uint16_t port);
    uint64_t readPort64(uint16_t port);

    void writePort8 (uint16_t port, uint8_t  value);
    void writePort16(uint16_t port, uint16_t value);
    void writePort32(uint16_t port, uint32_t value);
    void writePort64(uint16_t port, uint64_t value);

    Motherboard();
    ~Motherboard();
    void run();
};