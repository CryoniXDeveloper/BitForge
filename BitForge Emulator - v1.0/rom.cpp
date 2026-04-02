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

    return static_cast<uint16_t>(data[address]) |
        (static_cast<uint16_t>(data[address + 1]) << 8);
}

uint32_t ROM::read32(uint64_t start) {
    uint64_t address = start - Motherboard::ROM_START;

    if (start < Motherboard::ROM_START || address + 3 > Motherboard::ROM_SIZE - 1)
        error("RO06AOOB", "Absolute address: " + std::to_string(start) + " + length (4)");

    return static_cast<uint32_t>(data[address]) |
        (static_cast<uint32_t>(data[address + 1]) << 8) |
        (static_cast<uint32_t>(data[address + 2]) << 16) |
        (static_cast<uint32_t>(data[address + 3]) << 24);
}

uint64_t ROM::read64(uint64_t start) {
    uint64_t address = start - Motherboard::ROM_START;

    if (start < Motherboard::ROM_START || address + 7 > Motherboard::ROM_SIZE - 1)
        error("RO07AOOB", "Absolute address: " + std::to_string(start) + " + length (8)");

    return static_cast<uint64_t>(data[address]) |
        (static_cast<uint64_t>(data[address + 1]) << 8) |
        (static_cast<uint64_t>(data[address + 2]) << 16) |
        (static_cast<uint64_t>(data[address + 3]) << 24) |
        (static_cast<uint64_t>(data[address + 4]) << 32) |
        (static_cast<uint64_t>(data[address + 5]) << 40) |
        (static_cast<uint64_t>(data[address + 6]) << 48) |
        (static_cast<uint64_t>(data[address + 7]) << 56);
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
