#pragma once
#include <vector>
#include <cstdint>
#include <string>
#include <iostream>
#include <cstdlib>

struct RAM {
    std::vector<uint8_t> memory;

    RAM(int sizeInMB);

    uint8_t  read8(uint64_t address) const;
    uint16_t read16(uint64_t start) const;
    uint32_t read32(uint64_t start) const;
    uint64_t read64(uint64_t start) const;

    std::vector<uint8_t> readBytesVector(uint64_t start, size_t length) const;

    void write8(uint64_t address, uint8_t value);
    void writeBytesVector(uint64_t start, const std::vector<uint8_t>& data);

    size_t size() const;

    void error(std::string errorType, std::string info = "") const;
};

extern RAM ram;