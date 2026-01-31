#include "ram.h"
#include <fstream>
#include <cstring>

RAM::RAM(int sizeInMB) {
    memory.reserve(sizeInMB * 1024ull * 1024ull);
    memory.resize(sizeInMB * 1024ull * 1024ull, 0);
}

uint8_t RAM::read8(uint64_t address) const {
    if (address >= memory.size())
        error("RA01AOOB", std::to_string(address));
    return memory[address];
}

uint16_t RAM::read16(uint64_t start) const {
    if (start + 2 > memory.size())
    error("RA02AOOB", std::to_string(start) + " + length (" + std::to_string(2) + ")");
    uint16_t result = 0;
    for (size_t i = 0; i < 2; i++)
    result |= ((uint16_t)memory[start + i]) << (8 * i);
    return result;
}

uint32_t RAM::read32(uint64_t start) const {
    if (start + 4 > memory.size())
    error("RA03AOOB", std::to_string(start) + " + length (" + std::to_string(4) + ")");
    uint32_t result = 0;
    for (size_t i = 0; i < 4; i++)
    result |= ((uint32_t)memory[start + i]) << (8 * i);
    return result;
}

uint64_t RAM::read64(uint64_t start) const {
    if (start + 8 > memory.size())
    error("RA04AOOB", std::to_string(start) + " + length (" + std::to_string(8) + ")");
    uint64_t result = 0;
    for (size_t i = 0; i < 8; i++)
    result |= ((uint64_t)memory[start + i]) << (8 * i);
    return result;
}

std::vector<uint8_t> RAM::readBytesVector(uint64_t start, size_t length) const {
    if (start + length > memory.size())
        error("RA05AOOB", std::to_string(start) + " + length (" + std::to_string(length) + ")");
    return std::vector<uint8_t>(memory.begin() + start, memory.begin() + start + length);
}

void RAM::write8(uint64_t address, uint8_t value) {
    if (address >= memory.size())
        error("RA06AOOB", std::to_string(address));
    memory[address] = value;
}

void RAM::writeBytesVector(uint64_t start, const std::vector<uint8_t>& data) {
    if (start + data.size() > memory.size())
        error("RA07AOOB", std::to_string(start) + " + data size (" + std::to_string(data.size()) + ")");
    std::copy(data.begin(), data.end(), memory.begin() + start);
}

size_t RAM::size() const {
    return memory.size();
}

void RAM::error(std::string errorType, std::string info) const {
    std::string returnString = "ERROR [" + errorType + "]";
    if (info != "")
        returnString += " - More info: " + info;
    returnString += "\n\nFind out more in errorTypes.txt.\nStopping execution...";
    std::cout << returnString;
    std::cin.get();
    exit(0);
}

RAM ram(128);
