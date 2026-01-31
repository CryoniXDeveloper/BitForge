#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <cstdint>
#include <cstdlib>

class ROM {
    std::vector<uint8_t> data;
    void error(const std::string& e, const std::string& i = "") const;

public:
    void loadFromFile(const std::string& filename);

    uint8_t read8(uint64_t address) const;
    uint16_t read16(uint64_t address) const;
    uint32_t read32(uint64_t address) const;
    uint64_t read64(uint64_t address) const;

    std::vector<uint8_t> readBytesVector(uint64_t address, size_t length) const;
    size_t size() const;
};

extern ROM rom;
