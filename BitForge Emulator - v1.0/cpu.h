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

    std::unordered_map<int,int> mnemonicMoveOperand1Bytes = {
        {0x00,1},{0x01,1},{0x02,2},{0x03,4},{0x04,8},
        {0x05,1},{0x06,2},{0x07,4},{0x08,8},{0x09,1},
        {0x0A,1},{0x0B,1},{0x0C,2},{0x0D,4},{0x0E,8},
        {0x0F,1},{0x10,2},{0x11,4},{0x12,8},{0x13,1},
        {0x14,1},{0x15,1},{0x16,2},{0x17,4},{0x18,8},
        {0x19,1},{0x1A,2},{0x1B,4},{0x1C,8},{0x1D,1},
        {0x1E,1},{0x1F,1},{0x20,2},{0x21,4},{0x22,8},
        {0x23,1},{0x24,2},{0x25,4},{0x26,8},{0x27,1},
        {0x28,1},{0x29,1},{0x2A,2},{0x2B,4},{0x2C,8},
        {0x2D,1},{0x2E,2},{0x2F,4},{0x30,8},{0x31,1},
        {0x32,1},{0x33,1},{0x34,2},{0x35,4},{0x36,8},
        {0x37,1},{0x38,2},{0x39,4},{0x3A,8},{0x3B,1},
        {0x3C,1},{0x3D,1},{0x3E,2},{0x3F,4},{0x40,8},
        {0x41,1},{0x42,2},{0x43,4},{0x44,8},{0x45,1},
        {0x46,1},{0x47,1},{0x48,2},{0x49,4},{0x4A,8},{0x4B,1}
    };

    std::unordered_map<int,int> mnemonicMoveOperand2Bytes = {
        {0x0A,1},{0x0B,1},{0x0C,2},{0x0D,4},{0x0E,8},
        {0x0F,1},{0x10,2},{0x11,4},{0x12,8},{0x13,1},
        {0x14,1},{0x15,1},{0x16,2},{0x17,4},{0x18,8},
        {0x19,1},{0x1A,2},{0x1B,4},{0x1C,8},{0x1D,1},
        {0x1E,1},{0x1F,1},{0x20,2},{0x21,4},{0x22,8},
        {0x23,1},{0x24,2},{0x25,4},{0x26,8},{0x27,1},
        {0x28,1},{0x29,1},{0x2A,2},{0x2B,4},{0x2C,8},
        {0x2D,1},{0x2E,2},{0x2F,4},{0x30,8},{0x31,1},
        {0x32,1},{0x33,1},{0x34,2},{0x35,4},{0x36,8},
        {0x37,1},{0x38,2},{0x39,4},{0x3A,8},{0x3B,1},
        {0x3C,1},{0x3D,1},{0x3E,2},{0x3F,4},{0x40,8},
        {0x41,1},{0x42,2},{0x43,4},{0x44,8},{0x45,1},
        {0x46,0},{0x47,0},{0x48,0},{0x49,0},{0x4A,0},{0x4B,0}
    };

    std::unordered_map<int,std::string> mnemonicMoveOperand1Types = {
        {0x00,"r"},{0x01,"i"},{0x02,"i"},{0x03,"i"},{0x04,"i"},
        {0x05,"mi"},{0x06,"mi"},{0x07,"mi"},{0x08,"mi"},{0x09,"mr"},
        {0x0A,"r"},{0x0B,"r"},{0x0C,"r"},{0x0D,"r"},{0x0E,"r"},
        {0x0F,"r"},{0x10,"r"},{0x11,"r"},{0x12,"r"},{0x13,"r"},
        {0x14,"mi"},{0x15,"mi"},{0x16,"mi"},{0x17,"mi"},{0x18,"mi"},
        {0x19,"mi"},{0x1A,"mi"},{0x1B,"mi"},{0x1C,"mi"},{0x1D,"mi"},
        {0x1E,"mi"},{0x1F,"mi"},{0x20,"mi"},{0x21,"mi"},{0x22,"mi"},
        {0x23,"mi"},{0x24,"mi"},{0x25,"mi"},{0x26,"mi"},{0x27,"mi"},
        {0x28,"mi"},{0x29,"mi"},{0x2A,"mi"},{0x2B,"mi"},{0x2C,"mi"},
        {0x2D,"mi"},{0x2E,"mi"},{0x2F,"mi"},{0x30,"mi"},{0x31,"mi"},
        {0x32,"mr"},{0x33,"mr"},{0x34,"mr"},{0x35,"mr"},{0x36,"mr"},
        {0x37,"mr"},{0x38,"mr"},{0x39,"mr"},{0x3A,"mr"},{0x3B,"mr"},
        {0x3C,"r"},{0x3D,"i"},{0x3E,"i"},{0x3F,"i"},{0x40,"i"},
        {0x41,"mi"},{0x42,"mi"},{0x43,"mi"},{0x44,"mi"},{0x45,"mr"},
        {0x46,"r"},{0x47,"mi"},{0x48,"mi"},{0x49,"mi"},{0x4A,"mi"},{0x4B,"mr"}
    };

    std::unordered_map<int,std::string> mnemonicMoveOperand2Types = {
        {0x0A,"r"},{0x0B,"i"},{0x0C,"i"},{0x0D,"i"},{0x0E,"i"},
        {0x0F,"mi"},{0x10,"mi"},{0x11,"mi"},{0x12,"mi"},{0x13,"mr"},
        {0x14,"r"},{0x15,"i"},{0x16,"i"},{0x17,"i"},{0x18,"i"},
        {0x19,"mi"},{0x1A,"mi"},{0x1B,"mi"},{0x1C,"mi"},{0x1D,"mr"},
        {0x1E,"r"},{0x1F,"i"},{0x20,"i"},{0x21,"i"},{0x22,"i"},
        {0x23,"mi"},{0x24,"mi"},{0x25,"mi"},{0x26,"mi"},{0x27,"mr"},
        {0x28,"r"},{0x29,"i"},{0x2A,"i"},{0x2B,"i"},{0x2C,"i"},
        {0x2D,"mi"},{0x2E,"mi"},{0x2F,"mi"},{0x30,"mi"},{0x31,"mr"},
        {0x32,"r"},{0x33,"i"},{0x34,"i"},{0x35,"i"},{0x36,"i"},
        {0x37,"mi"},{0x38,"mi"},{0x39,"mi"},{0x3A,"mi"},{0x3B,"mr"},
        {0x3C,"r"},{0x3D,"i"},{0x3E,"i"},{0x3F,"i"},{0x40,"i"},
        {0x41,"mi"},{0x42,"mi"},{0x43,"mi"},{0x44,"mi"},{0x45,"mr"},
        {0x46,"i"},{0x47,"i"},{0x48,"i"},{0x49,"i"},{0x4A,"i"},{0x4B,"i"}
    };

    std::vector<std::vector<uint8_t>> operandsVector;
    uint8_t instructionSize;

    int mnemonicType;
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
