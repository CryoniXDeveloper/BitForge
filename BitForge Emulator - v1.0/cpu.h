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

    enum OpType {
        reg,
        imm,
        mem_imm,
        mem_reg
    };

    std::unordered_map<int,int> mnemonicMoveOperandBytes = {
        {0x00,1},{0x01,1},{0x02,2},{0x03,4},
        {0x04,8},{0x05,1},{0x06,2},{0x07,4},
        {0x08,8},{0x09,1},{0x0A,0}
    };
    std::unordered_map<int, OpType> mnemonicMoveOperandTypes = {
        {0x00,reg},{0x01,imm},{0x02,imm},{0x03,imm},{0x04,imm},
        {0x05,mem_imm},{0x06,mem_imm},{0x07,mem_imm},
        {0x08,mem_imm},{0x09,mem_reg},
        {0x0A,imm}
    };

    uint8_t mnemonicType1;
    uint8_t mnemonicType2;
    OpType op1Type;
    OpType op2Type;
    int op1Size;
    int op2Size;

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
    uint64_t addr;
    std::vector<uint8_t>* VectorOfBytesPointer = nullptr;
    std::vector<uint8_t> VectorOfBytes;

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

    void sleepMicroseconds(uint64_t microseconds);
};
