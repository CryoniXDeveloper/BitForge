#pragma once
#include <vector>
#include <string>
#include <iomanip>
#include <iostream>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include "../rom.h"

struct OperandInfo {
    std::string type1;
    int size1;
    std::string type2;
    int size2;
};

struct Assembler {
    std::vector<uint8_t> binaryToWrite;

    inline static const std::unordered_map<std::string, std::vector<uint8_t>> operandByte = {
        {"r8<r8<",    {0x00}},
        {"r8<<",      {0x00}},
        {"r8<i8<",    {0x01}},
        {"r8<i16<",   {0x02}},
        {"r8<i32<",   {0x03}},
        {"r8<i64<",   {0x04}},
        {"r8<mi8<",   {0x05}},
        {"r8<mi16<",  {0x06}},
        {"r8<mi32<",  {0x07}},
        {"r8<mi64<",  {0x08}},
        {"r8<mr8<",   {0x09}},

        {"mi8<r8<",   {0x0A}},
        {"mi8<i8<",   {0x0B}},
        {"mi8<i16<",  {0x0C}},
        {"mi8<i32<",  {0x0D}},
        {"mi8<i64<",  {0x0E}},
        {"mi8<mi8<",  {0x0F}},
        {"mi8<<",     {0x0F}},
        {"mi8<mi16<", {0x10}},
        {"mi8<mi32<", {0x11}},
        {"mi8<mi64<", {0x12}},
        {"mi8<mr8<",  {0x13}},

        {"mi16<r8<",  {0x14}},
        {"mi16<i8<",  {0x15}},
        {"mi16<i16<", {0x16}},
        {"mi16<i32<", {0x17}},
        {"mi16<i64<", {0x18}},
        {"mi16<mi8<", {0x19}},
        {"mi16<mi16<",{0x1A}},
        {"mi16<<",    {0x1A}},
        {"mi16<mi32<",{0x1B}},
        {"mi16<mi64<",{0x1C}},
        {"mi16<mr8<", {0x1D}},

        {"mi32<r8<",  {0x1E}},
        {"mi32<i8<",  {0x1F}},
        {"mi32<i16<", {0x20}},
        {"mi32<i32<", {0x21}},
        {"mi32<i64<", {0x22}},
        {"mi32<mi8<", {0x23}},
        {"mi32<mi16<",{0x24}},
        {"mi32<mi32<",{0x25}},
        {"mi32<<",    {0x25}},
        {"mi32<mi64<",{0x26}},
        {"mi32<mr8<", {0x27}},

        {"mi64<r8<",  {0x28}},
        {"mi64<i8<",  {0x29}},
        {"mi64<i16<", {0x2A}},
        {"mi64<i32<", {0x2B}},
        {"mi64<i64<", {0x2C}},
        {"mi64<mi8<", {0x2D}},
        {"mi64<mi16<",{0x2E}},
        {"mi64<mi32<",{0x2F}},
        {"mi64<mi64<",{0x30}},
        {"mi64<<",    {0x30}},
        {"mi64<mr8<", {0x31}},

        {"mr8<r8<",   {0x32}},
        {"mr8<i8<",   {0x33}},
        {"mr8<i16<",  {0x34}},
        {"mr8<i32<",  {0x35}},
        {"mr8<i64<",  {0x36}},
        {"mr8<mi8<",  {0x37}},
        {"mr8<mi16<", {0x38}},
        {"mr8<mi32<", {0x39}},
        {"mr8<mi64<", {0x3A}},
        {"mr8<mr8<",  {0x3B}},
        {"mr8<<",     {0x3B}},
    };

    inline static const std::unordered_set<std::string> operandInfoMnemonics = {
        "mval mb8", "mval mb16", "mval mb32", "mval mb64"
    };

    std::unordered_map<std::string, std::vector<uint8_t>> mnemonicOpcode = {
        {"nac", {0x00}}, {"mval mb8", {0x10}}, {"mval mb16", {0x11}}, {"mval mb32", {0x12}},
        {"mval mb64", {0x13}}, {"wait", {0xFC}}, {"stop", {0xFD}}, {"sleepms", {0xFE}}, {"sleepins", {0xFF}},
    };

    void pushData(std::vector<uint8_t> data);

    void checkFileExtension(std::string fileName, std::string fileExtension);

    std::string trim(const std::string& str);
    std::vector<std::string> split(const std::string& str);

    std::vector<uint8_t> getOpcode(std::string mnemonic);
    std::vector<uint8_t> getOperandInfoBytes(std::string operandDescriptor);

    void error(std::string errorType, std::string info) const;
};

extern Assembler assembler;