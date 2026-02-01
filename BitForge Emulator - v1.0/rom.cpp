#include "rom.h"
#include "motherboard.h"

ROM::ROM() {
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

uint8_t ROM::read8(uint64_t address) const {
    if (address > data.size())
        error("RO04AOOB", std::to_string(address));
    return data[address];
}

uint16_t ROM::read16(uint64_t start) const {
    if (start + 2 > data.size())
        error("RO05AOOB", std::to_string(start) + " + length (2)");
    uint16_t result = 0;
    for (size_t i = 0; i < 2; i++)
        result |= ((uint16_t)data[start + i]) << (8 * i);
    return result;
}

uint32_t ROM::read32(uint64_t start) const {
    if (start + 4 > data.size())
        error("RO06AOOB", std::to_string(start) + " + length (4)");
    uint32_t result = 0;
    for (size_t i = 0; i < 4; i++)
        result |= ((uint32_t)data[start + i]) << (8 * i);
    return result;
}

uint64_t ROM::read64(uint64_t start) const {
    if (start + 8 > data.size())
        error("RO07AOOB", std::to_string(start) + " + length (8)");
    uint64_t result = 0;
    for (size_t i = 0; i < 8; i++)
        result |= ((uint64_t)data[start + i]) << (8 * i);
    return result;
}

std::vector<uint8_t> ROM::readBytesVector(uint64_t address, size_t length) const {
    if (address + length > data.size())
        error("RO08AOOB", std::to_string(address) + " + length (" + std::to_string(length) + ")");
    return std::vector<uint8_t>(data.begin() + address, data.begin() + address + length);
}

void ROM::error(const std::string& e, const std::string& i) const {
    std::string r = "ERROR [" + e + "]";
    if (!i.empty()) r += " - " + i;
    r += "\nStopping execution...";
    std::cout << r;
    std::exit(0);
}

ROM rom;
