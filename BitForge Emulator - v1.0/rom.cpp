#include "rom.h"
#include "motherboard.h"

ROM::ROM() {
    data.reserve(Motherboard::ROM_SIZE);
    data.resize(Motherboard::ROM_SIZE);
}

void ROM::loadFromFile(const std::string& filename) {
    std::ifstream f(filename, std::ios::binary | std::ios::ate);
    if (!f.is_open()) error("RO01FTOF", filename);

    size_t fileSize = static_cast<size_t>(f.tellg());
    if (fileSize != Motherboard::ROM_SIZE) {
        error("RO03FSII", std::to_string(fileSize));
    }

    f.seekg(0, std::ios::beg);
    f.read(reinterpret_cast<char*>(data.data()), fileSize);

    if (!f) error("RO02FTRF", filename);
}

uint8_t ROM::read8(uint64_t address) {
    if (address < Motherboard::ROM_START || address > Motherboard::ROM_END)
        error("RO04AOOB", "Absolute address: " + std::to_string(address));

    return data[address - Motherboard::ROM_START];
}

uint16_t ROM::read16(uint64_t start) {
    uint64_t address = start - Motherboard::ROM_START;

    if (start < Motherboard::ROM_START || address + 1 > Motherboard::ROM_SIZE - 1)
        error("RO05AOOB", "Absolute address: " + std::to_string(start) + " + length (2)");

    uint16_t result = 0;
    for (size_t i = 0; i < 2; i++)
        result |= ((uint16_t)data[address + i]) << (8 * i);

    return result;
}

uint32_t ROM::read32(uint64_t start) {
    uint64_t address = start - Motherboard::ROM_START;

    if (start < Motherboard::ROM_START || address + 3 > Motherboard::ROM_SIZE - 1)
        error("RO06AOOB", "Absolute address: " + std::to_string(start) + " + length (4)");

    uint32_t result = 0;
    for (size_t i = 0; i < 4; i++)
        result |= ((uint32_t)data[address + i]) << (8 * i);

    return result;
}

uint64_t ROM::read64(uint64_t start) {
    uint64_t address = start - Motherboard::ROM_START;

    if (start < Motherboard::ROM_START || address + 7 > Motherboard::ROM_SIZE - 1)
        error("RO07AOOB", "Absolute address: " + std::to_string(start) + " + length (8)");

    uint64_t result = 0;
    for (size_t i = 0; i < 8; i++)
        result |= ((uint64_t)data[address + i]) << (8 * i);

    return result;
}

std::vector<uint8_t> ROM::readBytesVector(uint64_t start, size_t length) {
    uint64_t address = start - Motherboard::ROM_START;

    if (start < Motherboard::ROM_START || address + length - 1 > Motherboard::ROM_SIZE - 1) {
        error("RO08AOOB", "Absolute address: " + std::to_string(start) + " + length (" + std::to_string(length) + ")");
        return {0};

    } else {
        return std::vector<uint8_t>(data.begin() + address, data.begin() + address + length);
    }
}

void ROM::error(std::string errorType, std::string info) {
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

ROM rom;
