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
    std::string type1;
    int size1;
    std::string type2;
    int size2;
};

struct Assembler {
    std::vector<uint8_t> binaryToWrite;

    inline static const std::unordered_map<std::string, std::vector<uint8_t>> operandByte = {
        {"r8<",       {0x00}},
        {"i8<",       {0x01}},
        {"i16<",      {0x02}},
        {"i32<",      {0x03}},
        {"i64<",      {0x04}},
        {"mi8<",      {0x05}},
        {"mi16<",     {0x06}},
        {"mi32<",     {0x07}},
        {"mi64<",     {0x08}},
        {"mr8<",      {0x09}},

        {"r8<r8<",    {0x0A}},
        {"r8<<",      {0x0A}},
        {"r8<i8<",    {0x0B}},
        {"r8<i16<",   {0x0C}},
        {"r8<i32<",   {0x0D}},
        {"r8<i64<",   {0x0E}},
        {"r8<mi8<",   {0x0F}},
        {"r8<mi16<",  {0x10}},
        {"r8<mi32<",  {0x11}},
        {"r8<mi64<",  {0x12}},
        {"r8<mr8<",   {0x13}},

        {"mi8<r8<",   {0x14}},
        {"mi8<i8<",   {0x15}},
        {"mi8<i16<",  {0x16}},
        {"mi8<i32<",  {0x17}},
        {"mi8<i64<",  {0x18}},
        {"mi8<mi8<",  {0x19}},
        {"mi8<<",     {0x19}},
        {"mi8<mi16<", {0x1A}},
        {"mi8<mi32<", {0x1B}},
        {"mi8<mi64<", {0x1C}},
        {"mi8<mr8<",  {0x1D}},

        {"mi16<r8<",  {0x1E}},
        {"mi16<i8<",  {0x1F}},
        {"mi16<i16<", {0x20}},
        {"mi16<i32<", {0x21}},
        {"mi16<i64<", {0x22}},
        {"mi16<mi8<", {0x23}},
        {"mi16<mi16<",{0x24}},
        {"mi16<<",    {0x24}},
        {"mi16<mi32<",{0x25}},
        {"mi16<mi64<",{0x26}},
        {"mi16<mr8<", {0x27}},

        {"mi32<r8<",  {0x28}},
        {"mi32<i8<",  {0x29}},
        {"mi32<i16<", {0x2A}},
        {"mi32<i32<", {0x2B}},
        {"mi32<i64<", {0x2C}},
        {"mi32<mi8<", {0x2D}},
        {"mi32<mi16<",{0x2E}},
        {"mi32<mi32<",{0x2F}},
        {"mi32<<",    {0x2F}},
        {"mi32<mi64<",{0x30}},
        {"mi32<mr8<", {0x31}},

        {"mi64<r8<",  {0x32}},
        {"mi64<i8<",  {0x33}},
        {"mi64<i16<", {0x34}},
        {"mi64<i32<", {0x35}},
        {"mi64<i64<", {0x36}},
        {"mi64<mi8<", {0x37}},
        {"mi64<mi16<",{0x38}},
        {"mi64<mi32<",{0x39}},
        {"mi64<mi64<",{0x3A}},
        {"mi64<<",    {0x3A}},
        {"mi64<mr8<", {0x3B}},

        {"mr8<r8<",   {0x3C}},
        {"mr8<i8<",   {0x3D}},
        {"mr8<i16<",  {0x3E}},
        {"mr8<i32<",  {0x3F}},
        {"mr8<i64<",  {0x40}},
        {"mr8<mi8<",  {0x41}},
        {"mr8<mi16<", {0x42}},
        {"mr8<mi32<", {0x43}},
        {"mr8<mi64<", {0x44}},
        {"mr8<mr8<",  {0x45}},
        {"mr8<<",     {0x45}},

        {"r8<i<",     {0x46}},
        {"mi8<i<",    {0x47}},
        {"mi16<i<",   {0x48}},
        {"mi32<i<",   {0x49}},
        {"mi64<i<",   {0x4A}},
        {"mr8<i<",    {0x4B}},
    };

    inline static const std::unordered_map<std::string, operandInfo> operandMapBits = {
        {"r8<",       {"r", 8, "", 0}},          // 0x00
        {"i8<",       {"i", 8, "", 0}},          // 0x01
        {"i16<",      {"i", 16, "", 0}},         // 0x02
        {"i32<",      {"i", 32, "", 0}},         // 0x03
        {"i64<",      {"i", 64, "", 0}},         // 0x04
        {"mi8<",      {"mi", 8, "", 0}},         // 0x05
        {"mi16<",     {"mi", 16, "", 0}},        // 0x06
        {"mi32<",     {"mi", 32, "", 0}},        // 0x07
        {"mi64<",     {"mi", 64, "", 0}},        // 0x08
        {"mr8<",      {"mr", 8, "", 0}},         // 0x09

        {"r8<r8<",    {"r", 8, "r", 8}},         // 0x0A
        {"r8<<",      {"r", 8, "r", 8}},         // 0x0A
        {"r8<i8<",    {"r", 8, "i", 8}},         // 0x0B
        {"r8<i16<",   {"r", 8, "i", 16}},        // 0x0C
        {"r8<i32<",   {"r", 8, "i", 32}},        // 0x0D
        {"r8<i64<",   {"r", 8, "i", 64}},        // 0x0E
        {"r8<mi8<",   {"r", 8, "mi", 8}},        // 0x0F
        {"r8<mi16<",  {"r", 8, "mi", 16}},       // 0x10
        {"r8<mi32<",  {"r", 8, "mi", 32}},       // 0x11
        {"r8<mi64<",  {"r", 8, "mi", 64}},       // 0x12
        {"r8<mr8<",   {"r", 8, "mr", 8}},        // 0x13

        {"mi8<r8<",   {"mi", 8, "r", 8}},        // 0x14
        {"mi8<i8<",   {"mi", 8, "i", 8}},        // 0x15
        {"mi8<i16<",  {"mi", 8, "i", 16}},       // 0x16
        {"mi8<i32<",  {"mi", 8, "i", 32}},       // 0x17
        {"mi8<i64<",  {"mi", 8, "i", 64}},       // 0x18
        {"mi8<mi8<",  {"mi", 8, "mi", 8}},       // 0x19
        {"mi8<<",     {"mi", 8, "mi", 8}},       // 0x19
        {"mi8<mi16<", {"mi", 8, "mi", 16}},      // 0x1A
        {"mi8<mi32<", {"mi", 8, "mi", 32}},      // 0x1B
        {"mi8<mi64<", {"mi", 8, "mi", 64}},      // 0x1C
        {"mi8<mr8<",  {"mi", 8, "mr", 8}},       // 0x1D

        {"mi16<r8<",  {"mi", 16, "r", 8}},       // 0x1E
        {"mi16<i8<",  {"mi", 16, "i", 8}},       // 0x1F
        {"mi16<i16<", {"mi", 16, "i", 16}},      // 0x20
        {"mi16<i32<", {"mi", 16, "i", 32}},      // 0x21
        {"mi16<i64<", {"mi", 16, "i", 64}},      // 0x22
        {"mi16<mi8<", {"mi", 16, "mi", 8}},      // 0x23
        {"mi16<mi16<",{"mi", 16, "mi", 16}},     // 0x24
        {"mi16<<",    {"mi", 16, "mi", 16}},     // 0x24
        {"mi16<mi32<",{"mi", 16, "mi", 32}},     // 0x25
        {"mi16<mi64<",{"mi", 16, "mi", 64}},     // 0x26
        {"mi16<mr8<", {"mi", 16, "mr", 8}},      // 0x27

        {"mi32<r8<",  {"mi", 32, "r", 8}},       // 0x28
        {"mi32<i8<",  {"mi", 32, "i", 8}},       // 0x29
        {"mi32<i16<", {"mi", 32, "i", 16}},      // 0x2A
        {"mi32<i32<", {"mi", 32, "i", 32}},      // 0x2B
        {"mi32<i64<", {"mi", 32, "i", 64}},      // 0x2C
        {"mi32<mi8<", {"mi", 32, "mi", 8}},      // 0x2D
        {"mi32<mi16<",{"mi", 32, "mi", 16}},     // 0x2E
        {"mi32<mi32<",{"mi", 32, "mi", 32}},     // 0x2F
        {"mi32<<",    {"mi", 32, "mi", 32}},     // 0x2F
        {"mi32<mi64<",{"mi", 32, "mi", 64}},     // 0x30
        {"mi32<mr8<", {"mi", 32, "mr", 8}},      // 0x31

        {"mi64<r8<",  {"mi", 64, "r", 8}},       // 0x32
        {"mi64<i8<",  {"mi", 64, "i", 8}},       // 0x33
        {"mi64<i16<", {"mi", 64, "i", 16}},      // 0x34
        {"mi64<i32<", {"mi", 64, "i", 32}},      // 0x35
        {"mi64<i64<", {"mi", 64, "i", 64}},      // 0x36
        {"mi64<mi8<", {"mi", 64, "mi", 8}},      // 0x37
        {"mi64<mi16<",{"mi", 64, "mi", 16}},     // 0x38
        {"mi64<mi32<",{"mi", 64, "mi", 32}},     // 0x39
        {"mi64<mi64<",{"mi", 64, "mi", 64}},     // 0x3A
        {"mi64<<",    {"mi", 64, "mi", 64}},     // 0x3A
        {"mi64<mr8<", {"mi", 64, "mr", 8}},      // 0x3B

        {"mr8<r8<",   {"mr", 8, "r", 8}},        // 0x3C
        {"mr8<i8<",   {"mr", 8, "i", 8}},        // 0x3D
        {"mr8<i16<",  {"mr", 8, "i", 16}},       // 0x3E
        {"mr8<i32<",  {"mr", 8, "i", 32}},       // 0x3F
        {"mr8<i64<",  {"mr", 8, "i", 64}},       // 0x40
        {"mr8<mi8<",  {"mr", 8, "mi", 8}},       // 0x41
        {"mr8<mi16<", {"mr", 8, "mi", 16}},      // 0x42
        {"mr8<mi32<", {"mr", 8, "mi", 32}},      // 0x43
        {"mr8<mi64<", {"mr", 8, "mi", 64}},      // 0x44
        {"mr8<mr8<",  {"mr", 8, "mr", 8}},       // 0x45
        {"mr8<<",     {"mr", 8, "mr", 8}},       // 0x45

        {"r8<i<",     {"r", 8, "i", 0}},         // 0x46
        {"mi8<i<",    {"mi", 8, "i", 0}},        // 0x47
        {"mi16<i<",   {"mi", 16, "i", 0}},       // 0x48
        {"mi32<i<",   {"mi", 32, "i", 0}},       // 0x49
        {"mi64<i<",   {"mi", 64, "i", 0}},       // 0x4A
        {"mr8<i<",    {"mr", 8, "i", 0}},        // 0x4B
    };

    inline static const std::unordered_map<std::string, operandInfo> operandMapBytes = {
        {"r8<",       {"r", 1, "", 0}},
        {"i8<",       {"i", 1, "", 0}},
        {"i16<",      {"i", 2, "", 0}},
        {"i32<",      {"i", 4, "", 0}},
        {"i64<",      {"i", 8, "", 0}},
        {"mi8<",      {"mi", 1, "", 0}},
        {"mi16<",     {"mi", 2, "", 0}},
        {"mi32<",     {"mi", 4, "", 0}},
        {"mi64<",     {"mi", 8, "", 0}},
        {"mr8<",      {"mr", 1, "", 0}},

        {"r8<r8<",    {"r", 1, "r", 1}},
        {"r8<<",      {"r", 1, "r", 1}},
        {"r8<i8<",    {"r", 1, "i", 1}},
        {"r8<i16<",   {"r", 1, "i", 2}},
        {"r8<i32<",   {"r", 1, "i", 4}},
        {"r8<i64<",   {"r", 1, "i", 8}},
        {"r8<mi8<",   {"r", 1, "mi", 1}},
        {"r8<mi16<",  {"r", 1, "mi", 2}},
        {"r8<mi32<",  {"r", 1, "mi", 4}},
        {"r8<mi64<",  {"r", 1, "mi", 8}},
        {"r8<mr8<",   {"r", 1, "mr", 1}},

        {"mi8<r8<",   {"mi", 1, "r", 1}},
        {"mi8<i8<",   {"mi", 1, "i", 1}},
        {"mi8<i16<",  {"mi", 1, "i", 2}},
        {"mi8<i32<",  {"mi", 1, "i", 4}},
        {"mi8<i64<",  {"mi", 1, "i", 8}},
        {"mi8<mi8<",  {"mi", 1, "mi", 1}},
        {"mi8<<",     {"mi", 1, "mi", 1}},
        {"mi8<mi16<", {"mi", 1, "mi", 2}},
        {"mi8<mi32<", {"mi", 1, "mi", 4}},
        {"mi8<mi64<", {"mi", 1, "mi", 8}},
        {"mi8<mr8<",  {"mi", 1, "mr", 1}},

        {"mi16<r8<",  {"mi", 2, "r", 1}},
        {"mi16<i8<",  {"mi", 2, "i", 1}},
        {"mi16<i16<", {"mi", 2, "i", 2}},
        {"mi16<i32<", {"mi", 2, "i", 4}},
        {"mi16<i64<", {"mi", 2, "i", 8}},
        {"mi16<mi8<", {"mi", 2, "mi", 1}},
        {"mi16<mi16<",{"mi", 2, "mi", 2}},
        {"mi16<<",    {"mi", 2, "mi", 2}},
        {"mi16<mi32<",{"mi", 2, "mi", 4}},
        {"mi16<mi64<",{"mi", 2, "mi", 8}},
        {"mi16<mr8<", {"mi", 2, "mr", 1}},

        {"mi32<r8<",  {"mi", 4, "r", 1}},
        {"mi32<i8<",  {"mi", 4, "i", 1}},
        {"mi32<i16<", {"mi", 4, "i", 2}},
        {"mi32<i32<", {"mi", 4, "i", 4}},
        {"mi32<i64<", {"mi", 4, "i", 8}},
        {"mi32<mi8<", {"mi", 4, "mi", 1}},
        {"mi32<mi16<",{"mi", 4, "mi", 2}},
        {"mi32<mi32<",{"mi", 4, "mi", 4}},
        {"mi32<<",    {"mi", 4, "mi", 4}},
        {"mi32<mi64<",{"mi", 4, "mi", 8}},
        {"mi32<mr8<", {"mi", 4, "mr", 1}},

        {"mi64<r8<",  {"mi", 8, "r", 1}},
        {"mi64<i8<",  {"mi", 8, "i", 1}},
        {"mi64<i16<", {"mi", 8, "i", 2}},
        {"mi64<i32<", {"mi", 8, "i", 4}},
        {"mi64<i64<", {"mi", 8, "i", 8}},
        {"mi64<mi8<", {"mi", 8, "mi", 1}},
        {"mi64<mi16<",{"mi", 8, "mi", 2}},
        {"mi64<mi32<",{"mi", 8, "mi", 4}},
        {"mi64<mi64<",{"mi", 8, "mi", 8}},
        {"mi64<<",    {"mi", 8, "mi", 8}},
        {"mi64<mr8<", {"mi", 8, "mr", 1}},

        {"mr8<r8<",   {"mr", 1, "r", 1}},
        {"mr8<i8<",   {"mr", 1, "i", 1}},
        {"mr8<i16<",  {"mr", 1, "i", 2}},
        {"mr8<i32<",  {"mr", 1, "i", 4}},
        {"mr8<i64<",  {"mr", 1, "i", 8}},
        {"mr8<mi8<",  {"mr", 1, "mi", 1}},
        {"mr8<mi16<", {"mr", 1, "mi", 2}},
        {"mr8<mi32<", {"mr", 1, "mi", 4}},
        {"mr8<mi64<", {"mr", 1, "mi", 8}},
        {"mr8<mr8<",  {"mr", 1, "mr", 1}},
        {"mr8<<",     {"mr", 1, "mr", 1}},
    };

    inline static const std::unordered_set<std::string> operandInfoMnemonics = {
        "mval mb8", "mval mb16", "mval mb32", "mval mb64"
    };

    std::unordered_map<std::string, std::vector<uint8_t>> mnemonicOpcode = {
        {"nac", {0x00}}, {"mval mb8", {0x10}}, {"mval mb16", {0x11}}, {"mval mb32", {0x12}},
        {"mval mb64", {0x13}}, {"wait", {0xFC}}, {"stop", {0xFD}}, {"sleepms", {0xFE}}, {"sleepsec", {0xFF}},
    };

    std::vector<uint8_t> intToBytes(std::string value, int amountOfBytes);

    void pushData(std::vector<uint8_t> data);

    void checkFileExtension(std::string fileName, std::string fileExtension);

    std::string trim(const std::string& str);
    std::vector<std::string> split(const std::string& str);

    std::vector<uint8_t> getOpcode(std::string mnemonic);
    std::vector<uint8_t> getOperandInfoBytes(std::string operandDescriptor);

    void error(std::string errorType, std::string info) const;
};

extern Assembler assembler;