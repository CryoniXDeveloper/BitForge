#include "ram.h"
#include "motherboard.h"
#include <fstream>
#include <cstring>

RAM memory;

RAM::RAM() {
    memory.reserve(Motherboard::RAM_SIZE);
    memory.resize(Motherboard::RAM_SIZE);
}

uint8_t RAM::read8(uint64_t address) {
    if (address < Motherboard::RAM_START || address > Motherboard::RAM_END) {
        error("RA01AOOB", "Absolute address: " + std::to_string(address));
        return 0;

    } else {
        return memory[address - Motherboard::RAM_START];
    }
}

uint16_t RAM::read16(uint64_t start) {
    uint64_t address = start - Motherboard::RAM_START;

    if (start < Motherboard::RAM_START || address + 1 > Motherboard::RAM_SIZE - 1) {
        error("RA02AOOB", "Absolute address: " + std::to_string(start) + " + length (" + std::to_string(2) + ")");
        return 0;

    } else {
        uint16_t result = 0; 
        for (size_t i = 0; i < 2; i++)
        result |= ((uint16_t)memory[address + i]) << (8 * i);
        return result;
    }
}

uint32_t RAM::read32(uint64_t start) {
    uint64_t address = start - Motherboard::RAM_START;

    if (start < Motherboard::RAM_START || address + 3 > Motherboard::RAM_SIZE - 1) {
        error("RA03AOOB", "Absolute address: " + std::to_string(start) + " + length (" + std::to_string(4) + ")");
        return 0;

    } else {
        uint32_t result = 0;
        for (size_t i = 0; i < 4; i++)
        result |= ((uint32_t)memory[address + i]) << (8 * i);
        return result;
    }
}

uint64_t RAM::read64(uint64_t start) {
    uint64_t address = start - Motherboard::RAM_START;

    if (start < Motherboard::RAM_START || address + 7 > Motherboard::RAM_SIZE - 1) {
        error("RA04AOOB", "Absolute address: " + std::to_string(start) + " + length (" + std::to_string(8) + ")");
        return 0;

    } else {
        uint64_t result = 0;
        for (size_t i = 0; i < 8; i++)
        result |= ((uint64_t)memory[address + i]) << (8 * i);
        return result;
    }
}

std::vector<uint8_t> RAM::readBytesVector(uint64_t start, size_t length) {
    uint64_t address = start - Motherboard::RAM_START;

    if (start < Motherboard::RAM_START || address + length - 1 > Motherboard::RAM_SIZE - 1) {
        error("RA05AOOB", "Absolute address: " + std::to_string(start) + " + length (" + std::to_string(length) + ")");
        return {0};

    } else {
        return std::vector<uint8_t>(memory.begin() + address, memory.begin() + address + length);
    }
}

void RAM::write8(uint64_t address, uint8_t value) {
    if (address < Motherboard::RAM_START || address > Motherboard::RAM_END) {
        error("RA06AOOB", "Absolute address: " + std::to_string(address));

    } else {
        memory[address - Motherboard::RAM_START] = value;
    }
}

void RAM::writeBytesVector(uint64_t start, const std::vector<uint8_t>& data) {
    if (start < Motherboard::RAM_START) {
        error("RA07AOOB", "Absolute address: " + std::to_string(start) + " + data size (" + std::to_string(data.size()) + ")");
        return; 
    }

    uint64_t address = start - Motherboard::RAM_START;

    if (address + data.size() > Motherboard::RAM_SIZE) {
        error("RA07AOOB", "Absolute address: " + std::to_string(start) + " + data size (" + std::to_string(data.size()) + ")");
        return;
    }

    std::copy(data.begin(), data.end(), memory.begin() + address);
}

void RAM::error(std::string errorType, std::string info) {
    if (!testing) {
        std::string returnString = "ERROR [" + errorType + "]";
        if (info != "")
            returnString += " - More info: " + info;
        returnString += "\n\nFind out more in errorTypes.txt.\nStopping execution...";
        std::cout << returnString;
        std::cin.get();
        exit(0);

    } else {
        testingErrorSuccess = true;
    }
}