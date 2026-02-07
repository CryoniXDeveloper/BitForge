#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <cstdint>
#include <cstdlib>

class ROM {
    std::vector<uint8_t> data;
    void error(std::string errorType, std::string info);
    
public:
    void loadFromFile(const std::string& filename);
    bool testing = false;
    bool testingErrorSuccess;

    ROM();

    void setTesting(bool t) {
        testing = t;
    }

    bool getTestingErrorResult() const {
        return testingErrorSuccess;
    }

    void resetErrorResult() {
        testingErrorSuccess = false;
    }

    uint8_t read8(uint64_t address);
    uint16_t read16(uint64_t address);
    uint32_t read32(uint64_t address);
    uint64_t read64(uint64_t address);

    std::vector<uint8_t> readBytesVector(uint64_t address, size_t length);
};

extern ROM rom;
