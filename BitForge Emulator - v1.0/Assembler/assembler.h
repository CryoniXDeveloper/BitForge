#pragma once
#include <vector>
#include <string>
#include <iomanip>
#include <iostream>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include "../rom.h"

struct operandInfo {
    std::string type;
    int size;
};

struct Assembler {
    std::vector<uint8_t> binaryToWrite;
    int registers64max = 63;

    inline static const std::unordered_map<std::string, std::vector<uint8_t>> operandByte = {
        {"r8<",    {0x00}},
        {"i8<",    {0x01}},
        {"i16<",   {0x02}},
        {"i32<",   {0x04}},
        {"i64<",   {0x08}},
        {"mi8<",   {0x09}},
        {"mi16<",  {0x0A}},
        {"mi32<",  {0x0C}},
        {"mi64<",  {0x10}},
        {"mr8<",   {0x11}},
        {"i<",     {0x12}},
    };

    inline static const std::unordered_map<std::string, operandInfo> operandMapBits = {
        {"r8<",    {"r", 8}},    
        {"i8<",    {"i", 8}},    
        {"i16<",   {"i", 16}},   
        {"i32<",   {"i", 32}},   
        {"i64<",   {"i", 64}},   
        {"mi8<",   {"mi", 8}},   
        {"mi16<",  {"mi", 16}},  
        {"mi32<",  {"mi", 32}},  
        {"mi64<",  {"mi", 64}},  
        {"mr8<",   {"mr", 8}},   
        {"i<",     {"i", 0}}
    };

    inline static const std::unordered_map<std::string, operandInfo> operandMapBytes = {
        {"r8<",    {"r", 1}},    
        {"i8<",    {"i", 1}},    
        {"i16<",   {"i", 2}},   
        {"i32<",   {"i", 4}},   
        {"i64<",   {"i", 8}},   
        {"mi8<",   {"mi", 1}},   
        {"mi16<",  {"mi", 2}},  
        {"mi32<",  {"mi", 4}},  
        {"mi64<",  {"mi", 8}},  
        {"mr8<",   {"mr", 1}},   
        {"i<",     {"i", 0}},
    };

    inline static const std::unordered_set<std::string> mvalMnemonics = {
        "mval8", "mval16" "mval32", "mval64"
    };

    std::unordered_map<std::string, std::vector<uint8_t>> mnemonicOpcode = {
        {"nac", {0x00}}, {"mval8", {0x10}}, {"mval16", {0x11}},
        {"mval32", {0x12}}, {"mval64", {0x13}}, {"wait", {0xFC}}, {"stop", {0xFD}}, 
        {"sleepms", {0xFE}}, {"sleepsec", {0xFF}},
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