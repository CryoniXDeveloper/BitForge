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
    uint8_t flags = 0;

    bool running = false;
    uint64_t cycles = 0;
    int sleepCounter = 0;
    double CPURunTime = 0.0;
    volatile uint8_t warmup = 0;

    Timer timer;

    void triggerInterrupt(uint8_t vector);
    void checkInterrupts();

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
    uint8_t  operands8[4];
    uint16_t operands16[4];
    uint32_t operands32[4];
    uint64_t operands64[4];
    std::vector<std::vector<uint8_t>> operandsVector;
    uint8_t instructionSize;

    enum OpType {
        reg,
        imm,
        mem_imm,
        mem_reg
    };

    static constexpr uint8_t mnemonicMoveOperandBytes[11] = {
        1,1,2,4,8,1,2,4,8,1,0
    };

    static constexpr OpType mnemonicMoveOperandTypes[11] = {
        reg, imm, imm, imm, imm,
        mem_imm, mem_imm, mem_imm, mem_imm,
        mem_reg,
        imm
    };

    uint8_t mnemonicType1;
    uint8_t mnemonicType2;
    uint8_t mnemonicType3;
    uint8_t mnemonicType4;
    OpType op1Type;
    OpType op2Type;
    OpType op3Type;
    OpType op4Type;
    int op1Size;
    int op2Size;
    int op3Size;
    int op4Size;
    int op2VectorSize;
    int registersNeeded;

    int op8Index;
    int op16Index;
    int op32Index;
    int op64Index;
    int opVectorIndex;
    
    void resetOpIndexes() {
        op8Index = 0;
        op16Index = 0;
        op32Index = 0;
        op64Index = 0;
        opVectorIndex = 0;
    }

    uint64_t dest;
    uint64_t value;
    uint64_t value1;
    uint64_t value2;
    uint64_t addr;
    uint64_t sh;
    std::vector<uint8_t>* VectorOfBytesPointer = nullptr;
    std::vector<uint8_t> VectorOfBytes;
    int64_t result64;
    int64_t high64;
    int64_t low64;
    uint64_t carry;
    uint64_t sum;
    unsigned char c_out;

    bool pendingInterrupt = false;
    bool pendingNMI = false;
    uint8_t interruptNumber = 0;

    inline void setFlagBit(CPU::flagNames flagName, bool state) {
        flags = (flags & ~flagName) | (state ? flagName : 0);
    }

    inline bool getFlagBit(CPU::flagNames flagName) {
        return (flags & flagName) != 0;
    }

    uint8_t read8(uint64_t address);
    uint16_t read16(uint64_t address);
    uint32_t read32(uint64_t address);
    uint64_t read64(uint64_t address);

    void write8(uint64_t address, uint8_t value);
    void write16(uint64_t address, uint16_t value);
    void write32(uint64_t address, uint32_t value);
    void write64(uint64_t address, uint64_t value);

    std::vector<uint8_t> readBytesVector(uint64_t start, size_t length);
    void writeBytesVector(uint64_t start, const std::vector<uint8_t>& data);

    void start();
    void fetch();
    void decode();
    void execute();

    void error(std::string errorType, std::string info = "") const;
};
