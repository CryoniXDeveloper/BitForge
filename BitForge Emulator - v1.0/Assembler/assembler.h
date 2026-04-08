#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>

struct Label {
    std::string name;
    uint32_t address;
};

static const std::unordered_map<std::string, uint8_t> general0Operands = {
    {"nac",   0x00},
    {"ret",   0xAF},
    {"pusha", 0xB8},
    {"popa",  0xB9},
    {"pushf", 0xBA},
    {"popf",  0xBB},
    {"clc",   0xBC},
    {"stc",   0xBD},
    {"cld",   0xBE},
    {"std",   0xBF},
    {"cli",   0xC0},
    {"sti",   0xC1},
    {"clo",   0xC2},
    {"enter", 0xDE},
    {"leave", 0xDF},
    {"iret",  0xE7},
    {"wait",  0xFC},
    {"stop",  0xFD},
};

static const std::unordered_map<std::string, uint8_t> generalJumps = {
    {"jmp", 0x9C},
    {"jz",  0x9D}, {"jnz", 0x9E},
    {"jl",  0x9F}, {"jg",  0xA0}, {"jle", 0xA1}, {"jge", 0xA2},
    {"jb",  0xA3}, {"ja",  0xA4}, {"jbe", 0xA5}, {"jae", 0xA6},
    {"jo",  0xA7}, {"jno", 0xA8},
    {"js",  0xA9}, {"jns", 0xAA},
    {"jc",  0xAB}, {"jnc", 0xAC},
    {"call",0xAE},
};

static const std::unordered_map<std::string, uint8_t> general1Operands = {
    {"inc8",    0x35}, {"inc16",   0x36}, {"inc32",   0x37}, {"inc64",   0x38},
    {"dec8",    0x39}, {"dec16",   0x3A}, {"dec32",   0x3B}, {"dec64",   0x3C},
    {"neg8",    0x45}, {"neg16",   0x46}, {"neg32",   0x47}, {"neg64",   0x48},
    {"abs8",    0x98}, {"abs16",   0x99}, {"abs32",   0x9A}, {"abs64",   0x9B},
    {"bnot8",   0x61}, {"bnot16",  0x62}, {"bnot32",  0x63}, {"bnot64",  0x64},
    {"bswap16", 0x85}, {"bswap32", 0x86}, {"bswap64", 0x87},
    {"push8",   0xB0}, {"push16",  0xB1}, {"push32",  0xB2}, {"push64",  0xB3},
    {"pop8",    0xB4}, {"pop16",   0xB5}, {"pop32",   0xB6}, {"pop64",   0xB7},
    {"sleepms", 0xFE}, {"sleepsec",0xFF},
    {"int",     0xE6},
};

static const std::unordered_map<std::string, std::vector<uint8_t>> general2Operands = {
    {"mval8",  {0x10}}, {"mval16", {0x11}}, {"mval32", {0x12}}, {"mval64", {0x13}},
    {"scmp8",  {0x49}}, {"ucmp8",  {0x4A}}, {"scmp16", {0x4B}}, {"ucmp16", {0x4C}},
    {"scmp32", {0x4D}}, {"ucmp32", {0x4E}}, {"scmp64", {0x4F}}, {"ucmp64", {0x50}},
    {"test8",  {0x51}}, {"test16", {0x52}}, {"test32", {0x53}}, {"test64", {0x54}},
    {"btst8",  {0x81}}, {"btst16", {0x82}}, {"btst32", {0x83}}, {"btst64", {0x84}},
    {"loop",   {0xAD}},
    {"xchg",   {0xC3}},
    {"sext8",  {0xE0}}, {"sext16", {0xE1}}, {"sext32", {0xE2}},
    {"zext8",  {0xE3}}, {"zext16", {0xE4}}, {"zext32", {0xE5}},
    {"in8",    {0xE8}}, {"in16",   {0xE9}}, {"in32",   {0xEA}}, {"in64",   {0xEB}},
    {"out8",   {0xEC}}, {"out16",  {0xED}}, {"out32",  {0xEE}}, {"out64",  {0xEF}},
    {"popcnt8", {0x01,0x14}}, {"popcnt16",{0x01,0x15}}, {"popcnt32",{0x01,0x16}}, {"popcnt64",{0x01,0x17}},
    {"clz8",   {0x01,0x18}}, {"clz16",  {0x01,0x19}}, {"clz32",  {0x01,0x1A}}, {"clz64",  {0x01,0x1B}},
    {"ctz8",   {0x01,0x1C}}, {"ctz16",  {0x01,0x1D}}, {"ctz32",  {0x01,0x1E}}, {"ctz64",  {0x01,0x1F}},
    {"bsf8",   {0x01,0x20}}, {"bsf16",  {0x01,0x21}}, {"bsf32",  {0x01,0x22}}, {"bsf64",  {0x01,0x23}},
    {"bsr8",   {0x01,0x24}}, {"bsr16",  {0x01,0x25}}, {"bsr32",  {0x01,0x26}}, {"bsr64",  {0x01,0x27}},
};

static const std::unordered_map<std::string, std::vector<uint8_t>> general3Operands = {
    {"sadd8",  {0x15}}, {"uadd8",  {0x16}}, {"sadd16", {0x17}}, {"uadd16", {0x18}},
    {"sadd32", {0x19}}, {"uadd32", {0x1A}}, {"sadd64", {0x1B}}, {"uadd64", {0x1C}},
    {"ssub8",  {0x1D}}, {"usub8",  {0x1E}}, {"ssub16", {0x1F}}, {"usub16", {0x20}},
    {"ssub32", {0x21}}, {"usub32", {0x22}}, {"ssub64", {0x23}}, {"usub64", {0x24}},
    {"smul8",  {0x25}}, {"umul8",  {0x26}}, {"smul16", {0x27}}, {"umul16", {0x28}},
    {"smul32", {0x29}}, {"umul32", {0x2A}}, {"smul64", {0x2B}}, {"umul64", {0x2C}},
    {"sdiv8",  {0x2D}}, {"udiv8",  {0x2E}}, {"sdiv16", {0x2F}}, {"udiv16", {0x30}},
    {"sdiv32", {0x31}}, {"udiv32", {0x32}}, {"sdiv64", {0x33}}, {"udiv64", {0x34}},
    {"smod8",  {0x3D}}, {"umod8",  {0x3E}}, {"smod16", {0x3F}}, {"umod16", {0x40}},
    {"smod32", {0x41}}, {"umod32", {0x42}}, {"smod64", {0x43}}, {"umod64", {0x44}},
    {"and8",   {0x55}}, {"and16",  {0x56}}, {"and32",  {0x57}}, {"and64",  {0x58}},
    {"or8",    {0x59}}, {"or16",   {0x5A}}, {"or32",   {0x5B}}, {"or64",   {0x5C}},
    {"xor8",   {0x5D}}, {"xor16",  {0x5E}}, {"xor32",  {0x5F}}, {"xor64",  {0x60}},
    {"shl8",   {0x65}}, {"shl16",  {0x66}}, {"shl32",  {0x67}}, {"shl64",  {0x68}},
    {"shr8",   {0x69}}, {"shr16",  {0x6A}}, {"shr32",  {0x6B}}, {"shr64",  {0x6C}},
    {"sar8",   {0x6D}}, {"sar16",  {0x6E}}, {"sar32",  {0x6F}}, {"sar64",  {0x70}},
    {"andn8",  {0x71}}, {"andn16", {0x72}}, {"andn32", {0x73}}, {"andn64", {0x74}},
    {"bset8",  {0x75}}, {"bset16", {0x76}}, {"bset32", {0x77}}, {"bset64", {0x78}},
    {"bclr8",  {0x79}}, {"bclr16", {0x7A}}, {"bclr32", {0x7B}}, {"bclr64", {0x7C}},
    {"bflip8", {0x7D}}, {"bflip16",{0x7E}}, {"bflip32",{0x7F}}, {"bflip64",{0x80}},
    {"sadc8",  {0x88}}, {"uadc8",  {0x89}}, {"sadc16", {0x8A}}, {"uadc16", {0x8B}},
    {"sadc32", {0x8C}}, {"uadc32", {0x8D}}, {"sadc64", {0x8E}}, {"uadc64", {0x8F}},
    {"ssbc8",  {0x90}}, {"usbc8",  {0x91}}, {"ssbc16", {0x92}}, {"usbc16", {0x93}},
    {"ssbc32", {0x94}}, {"usbc32", {0x95}}, {"ssbc64", {0x96}}, {"usbc64", {0x97}},
    {"smulhi64",{0xC4}},{"umulhi64",{0xC5}},
    {"smin8",  {0xC6}}, {"umin8",  {0xC7}}, {"smax8",  {0xC8}}, {"umax8",  {0xC9}},
    {"smin16", {0xCA}}, {"umin16", {0xCB}}, {"smax16", {0xCC}}, {"umax16", {0xCD}},
    {"smin32", {0xCE}}, {"umin32", {0xCF}}, {"smax32", {0xD0}}, {"umax32", {0xD1}},
    {"smin64", {0xD2}}, {"umin64", {0xD3}}, {"smax64", {0xD4}}, {"umax64", {0xD5}},
    {"nand8",  {0x01,0x00}}, {"nand16",{0x01,0x01}}, {"nand32",{0x01,0x02}}, {"nand64",{0x01,0x03}},
    {"nor8",   {0x01,0x04}}, {"nor16", {0x01,0x05}}, {"nor32", {0x01,0x06}}, {"nor64", {0x01,0x07}},
    {"xnor8",  {0x01,0x08}}, {"xnor16",{0x01,0x09}}, {"xnor32",{0x01,0x0A}}, {"xnor64",{0x01,0x0B}},
    {"rol8",   {0x01,0x0C}}, {"rol16", {0x01,0x0D}}, {"rol32", {0x01,0x0E}}, {"rol64", {0x01,0x0F}},
    {"ror8",   {0x01,0x10}}, {"ror16", {0x01,0x11}}, {"ror32", {0x01,0x12}}, {"ror64", {0x01,0x13}},
};

static const std::unordered_map<std::string, uint8_t> general4Operands = {
    {"sclamp8",  0xD6}, {"uclamp8",  0xD7},
    {"sclamp16", 0xD8}, {"uclamp16", 0xD9},
    {"sclamp32", 0xDA}, {"uclamp32", 0xDB},
    {"sclamp64", 0xDC}, {"uclamp64", 0xDD},
};

struct Assembler {
    std::vector<uint8_t> output;
    std::vector<Label> labels;

    std::string trim(const std::string& s);
    std::vector<std::string> split(const std::string& s);
    std::vector<std::string> readFile(const std::string& filename);
    void secondPass(const std::vector<std::string>& lines);
    void writeOutput(const std::string& filename);
    void error(const std::string& type, const std::string& info) const;
    std::vector<std::string> parseDescriptor(const std::string& desc);
    std::vector<uint8_t> autoSizeBytes(const std::string& valueToken);

    uint8_t descriptorByte(const std::string& type);
    int operandSize(const std::string& type);
    void encodeOperand(const std::string& type, const std::string& valueToken);
};

extern Assembler assembler;