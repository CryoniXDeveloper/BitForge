#pragma once
#include <vector>
#include <string>
#include <iomanip>
#include <iostream>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include "../rom.h"

struct Assembler {
    std::vector<uint8_t> binaryToWrite;
    int registers64max = 63;

    enum OperandKind : uint8_t {
        r8, i8, i16, i32, i64,
        mi8, mi16, mi32, mi64,
        mr8, i,
        OPERAND_KIND_COUNT
    };

    static constexpr uint8_t operandByte[OPERAND_KIND_COUNT] = {
        0x00, 0x01, 0x02, 0x03, 0x04,
        0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A
    };

    enum OperandType : uint8_t { reg, imm, mem_imm, mem_reg };

    struct operandInfo {
        OperandType type;
        uint8_t size;
    };

    static constexpr operandInfo operandMapBits[OPERAND_KIND_COUNT] = {
        {reg,     8},  {imm,     8},  {imm,  16}, {imm,  32}, {imm,  64},
        {mem_imm, 8},  {mem_imm, 16}, {mem_imm, 32}, {mem_imm, 64},
        {mem_reg, 8},  {imm,     0}
    };

    static constexpr operandInfo operandMapBytes[OPERAND_KIND_COUNT] = {
        {reg,     1},  {imm,     1},  {imm,  2},  {imm,  4},  {imm,  8},
        {mem_imm, 1},  {mem_imm, 2},  {mem_imm, 4},  {mem_imm, 8},
        {mem_reg, 1},  {imm,     0}
    };
    enum Mnemonic : uint8_t {
        nac, mval8, mval16, mval32, mval64, mval64plus,
        wait, stop, sleepms, sleepsec,
        MNEMONIC_COUNT
    };

    static constexpr uint8_t mnemonicOpcode[MNEMONIC_COUNT] = {
        0x00, 0x10, 0x11, 0x12, 0x13, 0x14,
        0xFC, 0xFD, 0xFE, 0xFF
    };

    static constexpr bool isMvalMnemonic[MNEMONIC_COUNT] = {
        false, true, true, true, true, false,
        false, false, false, false
    };

    std::vector<uint8_t> intToBytes(std::string value, int amountOfBytes);

    void pushData(std::vector<uint8_t> data);

    void checkFileExtension(std::string fileName, std::string fileExtension);

    std::string trim(const std::string& str);
    std::vector<std::string> split(const std::string& str);

    std::vector<uint8_t> getOpcode(std::string mnemonic);
    std::vector<uint8_t> getOperandInfoByte(std::string operandDescriptor);

    void error(std::string errorType, std::string info) const;
    void errorWithException(const std::string& errorType, 
                                   const std::string& file, 
                                   int lineNumber, 
                                   const std::exception& e) const;

    std::vector<std::string> splitOperandDescriptor(const std::string& str);
};

extern Assembler assembler;