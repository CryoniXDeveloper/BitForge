#pragma once
#include <cstdint>
#include <string>
#include "timer.h"
#include <thread>
#include <chrono>
#include <vector>
#include <iomanip>
#include <Windows.h>
#include <unordered_map>

class Motherboard;
class ROM;
class RAM;

struct CPU {
    Motherboard* motherboard = nullptr;
    ROM* rom = nullptr;
    RAM* memory = nullptr;

    uint64_t instructionPointer = 0;
    uint64_t stackPointer = 0;
    int instructionCounter = 0;
    uint64_t registers[64]{};
    uint16_t flags = 0;

    bool running = false;
    int sleepCounter = 0;
    double CPURunTime = 0.0;

    Timer timer;

    enum flagNames : uint8_t {
        FLAG_CARRY      = 1 << 7,
        FLAG_ZERO       = 1 << 6,
        FLAG_NEGATIVE   = 1 << 5,
        FLAG_OVERFLOW   = 1 << 4,
        FLAG_INTERRUPT  = 1 << 3,
        FLAG_SLEEP      = 1 << 2,
        FLAG_DIRECTION  = 1 << 1,
        FLAG_SUPERVISOR = 1 << 0
    };

    uint16_t opcode;
    std::vector<uint8_t> operands8;
    std::vector<uint16_t> operands16;
    std::vector<uint32_t> operands32;
    std::vector<uint64_t> operands64;
    std::vector<std::vector<uint8_t>> operandsVector;
    uint8_t instructionSize;

    std::unordered_map<int,int> mnemonicMoveOperandBytes = {
        {0x00,1},{0x01,1},{0x02,2},{0x03,3},{0x04,4},
        {0x05,5},{0x06,6},{0x07,7},{0x08,8},{0x09,1},
        {0x0A,2},{0x0B,3},{0x0C,4},{0x0D,5},{0x0E,6},
        {0x0F,7},{0x10,8},{0x11,1},{0x12,0}
    };
    std::unordered_map<int,std::string> mnemonicMoveOperandTypes = {
        {0x00,"r"},{0x01,"i"},{0x02,"i"},{0x03,"i"},{0x04,"i"},{0x05,"i"},
        {0x06,"i"},{0x07,"i"},{0x08,"i"},{0x09,"mi"},{0x0A,"mi"},{0x0B,"mi"},
        {0x0C,"mi"},{0x0D,"mi"},{0x0E,"mi"},{0x0F,"mi"},{0x10,"mi"},{0x11,"r"},
        {0x12,"i"}
    };

    template<typename T>
    void pushData(T& data, size_t length);

    uint8_t mnemonicType1;
    uint8_t mnemonicType2;
    std::string op1Type;
    std::string op2Type;
    int op1Size;
    int op2Size;

    int op8Index;
    int op16Index;
    int op32Index;
    int op64Index;
    
    void resetOpIndexes() {
        op8Index = 0;
        op16Index = 0;
        op32Index = 0;
        op64Index = 0;
    }

    inline void setFlagBit(CPU::flagNames flagName, bool state) {
        if (state) {
            flags |= flagName;

        } else {
            flags &= ~flagName;

        }
    }

    inline bool getFlagBit(CPU::flagNames flagName) {
        return (flags & flagName) != 0;
    }

    uint8_t read8(uint64_t address);
    uint16_t read16(uint64_t address);
    uint32_t read32(uint64_t address);
    uint64_t read64(uint64_t address);

    void write8(uint64_t address, uint8_t value);
    std::vector<uint8_t> readBytesVector(uint64_t start, size_t length);
    void writeBytesVector(uint64_t start, const std::vector<uint8_t>& data);

    void start();
    void fetch();
    void decode();
    void execute();

    void error(std::string errorType, std::string info = "") const;

    void sleepMicroseconds(uint64_t microseconds);
};
