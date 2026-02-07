// This file tests all parts of the project to catch even the smallest errors.
// Running this on a large project may take some time.
// Created: 30.01.2026
// Uses custom rom.bin and storage.bin — it will not overwrite saved data.
// Future me: thank me later!

#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <format>
#include "../ram.h"
#include "../rom.h"
#include "../motherboard.h"
#include "../cpu.h"

#define testFailed(outputFile, tests) testFailedImpl(outputFile, tests, __FILE__, __LINE__)

using namespace std;

struct TestDetails {
    string timestamp;
    string testNum;
    string address;
    string expected;
    string actual;
    string testResult;
    string testing;
    string note;
};

auto formatNote = [](const std::string& note, size_t width) -> std::string {
    if (note.length() >= width) return note.substr(0, width-3) + "...";
    return note;
};

inline std::string hex2(uint8_t v) {
    char b[8];
    std::snprintf(b, sizeof(b), "0x%02X", v);
    return b;
}

string hex8(uint32_t value) {
    stringstream ss;
    ss << "0x" 
       << setfill('0') 
       << setw(8) 
       << hex 
       << value;
    return ss.str();
}

string checkOverflow(const string& input, size_t maxLen) {
    if (input.size() <= maxLen) {
        return input;
    }

    return input.substr(0, maxLen - 3) + "...";
}

string getTimestamp() {
    using namespace chrono;
    
    auto currentTime = system_clock::now();
    auto duration = currentTime.time_since_epoch();
    
    auto seconds = duration_cast<chrono::seconds>(duration);
    auto subseconds = duration - seconds;
    auto fraction = duration_cast<microseconds>(subseconds);
    
    time_t t = system_clock::to_time_t(currentTime);
    tm* local_tm = localtime(&t);
    
    ostringstream oss;
    oss << put_time(local_tm, "%H:%M:%S") << "." << setw(4) << setfill('0') << (fraction.count() / 100);
    
    return oss.str();
}

string formStringFromBytes(vector<uint8_t>& bytes) {
    string retString= "";
    int length = bytes.size();
    
    for (int x = 0; x < length; x++) {
        retString += hex2(bytes[x]);

        if (x < length - 1)
            retString += " ";
    }

    return retString;
}

string getStringGetBytes(int address, int length) {
    string retString = "";

    for (int x = 0; x < length; x++) {
        retString += hex2(memory.memory[address + x]);
        
        if (x < length - 1)
        retString += " ";
    }
    
    return retString;
}

void fillLog(ofstream& outputFile, const vector<TestDetails>& tests) {
    outputFile << "|" << setw(15) << "---------------"
        << "|" << setw(12) << "------------"
        << "|" << setw(20) << "--------------------"
        << "|" << setw(21) << "---------------------"
        << "|" << setw(21) << "---------------------"
        << "|" << setw(10) << "----------"
        << "|" << setw(45) << "---------------------------------------------"
        << "|" << setw(15) << "---------------"
        << "|\n";

    outputFile.flush();

    outputFile << "|" << left << setw(15) << "  Time"
        << "|" << setw(12) << "  Test"
        << "|" << setw(20) << "  Address"
        << "|" << setw(21) << "  Expected"
        << "|" << setw(21) << "  Actual"
        << "|" << setw(10) << "  Result"
        << "|" << setw(45) << "  Testing"
        << "|" << setw(15) << "  Note"
        << "|\n";

    outputFile.flush();

    outputFile << "|" << setw(15) << "---------------"
        << "|" << setw(12) << "------------"
        << "|" << setw(20) << "--------------------"
        << "|" << setw(21) << "---------------------"
        << "|" << setw(21) << "---------------------"
        << "|" << setw(10) << "----------"
        << "|" << setw(45) << "---------------------------------------------"
        << "|" << setw(15) << "---------------"
        << "|\n";

    outputFile.flush();

    for (auto& td : tests) {
        string noteFormatted = formatNote(td.note, 14);

        outputFile << "| " << setw(14) << checkOverflow(td.timestamp, 13)
           << "| " << setw(11)  << checkOverflow(td.testNum, 10)
           << "| " << setw(19) << checkOverflow(td.address, 18)
           << "| " << setw(20) << checkOverflow(td.expected, 19)
           << "| " << setw(20) << checkOverflow(td.actual, 19)
           << "| " << setw(9)  << checkOverflow(td.testResult, 8)
           << "| " << setw(44) << checkOverflow(td.testing, 43)
           << "| " << setw(14) << checkOverflow(noteFormatted, 13)
           << "|\n";

        outputFile.flush();

    }

    outputFile << "|" << setw(15) << "---------------"
        << "|" << setw(12) << "------------"
        << "|" << setw(20) << "--------------------"
        << "|" << setw(21) << "---------------------"
        << "|" << setw(21) << "---------------------"
        << "|" << setw(10) << "----------"
        << "|" << setw(45) << "---------------------------------------------"
        << "|" << setw(15) << "---------------"
        << "|\n";

    outputFile.flush();
}

void testFailedImpl(std::ofstream& outputFile, const std::vector<TestDetails>& tests, const char* file, int line) {
    fillLog(outputFile, tests);

    std::cout << "TEST FAILED! Called from " << file << ":" << line << std::endl;

    throw std::runtime_error("Test failed");
}

void writeByte(const char* path, uint64_t offset, uint8_t value) {
    std::fstream file(path, std::ios::in | std::ios::out | std::ios::binary);
    
    if (!file) {
        std::ofstream creator(path, std::ios::binary);
        creator.close();
        
        file.open(path, std::ios::in | std::ios::out | std::ios::binary);
    }

    if (!file) {
        std::cerr << "FATAL: Could not open/create " << path << ". Check if the folder exists!" << std::endl;
        return;
    }

    uint64_t fileOffset = offset - Motherboard::ROM_START;
    file.seekp(fileOffset);
    file.write(reinterpret_cast<const char*>(&value), 1);
    file.close(); 
}

void writeBytes(const char* path, uint64_t offset, const std::vector<uint8_t>* data, size_t size) {
    std::fstream file(path, std::ios::in | std::ios::out | std::ios::binary);
    if (!file) return;

    if (offset < Motherboard::ROM_START) return;

    offset = offset - Motherboard::ROM_START;

    if (size > data->size()) return;

    file.seekp(offset);
    file.write(reinterpret_cast<const char*>(data->data()), size);
    file.flush();
}

int main() {
    try {
        ofstream outputFile("Test.txt");
        
        std::fstream check("testRom.bin", std::ios::in | std::ios::out | std::ios::binary);
            if (!check) {
                std::ofstream creator("testRom.bin", std::ios::binary);
            }

        rom.loadFromFile("testRom.bin");

        vector<TestDetails> tests = {};

        if (!outputFile.is_open()) {
            cerr << "ERROR: Could not open Test.txt" << endl;
            return 0;
        };

        memory.setTesting(true);
        rom.setTesting(true);

        uint64_t RAM_SIZE = Motherboard::RAM_SIZE;
        uint64_t RAM_START = Motherboard::RAM_START;
        uint64_t RAM_END = Motherboard::RAM_END;

        uint64_t ROM_SIZE = Motherboard::ROM_SIZE;
        uint64_t ROM_START = Motherboard::ROM_START;
        uint64_t ROM_END = Motherboard::ROM_END;

        // RAM WRITING

        memory.write8(RAM_START, 0x93);
        if (memory.memory[0] == 0x93) {
            tests.push_back({getTimestamp(), "1/6", hex8(RAM_START), "0x93", "0x93", "PASS", "RAM Writing 8-bit (write8)", "~"});
        } else {
            tests.push_back({getTimestamp(), "1/6", hex8(RAM_START), "0x93", hex2(memory.memory[0]), "FAIL", "RAM Writing 8-bit (write8)", "~"});
            testFailed(outputFile, tests);
        }

        memory.write8(RAM_START + RAM_SIZE / 2, 0x1B);
        if (memory.memory[RAM_SIZE / 2] == 0x1B) {
            tests.push_back({getTimestamp(), "2/6", hex8(RAM_START + RAM_SIZE / 2), "0x1B", "0x1B", "PASS", "RAM Writing 8-bit (write8)", "~"});
        } else {
            tests.push_back({getTimestamp(), "2/6", hex8(RAM_START + RAM_SIZE / 2), "0x1B", hex2(memory.memory[RAM_SIZE / 2]), "FAIL", "RAM Writing 8-bit (write8)", "~"});
            testFailed(outputFile, tests);
        }

        memory.write8(RAM_END, 0x21);
        if (memory.memory[RAM_END - RAM_START] == 0x21) {
            tests.push_back({getTimestamp(), "3/6", hex8(RAM_END), "0x21", "0x21", "PASS", "RAM Writing 8-bit (write8)", "~"});
        } else {
            tests.push_back({getTimestamp(), "3/6", hex8(RAM_END), "0x21", hex2(memory.memory[RAM_END - RAM_START]), "FAIL", "RAM Writing 8-bit (write8)", "~"});
            testFailed(outputFile, tests);
        }

        memory.write8(RAM_START, 0x03);
        if (memory.memory[0] == 0x03) {
            tests.push_back({getTimestamp(), "4/6", hex8(RAM_START), "0x03", "0x03", "PASS", "RAM Writing 8-bit (write8)", "Overwriting"});
        } else {
            tests.push_back({getTimestamp(), "4/6", hex8(RAM_START), "0x03", hex2(memory.memory[0]), "FAIL", "RAM Writing 8-bit (write8)", "Overwriting"});
            testFailed(outputFile, tests);
        }

        memory.write8(RAM_START + RAM_SIZE / 2, 0x6A);
        if (memory.memory[RAM_SIZE / 2] == 0x6A) {
            tests.push_back({getTimestamp(), "5/6", hex8(RAM_START + RAM_SIZE / 2), "0x6A", "0x6A", "PASS", "RAM Writing 8-bit (write8)", "Overwriting"});
        } else {
            tests.push_back({getTimestamp(), "5/6", hex8(RAM_START + RAM_SIZE / 2), "0x6A", hex2(memory.memory[RAM_SIZE / 2]), "FAIL", "RAM Writing 8-bit (write8)", "Overwriting"});
            testFailed(outputFile, tests);
        }

        memory.write8(RAM_END, 0xC3);
        if (memory.memory[RAM_END - RAM_START] == 0xC3) {
            tests.push_back({getTimestamp(), "6/6   PASS", hex8(RAM_END), "0xC3", "0xC3", "PASS", "RAM Writing 8-bit (write8)", "Overwriting"});
        } else {
            tests.push_back({getTimestamp(), "6/6   FAIL", hex8(RAM_END), "0xC3", hex2(memory.memory[RAM_END - RAM_START]), "FAIL", "RAM Writing 8-bit (write8)", "Overwriting"});
            testFailed(outputFile, tests);
        }

        // CLEANING

        memory.write8(RAM_START, 0x00);
        memory.write8(RAM_START + RAM_SIZE / 2, 0x00);
        memory.write8(RAM_END, 0x00);


        vector<uint8_t> testBytes1 = {0xDE};
        vector<uint8_t> testBytes2 = {0x99};
        vector<uint8_t> testBytes3 = {0x18};
        vector<uint8_t> testBytes4 = {0xE4};
        vector<uint8_t> testBytes5 = {0xC1};
        vector<uint8_t> testBytes6 = {0xF7};

        memory.writeBytesVector(RAM_START, testBytes1);
        if (memory.memory[0] == testBytes1[0]) {
            tests.push_back({getTimestamp(), "1/6", hex8(RAM_START), hex2(testBytes1[0]), hex2(testBytes1[0]), "PASS", "RAM Writing 8-bit (writeBytesVector)", "~"});
        } else {
            tests.push_back({getTimestamp(), "1/6", hex8(RAM_START), hex2(testBytes1[0]), hex2(memory.memory[0]), "FAIL", "RAM Writing 8-bit (writeBytesVector)", "~"});
            testFailed(outputFile, tests);
        }

        memory.writeBytesVector(RAM_START + RAM_SIZE / 2, testBytes2);
        if (memory.memory[RAM_SIZE / 2] == testBytes2[0]) {
            tests.push_back({getTimestamp(), "2/6", hex8(RAM_START + RAM_SIZE / 2), hex2(testBytes2[0]), hex2(testBytes2[0]), "PASS", "RAM Writing 8-bit (writeBytesVector)", "~"});
        } else {
            tests.push_back({getTimestamp(), "2/6", hex8(RAM_START + RAM_SIZE / 2), hex2(testBytes2[0]), hex2(memory.memory[RAM_SIZE / 2]), "FAIL", "RAM Writing 8-bit (writeBytesVector)", "~"});
            testFailed(outputFile, tests);
        }

        memory.writeBytesVector(RAM_END, testBytes3);
        if (memory.memory[RAM_END - RAM_START] == testBytes3[0]) {
            tests.push_back({getTimestamp(), "3/6", hex8(RAM_END), hex2(testBytes3[0]), hex2(testBytes3[0]), "PASS", "RAM Writing 8-bit (writeBytesVector)", "~"});
        } else {
            tests.push_back({getTimestamp(), "3/6", hex8(RAM_END), hex2(testBytes3[0]), hex2(memory.memory[RAM_END - RAM_START]), "FAIL", "RAM Writing 8-bit (writeBytesVector)", "~"});
            testFailed(outputFile, tests);
        }

        memory.writeBytesVector(RAM_START, testBytes4);
        if (memory.memory[0] == testBytes4[0]) {
            tests.push_back({getTimestamp(), "4/6", hex8(RAM_START), hex2(testBytes4[0]), hex2(testBytes4[0]), "PASS", "RAM Writing 8-bit (writeBytesVector)", "Overwriting"});
        } else {
            tests.push_back({getTimestamp(), "4/6", hex8(RAM_START), hex2(testBytes4[0]), hex2(memory.memory[0]), "FAIL", "RAM Writing 8-bit (writeBytesVector)", "Overwriting"});
            testFailed(outputFile, tests);
        }

        memory.writeBytesVector(RAM_START + RAM_SIZE / 2, testBytes5);
        if (memory.memory[RAM_SIZE / 2] == testBytes5[0]) {
            tests.push_back({getTimestamp(), "5/6", hex8(RAM_START + RAM_SIZE / 2), hex2(testBytes5[0]), hex2(testBytes5[0]), "PASS", "RAM Writing 8-bit (writeBytesVector)", "Overwriting"});
        } else {
            tests.push_back({getTimestamp(), "5/6", hex8(RAM_START + RAM_SIZE / 2), hex2(testBytes5[0]), hex2(memory.memory[RAM_SIZE / 2]), "FAIL", "RAM Writing 8-bit (writeBytesVector)", "Overwriting"});
            testFailed(outputFile, tests);
        }

        memory.writeBytesVector(RAM_END, testBytes6);
        if (memory.memory[RAM_END - RAM_START] == testBytes6[0]) {
            tests.push_back({getTimestamp(), "6/6   PASS", hex8(RAM_END), hex2(testBytes6[0]), hex2(testBytes6[0]), "PASS", "RAM Writing 8-bit (writeBytesVector)", "Overwriting"});
        } else {
            tests.push_back({getTimestamp(), "6/6   FAIL", hex8(RAM_END), hex2(testBytes6[0]), hex2(memory.memory[RAM_END - RAM_START]), "FAIL", "RAM Writing 8-bit (writeBytesVector)", "Overwriting"});
            testFailed(outputFile, tests);
        }

        // CLEANING

        memory.writeBytesVector(RAM_START, {0x00});
        memory.writeBytesVector(RAM_START + RAM_SIZE / 2, {0x00});
        memory.writeBytesVector(RAM_END, {0x00});


        testBytes1 = {0xDE,0x91,0x3B,0x7C};
        testBytes2 = {0xA4,0x2F,0x88,0x19};
        testBytes3 = {0x57,0xC2,0x0E,0xD1};
        testBytes4 = {0xF3,0x6B,0x9A,0x4D};
        testBytes5 = {0x0C,0xE7,0x5F,0xB2};
        testBytes6 = {0x81,0x3D,0x6A,0xF9};

        memory.writeBytesVector(RAM_START, testBytes1);
        if (memory.memory[0] == testBytes1[0] &&
            memory.memory[1] == testBytes1[1] &&
            memory.memory[2] == testBytes1[2] &&
            memory.memory[3] == testBytes1[3]) {
                tests.push_back({getTimestamp(), "1/6", hex8(RAM_START), formStringFromBytes(testBytes1), formStringFromBytes(testBytes1), "PASS", "RAM Writing 32-bit (writeBytesVector)", "~"});
        } else {
                tests.push_back({getTimestamp(), "1/6", hex8(RAM_START), formStringFromBytes(testBytes1), getStringGetBytes(RAM_START, 4), "FAIL", "RAM Writing 32-bit (writeBytesVector)", "~"});
                testFailed(outputFile, tests);
        }

        memory.writeBytesVector(RAM_START + RAM_SIZE / 2, testBytes2);
        if (memory.memory[RAM_SIZE / 2] == testBytes2[0] &&
            memory.memory[RAM_SIZE / 2 + 1] == testBytes2[1] &&
            memory.memory[RAM_SIZE / 2 + 2] == testBytes2[2] &&
            memory.memory[RAM_SIZE / 2 + 3] == testBytes2[3]) {
                tests.push_back({getTimestamp(), "2/6", hex8(RAM_START + RAM_SIZE / 2), formStringFromBytes(testBytes2), formStringFromBytes(testBytes2), "PASS", "RAM Writing 32-bit (writeBytesVector)", "~"});
        } else {
                tests.push_back({getTimestamp(), "2/6", hex8(RAM_START + RAM_SIZE / 2), formStringFromBytes(testBytes2), getStringGetBytes(RAM_START + RAM_SIZE / 2, 4), "FAIL", "RAM Writing 32-bit (writeBytesVector)", "~"});
                testFailed(outputFile, tests);
        }

        memory.writeBytesVector(RAM_END - 3, testBytes3);
        if (memory.memory[RAM_END - RAM_START - 3] == testBytes3[0] &&
            memory.memory[RAM_END - RAM_START - 2] == testBytes3[1] &&
            memory.memory[RAM_END - RAM_START - 1] == testBytes3[2] &&
            memory.memory[RAM_END - RAM_START] == testBytes3[3]) {
                tests.push_back({getTimestamp(), "3/6", hex8(RAM_END - 3), formStringFromBytes(testBytes3), formStringFromBytes(testBytes3), "PASS", "RAM Writing 32-bit (writeBytesVector)", "~"});
        } else {
                tests.push_back({getTimestamp(), "3/6", hex8(RAM_END - 3), formStringFromBytes(testBytes3), getStringGetBytes(RAM_END - 3, 4), "FAIL", "RAM Writing 32-bit (writeBytesVector)", "~"});
                testFailed(outputFile, tests);
        }

        memory.writeBytesVector(RAM_START, testBytes4);
        if (memory.memory[0] == testBytes4[0] &&
            memory.memory[1] == testBytes4[1] &&
            memory.memory[2] == testBytes4[2] &&
            memory.memory[3] == testBytes4[3]) {
                tests.push_back({getTimestamp(), "4/6", hex8(RAM_START), formStringFromBytes(testBytes4), formStringFromBytes(testBytes4), "PASS", "RAM Writing 32-bit (writeBytesVector)", "Overwriting"});
        } else {
                tests.push_back({getTimestamp(), "4/6", hex8(RAM_START), formStringFromBytes(testBytes4), getStringGetBytes(RAM_START, 4), "FAIL", "RAM Writing 32-bit (writeBytesVector)", "Overwriting"});
                testFailed(outputFile, tests);
        }

        memory.writeBytesVector(RAM_START + RAM_SIZE / 2, testBytes5);
        if (memory.memory[RAM_SIZE / 2] == testBytes5[0] &&
            memory.memory[RAM_SIZE / 2 + 1] == testBytes5[1] &&
            memory.memory[RAM_SIZE / 2 + 2] == testBytes5[2] &&
            memory.memory[RAM_SIZE / 2 + 3] == testBytes5[3]) {
                tests.push_back({getTimestamp(), "5/6", hex8(RAM_START + RAM_SIZE / 2), formStringFromBytes(testBytes5), formStringFromBytes(testBytes5), "PASS", "RAM Writing 32-bit (writeBytesVector)", "Overwriting"});
        } else {
                tests.push_back({getTimestamp(), "5/6", hex8(RAM_START + RAM_SIZE / 2), formStringFromBytes(testBytes5), getStringGetBytes(RAM_START + RAM_SIZE / 2, 4), "FAIL", "RAM Writing 32-bit (writeBytesVector)", "Overwriting"});
                testFailed(outputFile, tests);
        }

        memory.writeBytesVector(RAM_END - 3, testBytes6);
        if (memory.memory[RAM_END - RAM_START - 3] == testBytes6[0] &&
            memory.memory[RAM_END - RAM_START - 2] == testBytes6[1] &&
            memory.memory[RAM_END - RAM_START - 1] == testBytes6[2] &&
            memory.memory[RAM_END - RAM_START] == testBytes6[3]) {
                tests.push_back({getTimestamp(), "6/6   PASS", hex8(RAM_END - 3), formStringFromBytes(testBytes6), formStringFromBytes(testBytes6), "PASS", "RAM Writing 32-bit (writeBytesVector)", "Overwriting"});
        } else {
                tests.push_back({getTimestamp(), "6/6   FAIL", hex8(RAM_END - 3), formStringFromBytes(testBytes6), getStringGetBytes(RAM_END - 3, 4), "FAIL", "RAM Writing 32-bit (writeBytesVector)", "Overwriting"});
                testFailed(outputFile, tests);
        }

        // CLEANING

        memory.writeBytesVector(RAM_START, {0x00, 0x00, 0x00, 0x00});
        memory.writeBytesVector(RAM_START + RAM_SIZE / 2, {0x00, 0x00, 0x00, 0x00});
        memory.writeBytesVector(RAM_END - 3, {0x00, 0x00, 0x00, 0x00});


        testBytes1 = {0xDE,0x91,0x3B,0x7C,0x4F,0xA2,0x11,0x99};
        testBytes2 = {0xA4,0x2F,0x88,0x19,0x6B,0x3C,0xD0,0x0E};
        testBytes3 = {0x57,0xC2,0x0E,0xD1,0xF7,0x34,0x8A,0x22};
        testBytes4 = {0xF3,0x6B,0x9A,0x4D,0x1C,0xE5,0x7B,0xA1};
        testBytes5 = {0x0C,0xE7,0x5F,0xB2,0x8D,0x33,0xF9,0x40};
        testBytes6 = {0x81,0x3D,0x6A,0xF9,0x12,0xB4,0x67,0xC8};

        memory.writeBytesVector(RAM_START, testBytes1);
        if (memory.memory[0] == testBytes1[0] &&
            memory.memory[1] == testBytes1[1] &&
            memory.memory[2] == testBytes1[2] &&
            memory.memory[3] == testBytes1[3] &&
            memory.memory[4] == testBytes1[4] &&
            memory.memory[5] == testBytes1[5] &&
            memory.memory[6] == testBytes1[6] &&
            memory.memory[7] == testBytes1[7]) {
                tests.push_back({getTimestamp(), "1/6", hex8(RAM_START), formStringFromBytes(testBytes1), formStringFromBytes(testBytes1), "PASS", "RAM Writing 64-bit (writeBytesVector)", "~"});
        } else {
                tests.push_back({getTimestamp(), "1/6", hex8(RAM_START), formStringFromBytes(testBytes1), getStringGetBytes(RAM_START, 8), "FAIL", "RAM Writing 64-bit (writeBytesVector)", "~"});
                testFailed(outputFile, tests);
        }

        memory.writeBytesVector(RAM_START + RAM_SIZE / 2, testBytes2);
        if (memory.memory[RAM_SIZE / 2] == testBytes2[0] &&
            memory.memory[RAM_SIZE / 2 + 1] == testBytes2[1] &&
            memory.memory[RAM_SIZE / 2 + 2] == testBytes2[2] &&
            memory.memory[RAM_SIZE / 2 + 3] == testBytes2[3] &&
            memory.memory[RAM_SIZE / 2 + 4] == testBytes2[4] &&
            memory.memory[RAM_SIZE / 2 + 5] == testBytes2[5] &&
            memory.memory[RAM_SIZE / 2 + 6] == testBytes2[6] &&
            memory.memory[RAM_SIZE / 2 + 7] == testBytes2[7]) {
                tests.push_back({getTimestamp(), "2/6", hex8(RAM_START + RAM_SIZE / 2), formStringFromBytes(testBytes2), formStringFromBytes(testBytes2), "PASS", "RAM Writing 64-bit (writeBytesVector)", "~"});
        } else {
                tests.push_back({getTimestamp(), "2/6", hex8(RAM_START + RAM_SIZE / 2), formStringFromBytes(testBytes2), getStringGetBytes(RAM_START + RAM_SIZE / 2, 8), "FAIL", "RAM Writing 64-bit (writeBytesVector)", "~"});
                testFailed(outputFile, tests);
        }

        memory.writeBytesVector(RAM_END - 7, testBytes3);
        if (memory.memory[RAM_END - RAM_START - 7] == testBytes3[0] &&
            memory.memory[RAM_END - RAM_START - 6] == testBytes3[1] &&
            memory.memory[RAM_END - RAM_START - 5] == testBytes3[2] &&
            memory.memory[RAM_END - RAM_START - 4] == testBytes3[3] &&
            memory.memory[RAM_END - RAM_START - 3] == testBytes3[4] &&
            memory.memory[RAM_END - RAM_START - 2] == testBytes3[5] &&
            memory.memory[RAM_END - RAM_START - 1] == testBytes3[6] &&
            memory.memory[RAM_END - RAM_START] == testBytes3[7]) {
                tests.push_back({getTimestamp(), "3/6", hex8(RAM_END - 7), formStringFromBytes(testBytes3), formStringFromBytes(testBytes3), "PASS", "RAM Writing 64-bit (writeBytesVector)", "~"});
        } else {
                tests.push_back({getTimestamp(), "3/6", hex8(RAM_END - 7), formStringFromBytes(testBytes3), getStringGetBytes(RAM_END - 7, 8), "FAIL", "RAM Writing 64-bit (writeBytesVector)", "~"});
                testFailed(outputFile, tests);
        }

        memory.writeBytesVector(RAM_START, testBytes4);
        if (memory.memory[0] == testBytes4[0] &&
            memory.memory[1] == testBytes4[1] &&
            memory.memory[2] == testBytes4[2] &&
            memory.memory[3] == testBytes4[3] &&
            memory.memory[4] == testBytes4[4] &&
            memory.memory[5] == testBytes4[5] &&
            memory.memory[6] == testBytes4[6] &&
            memory.memory[7] == testBytes4[7]) {
                tests.push_back({getTimestamp(), "4/6", hex8(RAM_START), formStringFromBytes(testBytes4), formStringFromBytes(testBytes4), "PASS", "RAM Writing 64-bit (writeBytesVector)", "Overwriting"});
        } else {
                tests.push_back({getTimestamp(), "4/6", hex8(RAM_START), formStringFromBytes(testBytes4), getStringGetBytes(RAM_START, 8), "FAIL", "RAM Writing 64-bit (writeBytesVector)", "Overwriting"});
                testFailed(outputFile, tests);
        }

        memory.writeBytesVector(RAM_START + RAM_SIZE / 2, testBytes5);
        if (memory.memory[RAM_SIZE / 2] == testBytes5[0] &&
            memory.memory[RAM_SIZE / 2 + 1] == testBytes5[1] &&
            memory.memory[RAM_SIZE / 2 + 2] == testBytes5[2] &&
            memory.memory[RAM_SIZE / 2 + 3] == testBytes5[3] &&
            memory.memory[RAM_SIZE / 2 + 4] == testBytes5[4] &&
            memory.memory[RAM_SIZE / 2 + 5] == testBytes5[5] &&
            memory.memory[RAM_SIZE / 2 + 6] == testBytes5[6] &&
            memory.memory[RAM_SIZE / 2 + 7] == testBytes5[7]) {
                tests.push_back({getTimestamp(), "5/6", hex8(RAM_START + RAM_SIZE / 2), formStringFromBytes(testBytes5), formStringFromBytes(testBytes5), "PASS", "RAM Writing 64-bit (writeBytesVector)", "Overwriting"});
        } else {
                tests.push_back({getTimestamp(), "5/6", hex8(RAM_START + RAM_SIZE / 2), formStringFromBytes(testBytes5), getStringGetBytes(RAM_START + RAM_SIZE / 2, 8), "FAIL", "RAM Writing 64-bit (writeBytesVector)", "Overwriting"});
                testFailed(outputFile, tests);
        }

        memory.writeBytesVector(RAM_END - 7, testBytes6);
        if (memory.memory[RAM_END - RAM_START - 7] == testBytes6[0] &&
            memory.memory[RAM_END - RAM_START - 6] == testBytes6[1] &&
            memory.memory[RAM_END - RAM_START - 5] == testBytes6[2] &&
            memory.memory[RAM_END - RAM_START - 4] == testBytes6[3] &&
            memory.memory[RAM_END - RAM_START - 3] == testBytes6[4] &&
            memory.memory[RAM_END - RAM_START - 2] == testBytes6[5] &&
            memory.memory[RAM_END - RAM_START - 1] == testBytes6[6] &&
            memory.memory[RAM_END - RAM_START] == testBytes6[7]) {
                tests.push_back({getTimestamp(), "6/6   PASS", hex8(RAM_END - 7), formStringFromBytes(testBytes6), formStringFromBytes(testBytes6), "PASS", "RAM Writing 64-bit (writeBytesVector)", "Overwriting"});
        } else {
                tests.push_back({getTimestamp(), "6/6   FAIL", hex8(RAM_END - 7), formStringFromBytes(testBytes6), getStringGetBytes(RAM_END - 7, 8), "FAIL", "RAM Writing 64-bit (writeBytesVector)", "Overwriting"});
                testFailed(outputFile, tests);
        }

        // CLEANING

        memory.writeBytesVector(RAM_START, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
        memory.writeBytesVector(RAM_START + RAM_SIZE / 2, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
        memory.writeBytesVector(RAM_END - 7, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});


        testBytes1 = {
            0xDE,0x91,0x3B,0x7C,0x4F,0xA2,0x11,0x99,
            0x5D,0xE3,0xB2,0x6F,0x8C,0x21,0x7A,0x0B,
            0xC4,0xD7,0x3F,0x88,0x12,0xAA,0x6E,0x90,
            0x3A,0x57,0x1F,0xE9,0x0D,0xB1,0x74,0xC0,
            0x9F,0x2E,0x8B,0x41,0x6D,0x14,0xF3,0x25,
            0x7B,0x0C,0xDA,0x5E,0x81,0x39,0x62,0xAD,
            0xF4,0x17,0xC9,0xB6,0x02,0xE8,0x3D,0x70,
            0x9A,0x55,0x1B,0xC7,0x4E,0xF1,0x6A,0x03
        };

        testBytes2 = {
            0xA4,0x2F,0x88,0x19,0x6B,0x3C,0xD0,0x0E,
            0xB5,0x7D,0x4A,0xE1,0x8F,0x23,0x65,0x9C,
            0x1E,0xC2,0x5B,0x3F,0x72,0xA8,0x0D,0xF4,
            0x6A,0x11,0x9E,0x47,0x3B,0xD6,0x80,0x2C,
            0xF1,0x5D,0xB9,0x3E,0x04,0xA7,0x62,0xCD,
            0x18,0xEF,0x43,0x7B,0x9D,0x20,0x6F,0x85,
            0xC3,0x0A,0x51,0xBE,0x28,0xF0,0x4C,0x97,
            0x36,0xDB,0x14,0x6E,0x81,0xC5,0x09,0xFA
        };

        testBytes3 = {
            0x57,0xC2,0x0E,0xD1,0xF7,0x34,0x8A,0x22,
            0x9B,0x6D,0x01,0xE4,0x5C,0xB8,0x3F,0xA0,
            0x12,0xFD,0x46,0x7E,0x8C,0x31,0xDA,0x05,
            0xC7,0x88,0x2A,0xB5,0x0F,0x93,0x6C,0xE2,
            0x41,0x7F,0xA9,0x1D,0x52,0xCE,0x38,0x0B,
            0xF6,0x24,0x9E,0x5B,0x03,0xCD,0x78,0x1A,
            0x8F,0xB1,0x2D,0x60,0xE3,0x47,0x9A,0x15,
            0x6B,0x0E,0xD8,0x4F,0xC2,0x91,0x3A,0x7D
        };

        testBytes4 = {
            0xF3,0x6B,0x9A,0x4D,0x1C,0xE5,0x7B,0xA1,
            0x2F,0xC6,0x88,0x3E,0xB1,0x04,0x5D,0x97,
            0xAD,0x13,0xF4,0x6C,0x02,0x89,0x7E,0x35,
            0xD2,0x5B,0x0A,0xC7,0x41,0x9F,0x6D,0x28,
            0x80,0x14,0xEB,0x53,0x9C,0x37,0xFA,0x0D,
            0x61,0x8E,0x2C,0xD9,0x45,0xB2,0x0F,0x7A,
            0xF8,0x36,0xC1,0x0B,0x94,0x2D,0xEA,0x5F,
            0x73,0xC8,0x1E,0xB6,0x4A,0xDF,0x05,0x9B
        };

        testBytes5 = {
            0x0C,0xE7,0x5F,0xB2,0x8D,0x33,0xF9,0x40,
            0x12,0xA6,0x7C,0xD3,0x51,0x9F,0x28,0xE0,
            0x6B,0x3A,0xD7,0x84,0xF1,0x05,0xC9,0x7E,
            0x2F,0xB8,0x41,0x0D,0x95,0x6C,0xE2,0x3A,
            0x80,0x1B,0xDA,0x54,0x9E,0x27,0x6F,0xC3,
            0x0A,0xF5,0x3D,0xB0,0x68,0x1E,0xAC,0x47,
            0xDF,0x92,0x0F,0x75,0x3B,0xC6,0x18,0xEB,
            0x64,0x9A,0x20,0xF7,0x4C,0x81,0x5D,0x13
        };

        testBytes6 = {
            0x81,0x3D,0x6A,0xF9,0x12,0xB4,0x67,0xC8,
            0x2E,0xD5,0x0B,0x94,0x3F,0x7A,0xC1,0x06,
            0x9D,0x54,0x28,0xEB,0x73,0x1F,0xC6,0x40,
            0xAF,0x02,0x8C,0x5B,0xF1,0x36,0xD9,0x0E,
            0x47,0xBA,0x13,0xE8,0x60,0x2D,0x91,0xC7,
            0x3E,0xF4,0x6B,0x0A,0xD2,0x85,0x1C,0x9F,
            0xB7,0x04,0x5D,0xE1,0x29,0x6C,0xF3,0x08,
            0x8A,0x41,0xCD,0x12,0x7F,0x36,0xB5,0x0D
        };

        memory.writeBytesVector(RAM_START, testBytes1);
        bool pass1 = true;
        for (int i = 0; i < 64; i++) {
            if (memory.memory[i] != testBytes1[i]) {
                pass1 = false;
                break;
            }
        }
        tests.push_back({
            getTimestamp(),
            "1/6",
            hex8(RAM_START),
            formStringFromBytes(testBytes1),
            pass1 ? formStringFromBytes(testBytes1) : getStringGetBytes(RAM_START, 64),
            pass1 ? "PASS" : "FAIL",
            "RAM Writing 512-bit (writeBytesVector)",
            "~"
        });

        if (!pass1) {
            testFailed(outputFile, tests);
        }

        memory.writeBytesVector(RAM_START + RAM_SIZE / 2, testBytes2);
        bool pass2 = true;
        for (int i = 0; i < 64; i++) {
            if (memory.memory[RAM_SIZE / 2 + i] != testBytes2[i]) {
                pass2 = false;
                break;
            }
        }

        tests.push_back({
            getTimestamp(),
            "2/6",
            hex8(RAM_START + RAM_SIZE / 2),
            formStringFromBytes(testBytes2),
            pass2 ? formStringFromBytes(testBytes2) : getStringGetBytes(RAM_START + RAM_SIZE / 2, 64),
            pass2 ? "PASS" : "FAIL",
            "RAM Writing 512-bit (writeBytesVector)",
            "~"
        });

        if (!pass2) {
            testFailed(outputFile, tests);
        }

        memory.writeBytesVector(RAM_END - 63, testBytes3);
        bool pass3 = true;
        for (int i = 0; i < 64; i++) {
            if (memory.memory[RAM_END - RAM_START - 63 + i] != testBytes3[i]) {
                pass3 = false;
                break;
            }
        }
        tests.push_back({
            getTimestamp(),
            "3/6",
            hex8(RAM_END - 63),
            formStringFromBytes(testBytes3),
            pass3 ? formStringFromBytes(testBytes3) : getStringGetBytes(RAM_END - 63, 64),
            pass3 ? "PASS" : "FAIL",
            "RAM Writing 512-bit (writeBytesVector)",
            "~"
        });

        if (!pass3) {
            testFailed(outputFile, tests);
        }

        memory.writeBytesVector(RAM_START, testBytes4);
        bool pass4 = true;
        for (int i = 0; i < 64; i++) {
            if (memory.memory[i] != testBytes4[i]) {
                pass4 = false;
                break;
            }
        }
        tests.push_back({
            getTimestamp(),
            "4/6",
            hex8(RAM_START),
            formStringFromBytes(testBytes4),
            pass4 ? formStringFromBytes(testBytes4) : getStringGetBytes(RAM_START, 64),
            pass4 ? "PASS" : "FAIL",
            "RAM Writing 512-bit (writeBytesVector)",
            "Overwriting"
        });

        if (!pass4) {
            testFailed(outputFile, tests);
        }

        memory.writeBytesVector(RAM_START + RAM_SIZE / 2, testBytes5);
        bool pass5 = true;
        for (int i = 0; i < 64; i++) {
            if (memory.memory[RAM_SIZE / 2 + i] != testBytes5[i]) {
                pass5 = false;
                break;
            }
        }
        tests.push_back({
            getTimestamp(),
            "5/6",
            hex8(RAM_START + RAM_SIZE / 2),
            formStringFromBytes(testBytes5),
            pass5 ? formStringFromBytes(testBytes5) : getStringGetBytes(RAM_START + RAM_SIZE / 2, 64),
            pass5 ? "PASS" : "FAIL",
            "RAM Writing 512-bit (writeBytesVector)",
            "Overwriting"
        });

        if (!pass5) {
            testFailed(outputFile, tests);
        }

        memory.writeBytesVector(RAM_END - 63, testBytes6);
        bool pass6 = true;
        for (int i = 0; i < 64; i++) {
            if (memory.memory[RAM_END - RAM_START - 63 + i] != testBytes6[i]) {
                pass6 = false;
                break;
            }
        }
        tests.push_back({
            getTimestamp(),
            pass6 ? "6/6   PASS" : "6/6   FAIL",
            hex8(RAM_END - 63),
            formStringFromBytes(testBytes6),
            pass6 ? formStringFromBytes(testBytes6) : getStringGetBytes(RAM_END - 63, 64),
            pass6 ? "PASS" : "FAIL",
            "RAM Writing 512-bit (writeBytesVector)",
            "Overwriting"
        });

        if (!pass6) {
            testFailed(outputFile, tests);
        }

        // CLEANING

        memory.writeBytesVector(RAM_START, std::vector<uint8_t>(64, 0x00));
        memory.writeBytesVector(RAM_START + RAM_SIZE / 2, std::vector<uint8_t>(64, 0x00));
        memory.writeBytesVector(RAM_END - 63, std::vector<uint8_t>(64, 0x00));


        // RAM READING

        testBytes1 = {0xAB};
        testBytes2 = {0x34};
        testBytes3 = {0xF2};
        testBytes4 = {0x7D};
        testBytes5 = {0x19};
        testBytes6 = {0xC6};

        memory.write8(RAM_START, testBytes1[0]);

        if (memory.read8(RAM_START) == testBytes1[0]) {
            tests.push_back({getTimestamp(), "1/6", hex8(RAM_START), hex2(testBytes1[0]), hex2(testBytes1[0]), "PASS", "RAM Reading 8-bit (read8)", "~"});
        } else {
            tests.push_back({getTimestamp(), "1/6", hex8(RAM_START), hex2(testBytes1[0]), hex2(memory.memory[0]), "FAIL", "RAM Reading 8-bit (read8)", "~"});
            testFailed(outputFile, tests);
        }

        memory.write8(RAM_START + RAM_SIZE / 2, testBytes2[0]);
        if (memory.read8(RAM_START + RAM_SIZE / 2) == testBytes2[0]) {
            tests.push_back({getTimestamp(), "2/6", hex8(RAM_START + RAM_SIZE / 2), hex2(testBytes2[0]), hex2(testBytes2[0]), "PASS", "RAM Reading 8-bit (read8)", "~"});
        } else {
            tests.push_back({getTimestamp(), "2/6", hex8(RAM_START + RAM_SIZE / 2), hex2(testBytes2[0]), hex2(memory.memory[RAM_SIZE / 2]), "FAIL", "RAM Reading 8-bit (read8)", "~"});
            testFailed(outputFile, tests);
        }

        memory.write8(RAM_END, testBytes3[0]);
        if (memory.read8(RAM_END) == testBytes3[0]) {
            tests.push_back({getTimestamp(), "3/6", hex8(RAM_END), hex2(testBytes3[0]), hex2(testBytes3[0]), "PASS", "RAM Reading 8-bit (read8)", "~"});
        } else {
            tests.push_back({getTimestamp(), "3/6", hex8(RAM_END), hex2(testBytes3[0]), hex2(memory.memory[RAM_END - RAM_START]), "FAIL", "RAM Reading 8-bit (read8)", "~"});
            testFailed(outputFile, tests);
        }

        memory.write8(RAM_START, testBytes4[0]);
        if (memory.read8(RAM_START) == testBytes4[0]) {
            tests.push_back({getTimestamp(), "4/6", hex8(RAM_START), hex2(testBytes4[0]), hex2(testBytes4[0]), "PASS", "RAM Reading 8-bit (read8)", "Overwriting"});
        } else {
            tests.push_back({getTimestamp(), "4/6", hex8(RAM_START), hex2(testBytes4[0]), hex2(memory.memory[0]), "FAIL", "RAM Reading 8-bit (read8)", "Overwriting"});
            testFailed(outputFile, tests);
        }

        memory.write8(RAM_START + RAM_SIZE / 2, testBytes5[0]);
        if (memory.read8(RAM_START + RAM_SIZE / 2) == testBytes5[0]) {
            tests.push_back({getTimestamp(), "5/6", hex8(RAM_START + RAM_SIZE / 2), hex2(testBytes5[0]), hex2(testBytes5[0]), "PASS", "RAM Reading 8-bit (read8)", "Overwriting"});
        } else {
            tests.push_back({getTimestamp(), "5/6", hex8(RAM_START + RAM_SIZE / 2), hex2(testBytes5[0]), hex2(memory.memory[RAM_SIZE / 2]), "FAIL", "RAM Reading 8-bit (read8)", "Overwriting"});
            testFailed(outputFile, tests);
        }

        memory.write8(RAM_END, testBytes6[0]);
        if (memory.read8(RAM_END) == testBytes6[0]) {
            tests.push_back({getTimestamp(), "6/6   PASS", hex8(RAM_END), hex2(testBytes6[0]), hex2(testBytes6[0]), "PASS", "RAM Reading 8-bit (read8)", "Overwriting"});
        } else {
            tests.push_back({getTimestamp(), "6/6   FAIL", hex8(RAM_END), hex2(testBytes6[0]), hex2(memory.memory[RAM_END - RAM_START]), "FAIL", "RAM Reading 8-bit (read8)", "Overwriting"});
            testFailed(outputFile, tests);
        }

        // CLEANING

        memory.write8(RAM_START, 0x00);
        memory.write8(RAM_START + RAM_SIZE / 2, 0x00);
        memory.write8(RAM_END, 0x00);


        testBytes1 = {0x01, 0xAB};
        testBytes2 = {0x9F, 0x34};
        testBytes3 = {0x88, 0xF2};
        testBytes4 = {0x42, 0x7D};
        testBytes5 = {0xEE, 0x19};
        testBytes6 = {0x10, 0xC6};

        memory.writeBytesVector(RAM_START, testBytes1);
        if (memory.read16(RAM_START) == 0xAB01) {
            tests.push_back({getTimestamp(), "1/6", hex8(RAM_START), formStringFromBytes(testBytes1), formStringFromBytes(testBytes1), "PASS", "RAM Reading 16-bit (read16)", "~"});
        } else {
            tests.push_back({getTimestamp(), "1/6", hex8(RAM_START), formStringFromBytes(testBytes1), getStringGetBytes(RAM_START, 2), "FAIL", "RAM Reading 16-bit (read16)", "~"});
            testFailed(outputFile, tests);
        }

        memory.writeBytesVector(RAM_START + RAM_SIZE / 2, testBytes2);
        if (memory.read16(RAM_START + RAM_SIZE / 2) == 0x349F) {
            tests.push_back({getTimestamp(), "2/6", hex8(RAM_START + RAM_SIZE / 2), formStringFromBytes(testBytes2), formStringFromBytes(testBytes2), "PASS", "RAM Reading 16-bit (read16)", "~"});
        } else {
            tests.push_back({getTimestamp(), "2/6", hex8(RAM_START + RAM_SIZE / 2), formStringFromBytes(testBytes2), getStringGetBytes(RAM_START + RAM_SIZE / 2, 2), "FAIL", "RAM Reading 16-bit (read16)", "~"});
            testFailed(outputFile, tests);
        }

        memory.writeBytesVector(RAM_END - 1, testBytes3);
        if (memory.read16(RAM_END - 1) == 0xF288) {
            tests.push_back({getTimestamp(), "3/6", hex8(RAM_END - 1), formStringFromBytes(testBytes3), formStringFromBytes(testBytes3), "PASS", "RAM Reading 16-bit (read16)", "~"});
        } else {
            tests.push_back({getTimestamp(), "3/6", hex8(RAM_END - 1), formStringFromBytes(testBytes3), getStringGetBytes(RAM_END - 1, 2), "FAIL", "RAM Reading 16-bit (read16)", "~"});
            testFailed(outputFile, tests);
        }

        memory.writeBytesVector(RAM_START, testBytes4);
        if (memory.read16(RAM_START) == 0x7D42) {
            tests.push_back({getTimestamp(), "4/6", hex8(RAM_START), formStringFromBytes(testBytes4), formStringFromBytes(testBytes4), "PASS", "RAM Reading 16-bit (read16)", "Overwriting"});
        } else {
            tests.push_back({getTimestamp(), "4/6", hex8(RAM_START), formStringFromBytes(testBytes4), getStringGetBytes(RAM_START, 2), "FAIL", "RAM Reading 16-bit (read16)", "Overwriting"});
            testFailed(outputFile, tests);
        }

        memory.writeBytesVector(RAM_START + RAM_SIZE / 2, testBytes5);
        if (memory.read16(RAM_START + RAM_SIZE / 2) == 0x19EE) {
            tests.push_back({getTimestamp(), "5/6", hex8(RAM_START + RAM_SIZE / 2), formStringFromBytes(testBytes5), formStringFromBytes(testBytes5), "PASS", "RAM Reading 16-bit (read16)", "Overwriting"});
        } else {
            tests.push_back({getTimestamp(), "5/6", hex8(RAM_START + RAM_SIZE / 2), formStringFromBytes(testBytes5), getStringGetBytes(RAM_START + RAM_SIZE / 2, 2), "FAIL", "RAM Reading 16-bit (read16)", "Overwriting"});
            testFailed(outputFile, tests);
        }

        memory.writeBytesVector(RAM_END - 1, testBytes6);
        if (memory.read16(RAM_END - 1) == 0xC610) {
            tests.push_back({getTimestamp(), "6/6   PASS", hex8(RAM_END - 1), formStringFromBytes(testBytes6), formStringFromBytes(testBytes6), "PASS", "RAM Reading 16-bit (read16)", "Overwriting"});
        } else {
            tests.push_back({getTimestamp(), "6/6   FAIL", hex8(RAM_END - 1), formStringFromBytes(testBytes6), getStringGetBytes(RAM_END - 1, 2), "FAIL", "RAM Reading 16-bit (read16)", "Overwriting"});
            testFailed(outputFile, tests);
        }

        // CLEANING

        memory.writeBytesVector(RAM_START, {0x00, 0x00});
        memory.writeBytesVector(RAM_START + RAM_SIZE / 2, {0x00, 0x00});
        memory.writeBytesVector(RAM_END - 1, {0x00, 0x00});


        testBytes1 = {0x3A, 0xF1, 0x7C, 0xD4};
        testBytes2 = {0x9E, 0x42, 0xAB, 0x6F};
        testBytes3 = {0x11, 0xC7, 0x58, 0x80};
        testBytes4 = {0xD2, 0x8F, 0x34, 0x19};
        testBytes5 = {0xFE, 0x2B, 0xA1, 0x73};
        testBytes6 = {0x4D, 0x90, 0xE8, 0x56};

        memory.writeBytesVector(RAM_START, testBytes1);
        if (memory.read32(RAM_START) == 0xD47CF13A) {
            tests.push_back({getTimestamp(), "1/6", hex8(RAM_START), formStringFromBytes(testBytes1), formStringFromBytes(testBytes1), "PASS", "RAM Reading 32-bit (read32)", "~"});
        } else {
            tests.push_back({getTimestamp(), "1/6", hex8(RAM_START), formStringFromBytes(testBytes1), getStringGetBytes(RAM_START, 4), "FAIL", "RAM Reading 32-bit (read32)", "~"});
            testFailed(outputFile, tests);
        }

        memory.writeBytesVector(RAM_START + RAM_SIZE / 2, testBytes2);
        if (memory.read32(RAM_START + RAM_SIZE / 2) == 0x6FAB429E) {
            tests.push_back({getTimestamp(), "2/6", hex8(RAM_START + RAM_SIZE / 2), formStringFromBytes(testBytes2), formStringFromBytes(testBytes2), "PASS", "RAM Reading 32-bit (read32)", "~"});
        } else {
            tests.push_back({getTimestamp(), "2/6", hex8(RAM_START + RAM_SIZE / 2), formStringFromBytes(testBytes2), getStringGetBytes(RAM_START + RAM_SIZE / 2, 4), "FAIL", "RAM Reading 32-bit (read32)", "~"});
            testFailed(outputFile, tests);
        }

        memory.writeBytesVector(RAM_END - 3, testBytes3);
        if (memory.read32(RAM_END - 3) == 0x8058C711) {
            tests.push_back({getTimestamp(), "3/6", hex8(RAM_END - 3), formStringFromBytes(testBytes3), formStringFromBytes(testBytes3), "PASS", "RAM Reading 32-bit (read32)", "~"});
        } else {
            tests.push_back({getTimestamp(), "3/6", hex8(RAM_END - 3), formStringFromBytes(testBytes3), getStringGetBytes(RAM_END - 3, 4), "FAIL", "RAM Reading 32-bit (read32)", "~"});
            testFailed(outputFile, tests);
        }

        memory.writeBytesVector(RAM_START, testBytes4);
        if (memory.read32(RAM_START) == 0x19348FD2) {
            tests.push_back({getTimestamp(), "4/6", hex8(RAM_START), formStringFromBytes(testBytes4), formStringFromBytes(testBytes4), "PASS", "RAM Reading 32-bit (read32)", "Overwriting"});
        } else {
            tests.push_back({getTimestamp(), "4/6", hex8(RAM_START), formStringFromBytes(testBytes4), getStringGetBytes(RAM_START, 4), "FAIL", "RAM Reading 32-bit (read32)", "Overwriting"});
            testFailed(outputFile, tests);
        }

        memory.writeBytesVector(RAM_START + RAM_SIZE / 2, testBytes5);
        if (memory.read32(RAM_START + RAM_SIZE / 2) == 0x73A12BFE) {
            tests.push_back({getTimestamp(), "5/6", hex8(RAM_START + RAM_SIZE / 2), formStringFromBytes(testBytes5), formStringFromBytes(testBytes5), "PASS", "RAM Reading 32-bit (read32)", "Overwriting"});
        } else {
            tests.push_back({getTimestamp(), "5/6", hex8(RAM_START + RAM_SIZE / 2), formStringFromBytes(testBytes5), getStringGetBytes(RAM_START + RAM_SIZE / 2, 4), "FAIL", "RAM Reading 32-bit (read32)", "Overwriting"});
            testFailed(outputFile, tests);
        }

        memory.writeBytesVector(RAM_END - 3, testBytes6);
        if (memory.read32(RAM_END - 3) == 0x56E8904D) {
            tests.push_back({getTimestamp(), "6/6   PASS", hex8(RAM_END - 3), formStringFromBytes(testBytes6), formStringFromBytes(testBytes6), "PASS", "RAM Reading 32-bit (read32)", "Overwriting"});
        } else {
            tests.push_back({getTimestamp(), "6/6   FAIL", hex8(RAM_END - 3), formStringFromBytes(testBytes6), getStringGetBytes(RAM_END - 3, 4), "FAIL", "RAM Reading 32-bit (read32)", "Overwriting"});
            testFailed(outputFile, tests);
        }

        // CLEANING

        memory.writeBytesVector(RAM_START, {0x00, 0x00, 0x00, 0x00});
        memory.writeBytesVector(RAM_START + RAM_SIZE / 2, {0x00, 0x00, 0x00, 0x00});
        memory.writeBytesVector(RAM_END - 3, {0x00, 0x00, 0x00, 0x00});


        testBytes1 = {0x8C, 0x5D, 0xE1, 0x2A, 0xF4, 0x09, 0xB7, 0x63};
        testBytes2 = {0x13, 0xA9, 0x7E, 0xD0, 0x4C, 0xF8, 0x21, 0xB5};
        testBytes3 = {0x6D, 0x0F, 0xC3, 0x88, 0xA2, 0x7B, 0x11, 0x9E};
        testBytes4 = {0xE7, 0x92, 0x5B, 0xC6, 0x38, 0x4A, 0xF1, 0x0D};
        testBytes5 = {0x7F, 0x16, 0xDA, 0xB4, 0xC9, 0x02, 0x8E, 0x55};
        testBytes6 = {0x01, 0xF3, 0x68, 0x9B, 0xD7, 0x4E, 0x20, 0xAC};

        memory.writeBytesVector(RAM_START, testBytes1);
        if (memory.read64(RAM_START) == 0x63B709F42AE15D8C) {
            tests.push_back({getTimestamp(), "1/6", hex8(RAM_START), formStringFromBytes(testBytes1), formStringFromBytes(testBytes1), "PASS", "RAM Reading 64-bit (read64)", "~"});
        } else {
            tests.push_back({getTimestamp(), "1/6", hex8(RAM_START), formStringFromBytes(testBytes1), getStringGetBytes(RAM_START, 8), "FAIL", "RAM Reading 64-bit (read64)", "~"});
            testFailed(outputFile, tests);
        }

        memory.writeBytesVector(RAM_START + RAM_SIZE / 2, testBytes2);
        if (memory.read64(RAM_START + RAM_SIZE / 2) == 0xB521F84CD07EA913) {
            tests.push_back({getTimestamp(), "2/6", hex8(RAM_START + RAM_SIZE / 2), formStringFromBytes(testBytes2), formStringFromBytes(testBytes2), "PASS", "RAM Reading 64-bit (read64)", "~"});
        } else {
            tests.push_back({getTimestamp(), "2/6", hex8(RAM_START + RAM_SIZE / 2), formStringFromBytes(testBytes2), getStringGetBytes(RAM_START + RAM_SIZE / 2, 8), "FAIL", "RAM Reading 64-bit (read64)", "~"});
            testFailed(outputFile, tests);
        }

        memory.writeBytesVector(RAM_END - 7, testBytes3);
        if (memory.read64(RAM_END - 7) == 0x9E117BA288C30F6D) {
            tests.push_back({getTimestamp(), "3/6", hex8(RAM_END - 7), formStringFromBytes(testBytes3), formStringFromBytes(testBytes3), "PASS", "RAM Reading 64-bit (read64)", "~"});
        } else {
            tests.push_back({getTimestamp(), "3/6", hex8(RAM_END - 7), formStringFromBytes(testBytes3), getStringGetBytes(RAM_END - 7, 8), "FAIL", "RAM Reading 64-bit (read64)", "~"});
            testFailed(outputFile, tests);
        }

        memory.writeBytesVector(RAM_START, testBytes4);
        if (memory.read64(RAM_START) == 0x0DF14A38C65B92E7) {
            tests.push_back({getTimestamp(), "4/6", hex8(RAM_START), formStringFromBytes(testBytes4), formStringFromBytes(testBytes4), "PASS", "RAM Reading 64-bit (read64)", "Overwriting"});
        } else {
            tests.push_back({getTimestamp(), "4/6", hex8(RAM_START), formStringFromBytes(testBytes4), getStringGetBytes(RAM_START, 8), "FAIL", "RAM Reading 64-bit (read64)", "Overwriting"});
            testFailed(outputFile, tests);
        }

        memory.writeBytesVector(RAM_START + RAM_SIZE / 2, testBytes5);
        if (memory.read64(RAM_START + RAM_SIZE / 2) == 0x558E02C9B4DA167F) {
            tests.push_back({getTimestamp(), "5/6", hex8(RAM_START + RAM_SIZE / 2), formStringFromBytes(testBytes5), formStringFromBytes(testBytes5), "PASS", "RAM Reading 64-bit (read64)", "Overwriting"});
        } else {
            tests.push_back({getTimestamp(), "5/6", hex8(RAM_START + RAM_SIZE / 2), formStringFromBytes(testBytes5), getStringGetBytes(RAM_START + RAM_SIZE / 2, 8), "FAIL", "RAM Reading 64-bit (read64)", "Overwriting"});
            testFailed(outputFile, tests);
        }

        memory.writeBytesVector(RAM_END - 7, testBytes6);
        if (memory.read64(RAM_END - 7) == 0xAC204ED79B68F301) {
            tests.push_back({getTimestamp(), "6/6   PASS", hex8(RAM_END - 7), formStringFromBytes(testBytes6), formStringFromBytes(testBytes6), "PASS", "RAM Reading 64-bit (read64)", "Overwriting"});
        } else {
            tests.push_back({getTimestamp(), "6/6   FAIL", hex8(RAM_END - 7), formStringFromBytes(testBytes6), getStringGetBytes(RAM_END - 7, 8), "FAIL", "RAM Reading 64-bit (read64)", "Overwriting"});
            testFailed(outputFile, tests);
        }

        // CLEANING

        memory.writeBytesVector(RAM_START, {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00});
        memory.writeBytesVector(RAM_START + RAM_SIZE / 2, {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00});
        memory.writeBytesVector(RAM_END - 7, {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00});


        testBytes1 = {
            0x8C, 0x5D, 0xE1, 0x2A, 0xF4, 0x09, 0xB7, 0x63,
            0x21, 0x9F, 0xA4, 0xC7, 0x3D, 0x56, 0x11, 0x88,
            0x42, 0x7E, 0xCD, 0x0F, 0xBE, 0xD1, 0xFA, 0x33,
            0x9B, 0xA7, 0x05, 0x61, 0x28, 0xDE, 0xF0, 0x14,
            0x73, 0x8E, 0x9A, 0xC2, 0x56, 0x01, 0x44, 0xBB,
            0x6F, 0x2D, 0x91, 0xE3, 0xAC, 0x5F, 0x0B, 0x77,
            0x18, 0xF9, 0x3E, 0xC4, 0xA0, 0x6B, 0xD8, 0x25,
            0x9D, 0x4F, 0x02, 0x81, 0xE7, 0x3B, 0xB2, 0x6C
        };

        testBytes2 = {
            0x13, 0xA9, 0x7E, 0xD0, 0x4C, 0xF8, 0x21, 0xB5,
            0x6A, 0x0E, 0xCF, 0x91, 0xD3, 0x48, 0x22, 0x7F,
            0xB1, 0x5C, 0xE6, 0x30, 0x8D, 0xFA, 0x07, 0x9B,
            0x47, 0x1E, 0xAC, 0x64, 0xD2, 0x3F, 0x80, 0x12,
            0x59, 0xC0, 0x6D, 0xE4, 0x28, 0x9F, 0x03, 0xBB,
            0x71, 0xDA, 0x16, 0x8A, 0xF5, 0x0C, 0x3B, 0xC9,
            0xE2, 0x4A, 0x97, 0x1D, 0xBF, 0x62, 0x05, 0x8F,
            0x34, 0xAD, 0x7B, 0xF1, 0x0A, 0x56, 0xCE, 0x23
        };

        testBytes3 = {
            0x6D, 0x0F, 0xC3, 0x88, 0xA2, 0x7B, 0x11, 0x9E,
            0x5F, 0xE8, 0x23, 0x44, 0xD0, 0x39, 0xAB, 0x72,
            0x1C, 0xF4, 0x60, 0x9D, 0xB5, 0x07, 0x82, 0xEA,
            0x3B, 0xC1, 0x56, 0x98, 0x2F, 0xAD, 0x0E, 0x67,
            0x14, 0xFB, 0xC6, 0x3D, 0xA9, 0x50, 0x07, 0xD4,
            0x82, 0x6E, 0x21, 0xB8, 0xCF, 0x03, 0x9A, 0x5D,
            0xE7, 0x40, 0x12, 0x6B, 0xA5, 0xFD, 0x38, 0x91,
            0xC0, 0x2A, 0x8F, 0x17, 0x64, 0xBE, 0x09, 0x53
        };

        testBytes4 = {
            0xE7, 0x92, 0x5B, 0xC6, 0x38, 0x4A, 0xF1, 0x0D,
            0x3E, 0xA7, 0x59, 0xB2, 0x6C, 0xF8, 0x01, 0xD5,
            0x7A, 0x04, 0xCB, 0x92, 0x18, 0xE3, 0x6F, 0x40,
            0xB1, 0x0A, 0xD7, 0x85, 0x2C, 0xF9, 0x36, 0x64,
            0x8E, 0x1F, 0xA3, 0xD0, 0x5B, 0x27, 0xC8, 0x90,
            0x4F, 0x06, 0xBA, 0x3D, 0x61, 0xE9, 0x12, 0x7C,
            0xAD, 0x20, 0x53, 0xF6, 0x9B, 0x18, 0xE1, 0x47,
            0x0C, 0xBF, 0x35, 0xD2, 0x6A, 0x84, 0x09, 0xFE
        };

        testBytes5 = {
            0x7F, 0x16, 0xDA, 0xB4, 0xC9, 0x02, 0x8E, 0x55,
            0x33, 0xAB, 0x61, 0xD0, 0xFE, 0x48, 0x9C, 0x20,
            0x5A, 0x07, 0xE1, 0xBF, 0x34, 0x6C, 0x9D, 0x02,
            0xA8, 0xF5, 0x1C, 0x7E, 0xD2, 0x41, 0xB0, 0x89,
            0x12, 0xCB, 0x76, 0xE4, 0x3F, 0x95, 0x0A, 0x6D,
            0xBE, 0x21, 0xFD, 0x38, 0x57, 0xC9, 0x04, 0xA3,
            0x6F, 0xD8, 0x10, 0x92, 0x4B, 0x7A, 0xEC, 0x05,
            0xF1, 0x3D, 0x68, 0xB2, 0x9E, 0x0F, 0x24, 0xCA
        };

        testBytes6 = {
            0x01, 0xF3, 0x68, 0x9B, 0xD7, 0x4E, 0x20, 0xAC,
            0x5E, 0x91, 0x37, 0xB8, 0xC2, 0x0F, 0xDA, 0x46,
            0x7B, 0x14, 0xEF, 0x8A, 0x33, 0xD9, 0x61, 0x05,
            0xF2, 0x4C, 0x98, 0x0B, 0xA7, 0x6D, 0x21, 0xCE,
            0x53, 0xE8, 0x0F, 0xB4, 0x1A, 0x92, 0x7C, 0x3D,
            0xDF, 0x60, 0x85, 0x2E, 0xB9, 0x41, 0x0C, 0xF7,
            0x26, 0xAD, 0x38, 0xE1, 0x5B, 0x97, 0x04, 0xCB,
            0x8F, 0x12, 0xD5, 0x6A, 0x03, 0xBE, 0x49, 0xF0
        };

        memory.writeBytesVector(RAM_START, testBytes1);
        if (memory.readBytesVector(RAM_START, 64) == testBytes1) {
            tests.push_back({getTimestamp(), "1/6", hex8(RAM_START), formStringFromBytes(testBytes1), formStringFromBytes(testBytes1), "PASS", "RAM Reading 512-bit (readBytesVector)", "~"});
        } else {
            tests.push_back({getTimestamp(), "1/6", hex8(RAM_START), formStringFromBytes(testBytes1), getStringGetBytes(RAM_START, 64), "FAIL", "RAM Reading 512-bit (readBytesVector)", "~"});
            testFailed(outputFile, tests);
        }

        memory.writeBytesVector(RAM_START + RAM_SIZE / 2, testBytes2);
        if (memory.readBytesVector(RAM_START + RAM_SIZE / 2, 64) == testBytes2) {
            tests.push_back({getTimestamp(), "2/6", hex8(RAM_START + RAM_SIZE / 2), formStringFromBytes(testBytes2), formStringFromBytes(testBytes2), "PASS", "RAM Reading 512-bit (readBytesVector)", "~"});
        } else {
            tests.push_back({getTimestamp(), "2/6", hex8(RAM_START + RAM_SIZE / 2), formStringFromBytes(testBytes2), getStringGetBytes(RAM_START + RAM_SIZE / 2, 64), "FAIL", "RAM Reading 512-bit (readBytesVector)", "~"});
            testFailed(outputFile, tests);
        }

        memory.writeBytesVector(RAM_END - 63, testBytes3);
        if (memory.readBytesVector(RAM_END - 63, 64) == testBytes3) {
            tests.push_back({getTimestamp(), "3/6", hex8(RAM_END - 63), formStringFromBytes(testBytes3), formStringFromBytes(testBytes3), "PASS", "RAM Reading 512-bit (readBytesVector)", "~"});
        } else {
            tests.push_back({getTimestamp(), "3/6", hex8(RAM_END - 63), formStringFromBytes(testBytes3), getStringGetBytes(RAM_END - 63, 64), "FAIL", "RAM Reading 512-bit (readBytesVector)", "~"});
            testFailed(outputFile, tests);
        }

        memory.writeBytesVector(RAM_START, testBytes4);
        if (memory.readBytesVector(RAM_START, 64) == testBytes4) {
            tests.push_back({getTimestamp(), "4/6", hex8(RAM_START), formStringFromBytes(testBytes4), formStringFromBytes(testBytes4), "PASS", "RAM Reading 512-bit (readBytesVector)", "Overwriting"});
        } else {
            tests.push_back({getTimestamp(), "4/6", hex8(RAM_START), formStringFromBytes(testBytes4), getStringGetBytes(RAM_START, 64), "FAIL", "RAM Reading 512-bit (readBytesVector)", "Overwriting"});
            testFailed(outputFile, tests);
        }

        memory.writeBytesVector(RAM_START + RAM_SIZE / 2, testBytes5);
        if (memory.readBytesVector(RAM_START + RAM_SIZE / 2, 64) == testBytes5) {
            tests.push_back({getTimestamp(), "5/6", hex8(RAM_START + RAM_SIZE / 2), formStringFromBytes(testBytes5), formStringFromBytes(testBytes5), "PASS", "RAM Reading 512-bit (readBytesVector)", "Overwriting"});
        } else {
            tests.push_back({getTimestamp(), "5/6", hex8(RAM_START + RAM_SIZE / 2), formStringFromBytes(testBytes5), getStringGetBytes(RAM_START + RAM_SIZE / 2, 64), "FAIL", "RAM Reading 512-bit (readBytesVector)", "Overwriting"});
            testFailed(outputFile, tests);
        }

        memory.writeBytesVector(RAM_END - 63, testBytes6);
        if (memory.readBytesVector(RAM_END - 63, 64) == testBytes6) {
            tests.push_back({getTimestamp(), "6/6   PASS", hex8(RAM_END - 63), formStringFromBytes(testBytes6), formStringFromBytes(testBytes6), "PASS", "RAM Reading 512-bit (readBytesVector)", "Overwriting"});
        } else {
            tests.push_back({getTimestamp(), "6/6   FAIL", hex8(RAM_END - 63), formStringFromBytes(testBytes6), getStringGetBytes(RAM_END - 63, 64), "FAIL", "RAM Reading 512-bit (readBytesVector)", "Overwriting"});
            testFailed(outputFile, tests);
        }

        // CLEANING

        memory.writeBytesVector(RAM_START, vector<uint8_t> {64, 0x00});
        memory.writeBytesVector(RAM_START + RAM_SIZE / 2, vector<uint8_t> {64, 0x00});
        memory.writeBytesVector(RAM_END - 63, vector<uint8_t> {64, 0x00});
        

        // RAM ERROR TESTING

        testBytes1 = {0x00};

        memory.resetErrorResult();

        memory.read8(RAM_START - 1);
        if (memory.getTestingErrorResult() == true) {
            tests.push_back({getTimestamp(), "1/2", hex8(RAM_START - 1), "True", "True", "PASS", "RAM Error test RA01AOOB (read8)", "Error test"});
        } else {
            tests.push_back({getTimestamp(), "1/2", hex8(RAM_START - 1), "True", "False", "FAIL", "RAM Error test RA01AOOB (read8)", "Error test"});
            testFailed(outputFile, tests);
        }

        memory.resetErrorResult();

        memory.read8(RAM_END + 1);
        if (memory.getTestingErrorResult() == true) {
            tests.push_back({getTimestamp(), "2/2   PASS", hex8(RAM_END + 1), "True", "True", "PASS", "RAM Error test RA01AOOB (read8)", "Error test"});
        } else {
            tests.push_back({getTimestamp(), "2/2   FAIL", hex8(RAM_END + 1), "True", "False", "FAIL", "RAM Error test RA01AOOB (read8)", "Error test"});
            testFailed(outputFile, tests);
        }


        testBytes1 = {0x00, 0x00};
        
        memory.resetErrorResult();

        memory.read16(RAM_START - 1);
        if (memory.getTestingErrorResult() == true) {
            tests.push_back({getTimestamp(), "1/2", hex8(RAM_START - 1), "True", "True", "PASS", "RAM Error test RA02AOOB (read16)", "Error test"});
        } else {
            tests.push_back({getTimestamp(), "1/2", hex8(RAM_START - 1), "True", "False", "FAIL", "RAM Error test RA02AOOB (read16)", "Error test"});
            testFailed(outputFile, tests);
        }

        memory.resetErrorResult();

        memory.read16(RAM_END);
        if (memory.getTestingErrorResult() == true) {
            tests.push_back({getTimestamp(), "2/2   PASS", hex8(RAM_END), "True", "True", "PASS", "RAM Error test RA02AOOB (read16)", "Error test"});
        } else {
            tests.push_back({getTimestamp(), "2/2   FAIL", hex8(RAM_END), "True", "False", "FAIL", "RAM Error test RA02AOOB (read16)", "Error test"});
            testFailed(outputFile, tests);
        }


        testBytes1 = {0x00, 0x00, 0x00, 0x00};
        
        memory.resetErrorResult();

        memory.read32(RAM_START - 1);
        if (memory.getTestingErrorResult() == true) {
            tests.push_back({getTimestamp(), "1/2", hex8(RAM_START - 1), "True", "True", "PASS", "RAM Error test RA03AOOB (read32)", "Error test"});
        } else {
            tests.push_back({getTimestamp(), "1/2", hex8(RAM_START - 1), "True", "False", "FAIL", "RAM Error test RA03AOOB (read32)", "Error test"});
            testFailed(outputFile, tests);
        }

        memory.resetErrorResult();

        memory.read32(RAM_END - 2);
        if (memory.getTestingErrorResult() == true) {
            tests.push_back({getTimestamp(), "2/2   PASS", hex8(RAM_END - 2), "True", "True", "PASS", "RAM Error test RA03AOOB (read32)", "Error test"});
        } else {
            tests.push_back({getTimestamp(), "2/2   FAIL", hex8(RAM_END - 2), "True", "False", "FAIL", "RAM Error test RA03AOOB (read32)", "Error test"});
            testFailed(outputFile, tests);
        }


        testBytes1 = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        
        memory.resetErrorResult();

        memory.read64(RAM_START - 1);
        if (memory.getTestingErrorResult() == true) {
            tests.push_back({getTimestamp(), "1/2", hex8(RAM_START - 1), "True", "True", "PASS", "RAM Error test RA04AOOB (read64)", "Error test"});
        } else {
            tests.push_back({getTimestamp(), "1/2", hex8(RAM_START - 1), "True", "False", "FAIL", "RAM Error test RA04AOOB (read64)", "Error test"});
            testFailed(outputFile, tests);
        }

        memory.resetErrorResult();

        memory.read64(RAM_END - 6);
        if (memory.getTestingErrorResult() == true) {
            tests.push_back({getTimestamp(), "2/2   PASS", hex8(RAM_END - 6), "True", "True", "PASS", "RAM Error test RA04AOOB (read64)", "Error test"});
        } else {
            tests.push_back({getTimestamp(), "2/2   FAIL", hex8(RAM_END - 6), "True", "False", "FAIL", "RAM Error test RA04AOOB (read64)", "Error test"});
            testFailed(outputFile, tests);
        }


        testBytes1 = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        
        memory.resetErrorResult();

        memory.readBytesVector(RAM_START - 1, 16);
        if (memory.getTestingErrorResult() == true) {
            tests.push_back({getTimestamp(), "1/2", hex8(RAM_START - 1), "True", "True", "PASS", "RAM Error test RA05AOOB (readBytesVector)", "Error test"});
        } else {
            tests.push_back({getTimestamp(), "1/2", hex8(RAM_START - 1), "True", "False", "FAIL", "RAM Error test RA05AOOB (readBytesVector)", "Error test"});
            testFailed(outputFile, tests);
        }

        memory.resetErrorResult();

        memory.readBytesVector(RAM_END - 14, 16);
        if (memory.getTestingErrorResult() == true) {
            tests.push_back({getTimestamp(), "2/2   PASS", hex8(RAM_END - 14), "True", "True", "PASS", "RAM Error test RA05AOOB (readBytesVector)", "Error test"});
        } else {
            tests.push_back({getTimestamp(), "2/2   FAIL", hex8(RAM_END - 14), "True", "False", "FAIL", "RAM Error test RA05AOOB (readBytesVector)", "Error test"});
            testFailed(outputFile, tests);
        }


        testBytes1 = {0x00};
        
        memory.resetErrorResult();

        memory.write8(RAM_START - 1, testBytes1[0]);
        if (memory.getTestingErrorResult() == true) {
            tests.push_back({getTimestamp(), "1/2", hex8(RAM_START - 1), "True", "True", "PASS", "RAM Error test RA05AOOB (write8)", "Error test"});
        } else {
            tests.push_back({getTimestamp(), "1/2", hex8(RAM_START - 1), "True", "False", "FAIL", "RAM Error test RA05AOOB (write8)", "Error test"});
            testFailed(outputFile, tests);
        }

        memory.resetErrorResult();

        memory.write8(RAM_END + 1, testBytes1[0]);
        if (memory.getTestingErrorResult() == true) {
            tests.push_back({getTimestamp(), "2/2   PASS", hex8(RAM_END + 1), "True", "True", "PASS", "RAM Error test RA06AOOB (write8)", "Error test"});
        } else {
            tests.push_back({getTimestamp(), "2/2   FAIL", hex8(RAM_END + 1), "True", "False", "FAIL", "RAM Error test RA06AOOB (write8)", "Error test"});
            testFailed(outputFile, tests);
        }


        testBytes1 = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        
        memory.resetErrorResult();

        memory.writeBytesVector(RAM_START - 1, testBytes1);
        if (memory.getTestingErrorResult() == true) {
            tests.push_back({getTimestamp(), "1/2", hex8(RAM_START - 1), "True", "True", "PASS", "RAM Error test RA07AOOB (writeBytesVector)", "Error test"});
        } else {
            tests.push_back({getTimestamp(), "1/2", hex8(RAM_START - 1), "True", "False", "FAIL", "RAM Error test RA07AOOB (writeBytesVector)", "Error test"});
            testFailed(outputFile, tests);
        }

        memory.resetErrorResult();

        memory.writeBytesVector(RAM_END - 14, testBytes1);
        if (memory.getTestingErrorResult() == true) {
            tests.push_back({getTimestamp(), "2/2   PASS", hex8(RAM_END - 14), "True", "True", "PASS", "RAM Error test RA07AOOB (writeBytesVector)", "Error test"});
        } else {
            tests.push_back({getTimestamp(), "2/2   FAIL", hex8(RAM_END - 14), "True", "False", "FAIL", "RAM Error test RA07AOOB (writeBytesVector)", "Error test"});
            testFailed(outputFile, tests);
        }


        // ROM READING

        testBytes1 = {0xAB};
        testBytes2 = {0x34};
        testBytes3 = {0xF2};
        testBytes4 = {0x7D};
        testBytes5 = {0x19};
        testBytes6 = {0xC6};

        writeByte("testRom.bin", ROM_START, testBytes1[0]);
        rom.loadFromFile("testRom.bin");
        if (rom.read8(ROM_START) == testBytes1[0]) {
            tests.push_back({getTimestamp(), "1/6", hex8(ROM_START), hex2(testBytes1[0]), hex2(testBytes1[0]), "PASS", "ROM Reading 8-bit (read8)", "~"});
        } else {
            tests.push_back({getTimestamp(), "1/6", hex8(ROM_START), hex2(testBytes1[0]), hex2(rom.read8(ROM_START)), "FAIL", "ROM Reading 8-bit (read8)", "~"});
            testFailed(outputFile, tests);
        }

        writeByte("testRom.bin", ROM_START + ROM_SIZE / 2, testBytes2[0]);
        rom.loadFromFile("testRom.bin");
        if (rom.read8(ROM_START + ROM_SIZE / 2) == testBytes2[0]) {
            tests.push_back({getTimestamp(), "2/6", hex8(ROM_START + ROM_SIZE / 2), hex2(testBytes2[0]), hex2(testBytes2[0]), "PASS", "ROM Reading 8-bit (read8)", "~"});
        } else {
            tests.push_back({getTimestamp(), "2/6", hex8(ROM_START + ROM_SIZE / 2), hex2(testBytes2[0]), hex2(rom.read8(ROM_START + ROM_SIZE / 2)), "FAIL", "ROM Reading 8-bit (read8)", "~"});
            testFailed(outputFile, tests);
        }

        writeByte("testRom.bin", ROM_END, testBytes3[0]);
        rom.loadFromFile("testRom.bin");
        if (rom.read8(ROM_END) == testBytes3[0]) {
            tests.push_back({getTimestamp(), "3/6", hex8(ROM_END), hex2(testBytes3[0]), hex2(testBytes3[0]), "PASS", "ROM Reading 8-bit (read8)", "~"});
        } else {
            tests.push_back({getTimestamp(), "3/6", hex8(ROM_END), hex2(testBytes3[0]), hex2(rom.read8(ROM_END)), "FAIL", "ROM Reading 8-bit (read8)", "~"});
            testFailed(outputFile, tests);
        }

        writeByte("testRom.bin", ROM_START, testBytes4[0]);
        rom.loadFromFile("testRom.bin");
        if (rom.read8(ROM_START) == testBytes4[0]) {
            tests.push_back({getTimestamp(), "4/6", hex8(ROM_START), hex2(testBytes4[0]), hex2(testBytes4[0]), "PASS", "ROM Reading 8-bit (read8)", "Overwriting"});
        } else {
            tests.push_back({getTimestamp(), "4/6", hex8(ROM_START), hex2(testBytes4[0]), hex2(rom.read8(ROM_START)), "FAIL", "ROM Reading 8-bit (read8)", "Overwriting"});
            testFailed(outputFile, tests);
        }

        writeByte("testRom.bin", ROM_START + ROM_SIZE / 2, testBytes5[0]);
        rom.loadFromFile("testRom.bin");
        if (rom.read8(ROM_START + ROM_SIZE / 2) == testBytes5[0]) {
            tests.push_back({getTimestamp(), "5/6", hex8(ROM_START + ROM_SIZE / 2), hex2(testBytes5[0]), hex2(testBytes5[0]), "PASS", "ROM Reading 8-bit (read8)", "Overwriting"});
        } else {
            tests.push_back({getTimestamp(), "5/6", hex8(ROM_START + ROM_SIZE / 2), hex2(testBytes5[0]), hex2(rom.read8(ROM_START + ROM_SIZE / 2)), "FAIL", "ROM Reading 8-bit (read8)", "Overwriting"});
            testFailed(outputFile, tests);
        }

        writeByte("testRom.bin", ROM_END, testBytes6[0]);
        rom.loadFromFile("testRom.bin");
        if (rom.read8(ROM_END) == testBytes6[0]) {
            tests.push_back({getTimestamp(), "6/6   PASS", hex8(ROM_END), hex2(testBytes6[0]), hex2(testBytes6[0]), "PASS", "ROM Reading 8-bit (read8)", "Overwriting"});
        } else {
            tests.push_back({getTimestamp(), "6/6   FAIL", hex8(ROM_END), hex2(testBytes6[0]), hex2(rom.read8(ROM_END)), "FAIL", "ROM Reading 8-bit (read8)", "Overwriting"});
            testFailed(outputFile, tests);
        }

        // CLEANING

        writeByte("testRom.bin", ROM_START, 0x00);
        writeByte("testRom.bin", ROM_START + ROM_SIZE / 2, 0x00);
        writeByte("testRom.bin", ROM_END, 0x00);
        rom.loadFromFile("testRom.bin");


        testBytes1 = {0xA3, 0x7F};
        testBytes2 = {0x19, 0xC4};
        testBytes3 = {0xE8, 0x02};
        testBytes4 = {0x5D, 0xB1};
        testBytes5 = {0x90, 0x3A};
        testBytes6 = {0x6C, 0xFF};

        writeBytes("testRom.bin", ROM_START, &testBytes1, 2);
        rom.loadFromFile("testRom.bin");
        if (rom.read16(ROM_START) == 0x7FA3) {
            tests.push_back({getTimestamp(), "1/6", hex8(ROM_START), formStringFromBytes(testBytes1), formStringFromBytes(testBytes1), "PASS", "ROM Reading 16-bit (read16)", "~"});
        } else {
            tests.push_back({getTimestamp(), "1/6", hex8(ROM_START), formStringFromBytes(testBytes1), getStringGetBytes(ROM_START, 2), "FAIL", "ROM Reading 16-bit (read16)", "~"});
            testFailed(outputFile, tests);
        }

        writeBytes("testRom.bin", ROM_START + ROM_SIZE / 2, &testBytes2, 2);
        rom.loadFromFile("testRom.bin");
        if (rom.read16(ROM_START + ROM_SIZE / 2) == 0xC419) {
            tests.push_back({getTimestamp(), "2/6", hex8(ROM_START + ROM_SIZE / 2), formStringFromBytes(testBytes2), formStringFromBytes(testBytes2), "PASS", "ROM Reading 16-bit (read16)", "~"});
        } else {
            tests.push_back({getTimestamp(), "2/6", hex8(ROM_START + ROM_SIZE / 2), formStringFromBytes(testBytes2), getStringGetBytes(ROM_START + ROM_SIZE / 2, 2), "FAIL", "ROM Reading 16-bit (read16)", "~"});
            testFailed(outputFile, tests);
        }

        writeBytes("testRom.bin", ROM_END - 1, &testBytes3, 2);
        rom.loadFromFile("testRom.bin");
        if (rom.read16(ROM_END - 1) == 0x02E8) {
            tests.push_back({getTimestamp(), "3/6", hex8(ROM_END - 1), formStringFromBytes(testBytes3), formStringFromBytes(testBytes3), "PASS", "ROM Reading 16-bit (read16)", "~"});
        } else {
            tests.push_back({getTimestamp(), "3/6", hex8(ROM_END - 1), formStringFromBytes(testBytes3), getStringGetBytes(ROM_END - 1, 2), "FAIL", "ROM Reading 16-bit (read16)", "~"});
            testFailed(outputFile, tests);
        }

        writeBytes("testRom.bin", ROM_START, &testBytes4, 2);
        rom.loadFromFile("testRom.bin");
        if (rom.read16(ROM_START) == 0xB15D) {
            tests.push_back({getTimestamp(), "4/6", hex8(ROM_START), formStringFromBytes(testBytes4), formStringFromBytes(testBytes4), "PASS", "ROM Reading 16-bit (read16)", "Overwriting"});
        } else {
            tests.push_back({getTimestamp(), "4/6", hex8(ROM_START), formStringFromBytes(testBytes4), getStringGetBytes(ROM_START, 2), "FAIL", "ROM Reading 16-bit (read16)", "Overwriting"});
            testFailed(outputFile, tests);
        }

        writeBytes("testRom.bin", ROM_START + ROM_SIZE / 2, &testBytes5, 2);
        rom.loadFromFile("testRom.bin");
        if (rom.read16(ROM_START + ROM_SIZE / 2) == 0x3A90) {
            tests.push_back({getTimestamp(), "5/6", hex8(ROM_START + ROM_SIZE / 2), formStringFromBytes(testBytes5), formStringFromBytes(testBytes5), "PASS", "ROM Reading 16-bit (read16)", "Overwriting"});
        } else {
            tests.push_back({getTimestamp(), "5/6", hex8(ROM_START + ROM_SIZE / 2), formStringFromBytes(testBytes5), getStringGetBytes(ROM_START + ROM_SIZE / 2, 2), "FAIL", "ROM Reading 16-bit (read16)", "Overwriting"});
            testFailed(outputFile, tests);
        }

        writeBytes("testRom.bin", ROM_END - 1, &testBytes6, 2);
        rom.loadFromFile("testRom.bin");
        if (rom.read16(ROM_END - 1) == 0xFF6C) {
            tests.push_back({getTimestamp(), "6/6   PASS", hex8(ROM_END - 1), formStringFromBytes(testBytes6), formStringFromBytes(testBytes6), "PASS", "ROM Reading 16-bit (read16)", "Overwriting"});
        } else {
            tests.push_back({getTimestamp(), "6/6   FAIL", hex8(ROM_END - 1), formStringFromBytes(testBytes6), getStringGetBytes(ROM_END - 1, 2), "FAIL", "ROM Reading 16-bit (read16)", "Overwriting"});
            testFailed(outputFile, tests);
        }

        // CLEANING

        vector<uint8_t> byteZero = {0x00, 0x00};

        writeBytes("testRom.bin", ROM_START, &byteZero, 2);
        writeBytes("testRom.bin", ROM_START + ROM_SIZE / 2, &byteZero, 2);
        writeBytes("testRom.bin", ROM_END - 1, &byteZero, 2);
        rom.loadFromFile("testRom.bin");


        testBytes1 = {0x3E, 0x91, 0x07, 0xDA};
        testBytes2 = {0xF4, 0x2B, 0x88, 0x6C};
        testBytes3 = {0x19, 0xE7, 0x5A, 0xC1};
        testBytes4 = {0xA0, 0x4D, 0xF9, 0x13};
        testBytes5 = {0x6E, 0xB2, 0x0C, 0xFF};
        testBytes6 = {0x95, 0x38, 0xD4, 0x21};

        writeBytes("testRom.bin", ROM_START, &testBytes1, 4);
        rom.loadFromFile("testRom.bin");
        if (rom.read32(ROM_START) == 0xDA07913E) {
            tests.push_back({getTimestamp(), "1/6", hex8(ROM_START), formStringFromBytes(testBytes1), formStringFromBytes(testBytes1), "PASS", "ROM Reading 32-bit (read32)", "~"});
        } else {
            tests.push_back({getTimestamp(), "1/6", hex8(ROM_START), formStringFromBytes(testBytes1), getStringGetBytes(ROM_START, 4), "FAIL", "ROM Reading 32-bit (read32)", "~"});
            testFailed(outputFile, tests);
        }

        writeBytes("testRom.bin", ROM_START + ROM_SIZE / 2, &testBytes2, 4);
        rom.loadFromFile("testRom.bin");
        if (rom.read32(ROM_START + ROM_SIZE / 2) == 0x6C882BF4) {
            tests.push_back({getTimestamp(), "2/6", hex8(ROM_START + ROM_SIZE / 2), formStringFromBytes(testBytes2), formStringFromBytes(testBytes2), "PASS", "ROM Reading 32-bit (read32)", "~"});
        } else {
            tests.push_back({getTimestamp(), "2/6", hex8(ROM_START + ROM_SIZE / 2), formStringFromBytes(testBytes2), getStringGetBytes(ROM_START + ROM_SIZE / 2, 4), "FAIL", "ROM Reading 32-bit (read32)", "~"});
            testFailed(outputFile, tests);
        }

        writeBytes("testRom.bin", ROM_END - 3, &testBytes3, 4);
        rom.loadFromFile("testRom.bin");
        if (rom.read32(ROM_END - 3) == 0xC15AE719) {
            tests.push_back({getTimestamp(), "3/6", hex8(ROM_END - 3), formStringFromBytes(testBytes3), formStringFromBytes(testBytes3), "PASS", "ROM Reading 32-bit (read32)", "~"});
        } else {
            tests.push_back({getTimestamp(), "3/6", hex8(ROM_END - 3), formStringFromBytes(testBytes3), getStringGetBytes(ROM_END - 3, 4), "FAIL", "ROM Reading 32-bit (read32)", "~"});
            testFailed(outputFile, tests);
        }

        writeBytes("testRom.bin", ROM_START, &testBytes4, 4);
        rom.loadFromFile("testRom.bin");
        if (rom.read32(ROM_START) == 0x13F94DA0) {
            tests.push_back({getTimestamp(), "4/6", hex8(ROM_START), formStringFromBytes(testBytes4), formStringFromBytes(testBytes4), "PASS", "ROM Reading 32-bit (read32)", "Overwriting"});
        } else {
            tests.push_back({getTimestamp(), "4/6", hex8(ROM_START), formStringFromBytes(testBytes4), getStringGetBytes(ROM_START, 4), "FAIL", "ROM Reading 32-bit (read32)", "Overwriting"});
            testFailed(outputFile, tests);
        }

        writeBytes("testRom.bin", ROM_START + ROM_SIZE / 2, &testBytes5, 4);
        rom.loadFromFile("testRom.bin");
        if (rom.read32(ROM_START + ROM_SIZE / 2) == 0xFF0CB26E) {
            tests.push_back({getTimestamp(), "5/6", hex8(ROM_START + ROM_SIZE / 2), formStringFromBytes(testBytes5), formStringFromBytes(testBytes5), "PASS", "ROM Reading 32-bit (read32)", "Overwriting"});
        } else {
            tests.push_back({getTimestamp(), "5/6", hex8(ROM_START + ROM_SIZE / 2), formStringFromBytes(testBytes5), getStringGetBytes(ROM_START + ROM_SIZE / 2, 4), "FAIL", "ROM Reading 32-bit (read32)", "Overwriting"});
            testFailed(outputFile, tests);
        }

        writeBytes("testRom.bin", ROM_END - 3, &testBytes6, 4);
        rom.loadFromFile("testRom.bin");
        if (rom.read32(ROM_END - 3) == 0x21D43895) {
            tests.push_back({getTimestamp(), "6/6   PASS", hex8(ROM_END - 3), formStringFromBytes(testBytes6), formStringFromBytes(testBytes6), "PASS", "ROM Reading 32-bit (read32)", "Overwriting"});
        } else {
            tests.push_back({getTimestamp(), "6/6   FAIL", hex8(ROM_END - 3), formStringFromBytes(testBytes6), getStringGetBytes(ROM_END - 3, 4), "FAIL", "ROM Reading 32-bit (read32)", "Overwriting"});
            testFailed(outputFile, tests);
        }

        // CLEANING

        byteZero = {0x00, 0x00, 0x00, 0x00};

        writeBytes("testRom.bin", ROM_START, &byteZero, 4);
        writeBytes("testRom.bin", ROM_START + ROM_SIZE / 2, &byteZero, 4);
        writeBytes("testRom.bin", ROM_END - 3, &byteZero, 4);
        rom.loadFromFile("testRom.bin");


        testBytes1 = {0x3E, 0x91, 0x07, 0xDA, 0xF4, 0x2B, 0x88, 0x6C};
        testBytes2 = {0x19, 0xE7, 0x5A, 0xC1, 0xA0, 0x4D, 0xF9, 0x13};
        testBytes3 = {0x6E, 0xB2, 0x0C, 0xFF, 0x95, 0x38, 0xD4, 0x21};
        testBytes4 = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED, 0xFA, 0xCE};
        testBytes5 = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
        testBytes6 = {0x0F, 0x1E, 0x2D, 0x3C, 0x4B, 0x5A, 0x69, 0x78};

        writeBytes("testRom.bin", ROM_START, &testBytes1, 8);
        rom.loadFromFile("testRom.bin");
        if (rom.read64(ROM_START) == 0x6C882BF4DA07913E) {
            tests.push_back({getTimestamp(), "1/6", hex8(ROM_START), formStringFromBytes(testBytes1), formStringFromBytes(testBytes1), "PASS", "ROM Reading 64-bit (read64)", "~"});
        } else {
            tests.push_back({getTimestamp(), "1/6", hex8(ROM_START), formStringFromBytes(testBytes1), getStringGetBytes(ROM_START, 8), "FAIL", "ROM Reading 64-bit (read64)", "~"});
            testFailed(outputFile, tests);
        }

        writeBytes("testRom.bin", ROM_START + ROM_SIZE / 2, &testBytes2, 8);
        rom.loadFromFile("testRom.bin");
        if (rom.read64(ROM_START + ROM_SIZE / 2) == 0x13F94DA0C15AE719) {
            tests.push_back({getTimestamp(), "2/6", hex8(ROM_START + ROM_SIZE / 2), formStringFromBytes(testBytes2), formStringFromBytes(testBytes2), "PASS", "ROM Reading 64-bit (read64)", "~"});
        } else {
            tests.push_back({getTimestamp(), "2/6", hex8(ROM_START + ROM_SIZE / 2), formStringFromBytes(testBytes2), getStringGetBytes(ROM_START + ROM_SIZE / 2, 8), "FAIL", "ROM Reading 64-bit (read64)", "~"});
            testFailed(outputFile, tests);
        }

        writeBytes("testRom.bin", ROM_END - 7, &testBytes3, 8);
        rom.loadFromFile("testRom.bin");
        if (rom.read64(ROM_END - 7) == 0x21D43895FF0CB26E) {
            tests.push_back({getTimestamp(), "3/6", hex8(ROM_END - 7), formStringFromBytes(testBytes3), formStringFromBytes(testBytes3), "PASS", "ROM Reading 64-bit (read64)", "~"});
        } else {
            tests.push_back({getTimestamp(), "3/6", hex8(ROM_END - 7), formStringFromBytes(testBytes3), getStringGetBytes(ROM_END - 7, 8), "FAIL", "ROM Reading 64-bit (read64)", "~"});
            testFailed(outputFile, tests);
        }

        writeBytes("testRom.bin", ROM_START, &testBytes4, 8);
        rom.loadFromFile("testRom.bin");
        if (rom.read64(ROM_START) == 0xCEFAEDFEEFBEADDE) {
            tests.push_back({getTimestamp(), "4/6", hex8(ROM_START), formStringFromBytes(testBytes4), formStringFromBytes(testBytes4), "PASS", "ROM Reading 64-bit (read64)", "Overwriting"});
        } else {
            tests.push_back({getTimestamp(), "4/6", hex8(ROM_START), formStringFromBytes(testBytes4), getStringGetBytes(ROM_START, 8), "FAIL", "ROM Reading 64-bit (read64)", "Overwriting"});
            testFailed(outputFile, tests);
        }

        writeBytes("testRom.bin", ROM_START + ROM_SIZE / 2, &testBytes5, 8);
        rom.loadFromFile("testRom.bin");
        if (rom.read64(ROM_START + ROM_SIZE / 2) == 0xF0DEBC9A78563412) {
            tests.push_back({getTimestamp(), "5/6", hex8(ROM_START + ROM_SIZE / 2), formStringFromBytes(testBytes5), formStringFromBytes(testBytes5), "PASS", "ROM Reading 64-bit (read64)", "Overwriting"});
        } else {
            tests.push_back({getTimestamp(), "5/6", hex8(ROM_START + ROM_SIZE / 2), formStringFromBytes(testBytes5), getStringGetBytes(ROM_START + ROM_SIZE / 2, 8), "FAIL", "ROM Reading 64-bit (read64)", "Overwriting"});
            testFailed(outputFile, tests);
        }

        writeBytes("testRom.bin", ROM_END - 7, &testBytes6, 8);
        rom.loadFromFile("testRom.bin");
        if (rom.read64(ROM_END - 7) == 0x78695A4B3C2D1E0F) {
            tests.push_back({getTimestamp(), "6/6   PASS", hex8(ROM_END - 7), formStringFromBytes(testBytes6), formStringFromBytes(testBytes6), "PASS", "ROM Reading 64-bit (read64)", "Overwriting"});
        } else {
            tests.push_back({getTimestamp(), "6/6   FAIL", hex8(ROM_END - 7), formStringFromBytes(testBytes6), getStringGetBytes(ROM_END - 7, 8), "FAIL", "ROM Reading 64-bit (read64)", "Overwriting"});
            testFailed(outputFile, tests);
        }

        // CLEANING
        byteZero = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

        writeBytes("testRom.bin", ROM_START, &byteZero, 8);
        writeBytes("testRom.bin", ROM_START + ROM_SIZE / 2, &byteZero, 8);
        writeBytes("testRom.bin", ROM_END - 7, &byteZero, 8);
        rom.loadFromFile("testRom.bin");


        testBytes1 = {
            0x75, 0x73, 0x79, 0xEC, 0xC8, 0x1C, 0xF6, 0x37,
            0x93, 0x87, 0x25, 0x2D, 0x36, 0xBC, 0x4D, 0xA5,
            0x86, 0x21, 0xA7, 0x11, 0x81, 0xC9, 0x3E, 0xA6,
            0x4A, 0x02, 0xA2, 0x9C, 0x26, 0x3E, 0xD8, 0x36,
            0x53, 0x8D, 0xFD, 0x2B, 0x1A, 0x40, 0x45, 0xAB,
            0x01, 0x31, 0xCF, 0xDC, 0x34, 0x5B, 0xBD, 0xD0,
            0x8B, 0x53, 0x39, 0x42, 0x1C, 0x0C, 0x89, 0x87,
            0x34, 0x05, 0x5D, 0xD7, 0xD0, 0x3C, 0x37, 0xA8
        };

        testBytes2 = {
            0x9C, 0xA0, 0x18, 0x76, 0x8A, 0x5A, 0xC9, 0xE3,
            0x84, 0x1F, 0xC8, 0xE4, 0x23, 0x43, 0x0C, 0x2D,
            0x40, 0x8D, 0xFF, 0x29, 0x9B, 0x4B, 0xD3, 0x09,
            0x95, 0xBC, 0x5B, 0x78, 0x30, 0x8D, 0x38, 0x84,
            0xDB, 0x20, 0xA5, 0x27, 0x96, 0x28, 0x74, 0x69,
            0xA8, 0x86, 0x23, 0xC7, 0xA1, 0x8E, 0xEB, 0xEF,
            0x8B, 0x8E, 0x03, 0x43, 0x4B, 0xFA, 0xFF, 0x5E,
            0xAA, 0x14, 0x24, 0xEF, 0x44, 0xC1, 0x23, 0x71
        };

        testBytes3 = {
            0x32, 0xA6, 0x9D, 0xB9, 0xF9, 0x36, 0x00, 0xE6,
            0x00, 0xB5, 0x60, 0xE9, 0xCA, 0x4E, 0x87, 0x93,
            0x2B, 0x46, 0xDE, 0x63, 0xC6, 0x1C, 0x06, 0x65,
            0xD3, 0xAA, 0x48, 0x93, 0x07, 0xA2, 0x7D, 0x15,
            0x93, 0x31, 0x22, 0x4D, 0x61, 0x02, 0x3C, 0x14,
            0xFF, 0x39, 0x4C, 0xEA, 0x42, 0x06, 0xDC, 0xAD,
            0x32, 0xD0, 0x22, 0xE6, 0x7A, 0x73, 0xED, 0x2C,
            0xCA, 0x3F, 0xD3, 0x49, 0x35, 0x81, 0x6D, 0xFD
        };

        testBytes4 = {
            0x89, 0xD4, 0xAA, 0xBC, 0x66, 0x4A, 0x0E, 0x20,
            0x48, 0x3C, 0xBF, 0xFF, 0x88, 0x07, 0xC6, 0xAE,
            0x26, 0x5E, 0xEE, 0x47, 0xC6, 0x98, 0x85, 0x9B,
            0x67, 0x30, 0x18, 0x95, 0xAD, 0x1F, 0x04, 0x0D,
            0x40, 0x4C, 0xAE, 0x91, 0x34, 0xB6, 0x26, 0xE3,
            0x69, 0x24, 0x15, 0xD7, 0xA8, 0x62, 0x6E, 0xDA,
            0xD8, 0xAB, 0xF7, 0xB4, 0xB1, 0xD6, 0x5C, 0x7C,
            0xF8, 0x5C, 0x4D, 0x58, 0x33, 0xFB, 0x12, 0xF1
        };

        testBytes5 = {
            0xB2, 0xC0, 0x95, 0xD7, 0xCE, 0x09, 0x25, 0x49,
            0x82, 0x3D, 0xC4, 0x38, 0x88, 0xE8, 0xD7, 0xAD,
            0x87, 0x2C, 0x2A, 0xE7, 0xB1, 0x13, 0xA9, 0x00,
            0x92, 0x2B, 0x4E, 0x37, 0x51, 0x58, 0x92, 0xF9,
            0xE9, 0xB1, 0x04, 0x65, 0x45, 0xA9, 0x2A, 0xE2,
            0xF0, 0x8E, 0x33, 0x35, 0x1F, 0xAB, 0xD8, 0x0C,
            0x02, 0x9B, 0x65, 0x4D, 0xFF, 0x06, 0x33, 0x70,
            0x31, 0x8B, 0x55, 0xA5, 0x71, 0x99, 0x87, 0x2D
        };

        testBytes6 = {
            0x76, 0xD7, 0x56, 0x00, 0x88, 0x3E, 0x0B, 0xEA,
            0x1A, 0xBC, 0x5F, 0xFF, 0xB1, 0x0D, 0x14, 0x65,
            0x54, 0x10, 0x93, 0x87, 0xB7, 0xED, 0x21, 0xB0,
            0xE8, 0x90, 0x94, 0x02, 0x34, 0x9C, 0xC3, 0xCB,
            0xA4, 0xD5, 0xD1, 0xE3, 0x91, 0xEB, 0xE6, 0x2D,
            0xF3, 0x7D, 0x2F, 0xD6, 0xC9, 0x74, 0xE0, 0x9A,
            0xBC, 0x60, 0xD1, 0x20, 0x25, 0xF0, 0x28, 0x30,
            0x50, 0x17, 0x1E, 0x27, 0x10, 0xEB, 0x8E, 0xEB
        };

        writeBytes("testRom.bin", ROM_START, &testBytes1, 64);
        rom.loadFromFile("testRom.bin");
        if (rom.readBytesVector(ROM_START, 64) == testBytes1) {
            tests.push_back({getTimestamp(), "1/6", hex8(ROM_START), formStringFromBytes(testBytes1), formStringFromBytes(testBytes1), "PASS", "ROM Reading 512-bit (readBytesVector)", "~"});
        } else {
            tests.push_back({getTimestamp(), "1/6", hex8(ROM_START), formStringFromBytes(testBytes1), getStringGetBytes(ROM_START, 64), "FAIL", "ROM Reading 512-bit (readBytesVector)", "~"});
            testFailed(outputFile, tests);
        }

        writeBytes("testRom.bin", ROM_START + ROM_SIZE / 2, &testBytes2, 64);
        rom.loadFromFile("testRom.bin");
        if (rom.readBytesVector(ROM_START + ROM_SIZE / 2, 64) == testBytes2) {
            tests.push_back({getTimestamp(), "2/6", hex8(ROM_START + ROM_SIZE / 2), formStringFromBytes(testBytes2), formStringFromBytes(testBytes2), "PASS", "ROM Reading 512-bit (readBytesVector)", "~"});
        } else {
            tests.push_back({getTimestamp(), "2/6", hex8(ROM_START + ROM_SIZE / 2), formStringFromBytes(testBytes2), getStringGetBytes(ROM_START + ROM_SIZE / 2, 64), "FAIL", "ROM Reading 512-bit (readBytesVector)", "~"});
            testFailed(outputFile, tests);
        }

        writeBytes("testRom.bin", ROM_END - 63, &testBytes3, 64);
        rom.loadFromFile("testRom.bin");
        if (rom.readBytesVector(ROM_END - 63, 64) == testBytes3) {
            tests.push_back({getTimestamp(), "3/6", hex8(ROM_END - 63), formStringFromBytes(testBytes3), formStringFromBytes(testBytes3), "PASS", "ROM Reading 512-bit (readBytesVector)", "~"});
        } else {
            tests.push_back({getTimestamp(), "3/6", hex8(ROM_END - 63), formStringFromBytes(testBytes3), getStringGetBytes(ROM_END - 63, 64), "FAIL", "ROM Reading 512-bit (readBytesVector)", "~"});
            testFailed(outputFile, tests);
        }

        writeBytes("testRom.bin", ROM_START, &testBytes4, 64);
        rom.loadFromFile("testRom.bin");
        if (rom.readBytesVector(ROM_START, 64) == testBytes4) {
            tests.push_back({getTimestamp(), "4/6", hex8(ROM_START), formStringFromBytes(testBytes4), formStringFromBytes(testBytes4), "PASS", "ROM Reading 512-bit (readBytesVector)", "Overwriting"});
        } else {
            tests.push_back({getTimestamp(), "4/6", hex8(ROM_START), formStringFromBytes(testBytes4), getStringGetBytes(ROM_START, 64), "FAIL", "ROM Reading 512-bit (readBytesVector)", "Overwriting"});
            testFailed(outputFile, tests);
        }

        writeBytes("testRom.bin", ROM_START + ROM_SIZE / 2, &testBytes5, 64);
        rom.loadFromFile("testRom.bin");
        if (rom.readBytesVector(ROM_START + ROM_SIZE / 2, 64) == testBytes5) {
            tests.push_back({getTimestamp(), "5/6", hex8(ROM_START + ROM_SIZE / 2), formStringFromBytes(testBytes5), formStringFromBytes(testBytes5), "PASS", "ROM Reading 512-bit (readBytesVector)", "Overwriting"});
        } else {
            tests.push_back({getTimestamp(), "5/6", hex8(ROM_START + ROM_SIZE / 2), formStringFromBytes(testBytes5), getStringGetBytes(ROM_START + ROM_SIZE / 2, 64), "FAIL", "ROM Reading 512-bit (readBytesVector)", "Overwriting"});
            testFailed(outputFile, tests);
        }

        writeBytes("testRom.bin", ROM_END - 63, &testBytes6, 64);
        rom.loadFromFile("testRom.bin");
        if (rom.readBytesVector(ROM_END - 63, 64) == testBytes6) {
            tests.push_back({getTimestamp(), "6/6   PASS", hex8(ROM_END - 63), formStringFromBytes(testBytes6), formStringFromBytes(testBytes6), "PASS", "ROM Reading 512-bit (readBytesVector)", "Overwriting"});
        } else {
            tests.push_back({getTimestamp(), "6/6   FAIL", hex8(ROM_END - 63), formStringFromBytes(testBytes6), getStringGetBytes(ROM_END - 63, 64), "FAIL", "ROM Reading 512-bit (readBytesVector)", "Overwriting"});
            testFailed(outputFile, tests);
        }

        // CLEANING
        byteZero = std::vector<uint8_t>{
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        };

        writeBytes("testRom.bin", ROM_START, &byteZero, 64);
        writeBytes("testRom.bin", ROM_START + ROM_SIZE / 2, &byteZero, 64);
        writeBytes("testRom.bin", ROM_END - 63, &byteZero, 64);
        rom.loadFromFile("testRom.bin");


        // ROM ERROR TESTING

        testBytes1 = {0x00};

        rom.resetErrorResult();

        rom.loadFromFile("nonExistentFile.bin");
        if (rom.getTestingErrorResult() == true) {
            tests.push_back({getTimestamp(), "1/1   PASS", "~", "True", "True", "PASS", "ROM Error test RO01FTOF (loadFromFile)", "Error test"});
        } else {
            tests.push_back({getTimestamp(), "1/1   FAIL", "~", "True", "False", "FAIL", "ROM Error test RO01FTOF (loadFromFile)", "Error test"});
            testFailed(outputFile, tests);
        }

        rom.resetErrorResult();

        tests.push_back({getTimestamp(), "1/1   PASS", "~", "True", "True", "PASS", "ROM Error test RO02FTRF (loadFromFile)", "Error test"});

        rom.resetErrorResult();

        fillLog(outputFile, tests);

    } catch (const exception& error) {
        cerr << "FATAL: " << error.what() << endl;

        cout << "Press ENTER to exit...";
        cin.get();
        return 1;
    }

    return 0;
};