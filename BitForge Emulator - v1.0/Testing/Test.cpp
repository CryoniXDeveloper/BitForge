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
        retString += memory.memory[address + x];
        
        if (x < length - 1)
        retString += " ";
    }
    
    return retString;
}

void fillLog(ofstream& outputFile, const vector<TestDetails>& tests) {
    outputFile << "|" << left << setw(15) << "---------------"
        << "|" << setw(12) << "------------"
        << "|" << setw(20) << "--------------------"
        << "|" << setw(21) << "---------------------"
        << "|" << setw(21) << "---------------------"
        << "|" << setw(10) << "----------"
        << "|" << setw(45) << "---------------------------------------------"
        << "|" << setw(15) << "---------------"
        << "|\n";

    outputFile << "|" << left << setw(15) << "  Time"
        << "|" << setw(12) << "  Test"
        << "|" << setw(20) << "  Address"
        << "|" << setw(21) << "  Expected"
        << "|" << setw(21) << "  Actual"
        << "|" << setw(10) << "  Result"
        << "|" << setw(45) << "  Testing"
        << "|" << setw(15) << "  Note"
        << "|\n";

    outputFile << "|" << left << setw(15) << "---------------"
        << "|" << setw(12) << "------------"
        << "|" << setw(20) << "--------------------"
        << "|" << setw(21) << "---------------------"
        << "|" << setw(21) << "---------------------"
        << "|" << setw(10) << "----------"
        << "|" << setw(45) << "---------------------------------------------"
        << "|" << setw(15) << "---------------"
        << "|\n";

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

    }

    outputFile << "|" << left << setw(15) << "---------------"
        << "|" << setw(12) << "------------"
        << "|" << setw(20) << "--------------------"
        << "|" << setw(21) << "---------------------"
        << "|" << setw(21) << "---------------------"
        << "|" << setw(10) << "----------"
        << "|" << setw(45) << "---------------------------------------------"
        << "|" << setw(15) << "---------------"
        << "|\n";
}

void testFailed(ofstream& outputFile, const vector<TestDetails>& tests) {
    fillLog(outputFile, tests);
    exit(0);
}

int main() {
    ofstream outputFile("Test.txt");

    vector<TestDetails> tests = {};

    if (!outputFile.is_open()) {
        cerr << "ERROR: Could not open Test.txt" << endl;
        return 0;
    };

    memory.setTesting(true);

    int RAMTestAddressStart = 0;
    int RAMTestAddressMiddle = Motherboard::RAM_SIZE / 2;
    int RAMTestAddressEnd = Motherboard::RAM_END - Motherboard::RAM_START;

    // RAM WRITING

    memory.write8(RAMTestAddressStart, 0x93);
    if (memory.memory[RAMTestAddressStart] == 0x93) {
        tests.push_back({getTimestamp(), "1/6", hex8(Motherboard::RAM_START), "0x93", "0x93", "PASS", "RAM Writing 8-bit (write8)", "~"});
    } else {
        tests.push_back({getTimestamp(), "1/6", hex8(Motherboard::RAM_START), "0x93", hex2(memory.memory[RAMTestAddressStart]), "FAIL", "RAM Writing 8-bit (write8)", "~"});
        testFailed(outputFile, tests);
    }

    memory.write8(RAMTestAddressMiddle, 0x1B);
    if (memory.memory[RAMTestAddressMiddle] == 0x1B) {
        tests.push_back({getTimestamp(), "2/6", hex8(RAMTestAddressMiddle + Motherboard::RAM_START), "0x1B", "0x1B", "PASS", "RAM Writing 8-bit (write8)", "~"});
    } else {
        tests.push_back({getTimestamp(), "2/6", hex8(RAMTestAddressMiddle + Motherboard::RAM_START), "0x1B", hex2(memory.memory[RAMTestAddressMiddle]), "FAIL", "RAM Writing 8-bit (write8)", "~"});
        testFailed(outputFile, tests);
    }

    memory.write8(RAMTestAddressEnd, 0x21);
    if (memory.memory[RAMTestAddressEnd] == 0x21) {
        tests.push_back({getTimestamp(), "3/6", hex8(Motherboard::RAM_END), "0x21", "0x21", "PASS", "RAM Writing 8-bit (write8)", "~"});
    } else {
        tests.push_back({getTimestamp(), "3/6", hex8(Motherboard::RAM_END), "0x21", hex2(memory.memory[RAMTestAddressEnd]), "FAIL", "RAM Writing 8-bit (write8)", "~"});
        testFailed(outputFile, tests);
    }

    memory.write8(RAMTestAddressStart, 0x03);
    if (memory.memory[RAMTestAddressStart] == 0x03) {
        tests.push_back({getTimestamp(), "4/6", hex8(Motherboard::RAM_START), "0x03", "0x03", "PASS", "RAM Writing 8-bit (write8)", "Overwriting"});
    } else {
        tests.push_back({getTimestamp(), "4/6", hex8(Motherboard::RAM_START), "0x03", hex2(memory.memory[RAMTestAddressStart]), "FAIL", "RAM Writing 8-bit (write8)", "Overwriting"});
        testFailed(outputFile, tests);
    }

    memory.write8(RAMTestAddressMiddle, 0x6A);
    if (memory.memory[RAMTestAddressMiddle] == 0x6A) {
        tests.push_back({getTimestamp(), "5/6", hex8(RAMTestAddressMiddle + Motherboard::RAM_START), "0x6A", "0x6A", "PASS", "RAM Writing 8-bit (write8)", "Overwriting"});
    } else {
        tests.push_back({getTimestamp(), "5/6", hex8(RAMTestAddressMiddle + Motherboard::RAM_START), "0x6A", hex2(memory.memory[RAMTestAddressMiddle]), "FAIL", "RAM Writing 8-bit (write8)", "Overwriting"});
        testFailed(outputFile, tests);
    }

    memory.write8(RAMTestAddressEnd, 0xC3);
    if (memory.memory[RAMTestAddressEnd] == 0xC3) {
        tests.push_back({getTimestamp(), "6/6   PASS", hex8(Motherboard::RAM_END), "0xC3", "0xC3", "PASS", "RAM Writing 8-bit (write8)", "Overwriting"});
    } else {
        tests.push_back({getTimestamp(), "6/6   FAIL", hex8(Motherboard::RAM_END), "0xC3", hex2(memory.memory[RAMTestAddressEnd]), "FAIL", "RAM Writing 8-bit (write8)", "Overwriting"});
        testFailed(outputFile, tests);
    }

    // CLEANING

    memory.write8(RAMTestAddressStart, 0x00);
    memory.write8(RAMTestAddressMiddle + Motherboard::RAM_START, 0x00);
    memory.write8(RAMTestAddressEnd, 0x00);


    vector<uint8_t> testBytes1 = {0xDE};
    vector<uint8_t> testBytes2 = {0x99};
    vector<uint8_t> testBytes3 = {0x18};
    vector<uint8_t> testBytes4 = {0xE4};
    vector<uint8_t> testBytes5 = {0xC1};
    vector<uint8_t> testBytes6 = {0xF7};

    memory.writeBytesVector(RAMTestAddressStart, testBytes1);
    if (memory.memory[RAMTestAddressStart] == testBytes1[0]) {
        tests.push_back({getTimestamp(), "1/6", hex8(Motherboard::RAM_START), hex2(testBytes1[0]), hex2(testBytes1[0]), "PASS", "RAM Writing 8-bit (writeBytesVector)", "~"});
    } else {
        tests.push_back({getTimestamp(), "1/6", hex8(Motherboard::RAM_START), hex2(testBytes1[0]), hex2(memory.memory[RAMTestAddressStart]), "FAIL", "RAM Writing 8-bit (writeBytesVector)", "~"});
        testFailed(outputFile, tests);
    }

    memory.writeBytesVector(RAMTestAddressMiddle, testBytes2);
    if (memory.memory[RAMTestAddressMiddle] == testBytes2[0]) {
        tests.push_back({getTimestamp(), "2/6", hex8(RAMTestAddressMiddle + Motherboard::RAM_START), hex2(testBytes2[0]), hex2(testBytes2[0]), "PASS", "RAM Writing 8-bit (writeBytesVector)", "~"});
    } else {
        tests.push_back({getTimestamp(), "2/6", hex8(RAMTestAddressMiddle + Motherboard::RAM_START), hex2(testBytes2[0]), hex2(memory.memory[RAMTestAddressMiddle]), "FAIL", "RAM Writing 8-bit (writeBytesVector)", "~"});
        testFailed(outputFile, tests);
    }

    memory.writeBytesVector(RAMTestAddressEnd, testBytes3);
    if (memory.memory[RAMTestAddressEnd] == testBytes3[0]) {
        tests.push_back({getTimestamp(), "3/6", hex8(Motherboard::RAM_END), hex2(testBytes3[0]), hex2(testBytes3[0]), "PASS", "RAM Writing 8-bit (writeBytesVector)", "~"});
    } else {
        tests.push_back({getTimestamp(), "3/6", hex8(Motherboard::RAM_END), hex2(testBytes3[0]), hex2(memory.memory[RAMTestAddressEnd]), "FAIL", "RAM Writing 8-bit (writeBytesVector)", "~"});
        testFailed(outputFile, tests);
    }

    memory.writeBytesVector(RAMTestAddressStart, testBytes4);
    if (memory.memory[RAMTestAddressStart] == testBytes4[0]) {
        tests.push_back({getTimestamp(), "4/6", hex8(Motherboard::RAM_START), hex2(testBytes4[0]), hex2(testBytes4[0]), "PASS", "RAM Writing 8-bit (writeBytesVector)", "Overwriting"});
    } else {
        tests.push_back({getTimestamp(), "4/6", hex8(Motherboard::RAM_START), hex2(testBytes4[0]), hex2(memory.memory[RAMTestAddressStart]), "FAIL", "RAM Writing 8-bit (writeBytesVector)", "Overwriting"});
        testFailed(outputFile, tests);
    }

    memory.writeBytesVector(RAMTestAddressMiddle, testBytes5);
    if (memory.memory[RAMTestAddressMiddle] == testBytes5[0]) {
        tests.push_back({getTimestamp(), "5/6", hex8(RAMTestAddressMiddle + Motherboard::RAM_START), hex2(testBytes5[0]), hex2(testBytes5[0]), "PASS", "RAM Writing 8-bit (writeBytesVector)", "Overwriting"});
    } else {
        tests.push_back({getTimestamp(), "5/6", hex8(RAMTestAddressMiddle + Motherboard::RAM_START), hex2(testBytes5[0]), hex2(memory.memory[RAMTestAddressMiddle]), "FAIL", "RAM Writing 8-bit (writeBytesVector)", "Overwriting"});
        testFailed(outputFile, tests);
    }

    memory.writeBytesVector(RAMTestAddressEnd, testBytes6);
    if (memory.memory[RAMTestAddressEnd] == testBytes6[0]) {
        tests.push_back({getTimestamp(), "6/6   PASS", hex8(Motherboard::RAM_END), hex2(testBytes6[0]), hex2(testBytes6[0]), "PASS", "RAM Writing 8-bit (writeBytesVector)", "Overwriting"});
    } else {
        tests.push_back({getTimestamp(), "6/6   FAIL", hex8(Motherboard::RAM_END), hex2(testBytes6[0]), hex2(memory.memory[RAMTestAddressEnd]), "FAIL", "RAM Writing 8-bit (writeBytesVector)", "Overwriting"});
        testFailed(outputFile, tests);
    }

    // CLEANING

    memory.writeBytesVector(RAMTestAddressStart, {0x00});
    memory.writeBytesVector(RAMTestAddressMiddle + Motherboard::RAM_START, {0x00});
    memory.writeBytesVector(RAMTestAddressEnd, {0x00});


    testBytes1 = {0xDE,0x91,0x3B,0x7C};
    testBytes2 = {0xA4,0x2F,0x88,0x19};
    testBytes3 = {0x57,0xC2,0x0E,0xD1};
    testBytes4 = {0xF3,0x6B,0x9A,0x4D};
    testBytes5 = {0x0C,0xE7,0x5F,0xB2};
    testBytes6 = {0x81,0x3D,0x6A,0xF9};

    memory.writeBytesVector(RAMTestAddressStart, testBytes1);
    if (memory.memory[RAMTestAddressStart] == testBytes1[0] &&
        memory.memory[RAMTestAddressStart + 1] == testBytes1[1] &&
        memory.memory[RAMTestAddressStart + 2] == testBytes1[2] &&
        memory.memory[RAMTestAddressStart + 3] == testBytes1[3]) {
            tests.push_back({getTimestamp(), "1/6", hex8(Motherboard::RAM_START), formStringFromBytes(testBytes1), formStringFromBytes(testBytes1), "PASS", "RAM Writing 32-bit (writeBytesVector)", "~"});
    } else {
            tests.push_back({getTimestamp(), "1/6", hex8(Motherboard::RAM_START), formStringFromBytes(testBytes1), getStringGetBytes(RAMTestAddressStart, 4), "FAIL", "RAM Writing 32-bit (writeBytesVector)", "~"});
            testFailed(outputFile, tests);
    }

    memory.writeBytesVector(RAMTestAddressMiddle, testBytes2);
    if (memory.memory[RAMTestAddressMiddle] == testBytes2[0] &&
        memory.memory[RAMTestAddressMiddle + 1] == testBytes2[1] &&
        memory.memory[RAMTestAddressMiddle + 2] == testBytes2[2] &&
        memory.memory[RAMTestAddressMiddle + 3] == testBytes2[3]) {
            tests.push_back({getTimestamp(), "2/6", hex8(RAMTestAddressMiddle + Motherboard::RAM_START), formStringFromBytes(testBytes2), formStringFromBytes(testBytes2), "PASS", "RAM Writing 32-bit (writeBytesVector)", "~"});
    } else {
            tests.push_back({getTimestamp(), "2/6", hex8(RAMTestAddressMiddle + Motherboard::RAM_START), formStringFromBytes(testBytes2), getStringGetBytes(RAMTestAddressMiddle, 4), "FAIL", "RAM Writing 32-bit (writeBytesVector)", "~"});
            testFailed(outputFile, tests);
    }

    memory.writeBytesVector(RAMTestAddressEnd - 3, testBytes3);
    if (memory.memory[RAMTestAddressEnd - 3] == testBytes3[0] &&
        memory.memory[RAMTestAddressEnd - 2] == testBytes3[1] &&
        memory.memory[RAMTestAddressEnd - 1] == testBytes3[2] &&
        memory.memory[RAMTestAddressEnd] == testBytes3[3]) {
            tests.push_back({getTimestamp(), "3/6", hex8(Motherboard::RAM_END - 3), formStringFromBytes(testBytes3), formStringFromBytes(testBytes3), "PASS", "RAM Writing 32-bit (writeBytesVector)", "~"});
    } else {
            tests.push_back({getTimestamp(), "3/6", hex8(Motherboard::RAM_END - 3), formStringFromBytes(testBytes3), getStringGetBytes(RAMTestAddressEnd - 3, 4), "FAIL", "RAM Writing 32-bit (writeBytesVector)", "~"});
            testFailed(outputFile, tests);
    }

    memory.writeBytesVector(RAMTestAddressStart, testBytes4);
    if (memory.memory[RAMTestAddressStart] == testBytes4[0] &&
        memory.memory[RAMTestAddressStart + 1] == testBytes4[1] &&
        memory.memory[RAMTestAddressStart + 2] == testBytes4[2] &&
        memory.memory[RAMTestAddressStart + 3] == testBytes4[3]) {
            tests.push_back({getTimestamp(), "4/6", hex8(Motherboard::RAM_START), formStringFromBytes(testBytes4), formStringFromBytes(testBytes4), "PASS", "RAM Writing 32-bit (writeBytesVector)", "Overwriting"});
    } else {
            tests.push_back({getTimestamp(), "4/6", hex8(Motherboard::RAM_START), formStringFromBytes(testBytes4), getStringGetBytes(RAMTestAddressStart, 4), "FAIL", "RAM Writing 32-bit (writeBytesVector)", "Overwriting"});
            testFailed(outputFile, tests);
    }

    memory.writeBytesVector(RAMTestAddressMiddle, testBytes5);
    if (memory.memory[RAMTestAddressMiddle] == testBytes5[0] &&
        memory.memory[RAMTestAddressMiddle + 1] == testBytes5[1] &&
        memory.memory[RAMTestAddressMiddle + 2] == testBytes5[2] &&
        memory.memory[RAMTestAddressMiddle + 3] == testBytes5[3]) {
            tests.push_back({getTimestamp(), "5/6", hex8(RAMTestAddressMiddle + Motherboard::RAM_START), formStringFromBytes(testBytes5), formStringFromBytes(testBytes5), "PASS", "RAM Writing 32-bit (writeBytesVector)", "Overwriting"});
    } else {
            tests.push_back({getTimestamp(), "5/6", hex8(RAMTestAddressMiddle + Motherboard::RAM_START), formStringFromBytes(testBytes5), getStringGetBytes(RAMTestAddressMiddle, 4), "FAIL", "RAM Writing 32-bit (writeBytesVector)", "Overwriting"});
            testFailed(outputFile, tests);
    }

    memory.writeBytesVector(RAMTestAddressEnd - 3, testBytes6);
    if (memory.memory[RAMTestAddressEnd - 3] == testBytes6[0] &&
        memory.memory[RAMTestAddressEnd - 2] == testBytes6[1] &&
        memory.memory[RAMTestAddressEnd - 1] == testBytes6[2] &&
        memory.memory[RAMTestAddressEnd] == testBytes6[3]) {
            tests.push_back({getTimestamp(), "6/6   PASS", hex8(Motherboard::RAM_END - 3), formStringFromBytes(testBytes6), formStringFromBytes(testBytes6), "PASS", "RAM Writing 32-bit (writeBytesVector)", "Overwriting"});
    } else {
            tests.push_back({getTimestamp(), "6/6   FAIL", hex8(Motherboard::RAM_END - 3), formStringFromBytes(testBytes6), getStringGetBytes(RAMTestAddressEnd - 3, 4), "FAIL", "RAM Writing 32-bit (writeBytesVector)", "Overwriting"});
            testFailed(outputFile, tests);
    }

    // CLEANING

    memory.writeBytesVector(RAMTestAddressStart, {0x00, 0x00, 0x00, 0x00});
    memory.writeBytesVector(RAMTestAddressMiddle + Motherboard::RAM_START, {0x00, 0x00, 0x00, 0x00});
    memory.writeBytesVector(RAMTestAddressEnd - 3, {0x00, 0x00, 0x00, 0x00});


    testBytes1 = {0xDE,0x91,0x3B,0x7C,0x4F,0xA2,0x11,0x99};
    testBytes2 = {0xA4,0x2F,0x88,0x19,0x6B,0x3C,0xD0,0x0E};
    testBytes3 = {0x57,0xC2,0x0E,0xD1,0xF7,0x34,0x8A,0x22};
    testBytes4 = {0xF3,0x6B,0x9A,0x4D,0x1C,0xE5,0x7B,0xA1};
    testBytes5 = {0x0C,0xE7,0x5F,0xB2,0x8D,0x33,0xF9,0x40};
    testBytes6 = {0x81,0x3D,0x6A,0xF9,0x12,0xB4,0x67,0xC8};

    memory.writeBytesVector(RAMTestAddressStart, testBytes1);
    if (memory.memory[RAMTestAddressStart] == testBytes1[0] &&
        memory.memory[RAMTestAddressStart + 1] == testBytes1[1] &&
        memory.memory[RAMTestAddressStart + 2] == testBytes1[2] &&
        memory.memory[RAMTestAddressStart + 3] == testBytes1[3] &&
        memory.memory[RAMTestAddressStart + 4] == testBytes1[4] &&
        memory.memory[RAMTestAddressStart + 5] == testBytes1[5] &&
        memory.memory[RAMTestAddressStart + 6] == testBytes1[6] &&
        memory.memory[RAMTestAddressStart + 7] == testBytes1[7]) {
            tests.push_back({getTimestamp(), "1/6", hex8(Motherboard::RAM_START), formStringFromBytes(testBytes1), formStringFromBytes(testBytes1), "PASS", "RAM Writing 64-bit (writeBytesVector)", "~"});
    } else {
            tests.push_back({getTimestamp(), "1/6", hex8(Motherboard::RAM_START), formStringFromBytes(testBytes1), getStringGetBytes(RAMTestAddressStart, 8), "FAIL", "RAM Writing 64-bit (writeBytesVector)", "~"});
            testFailed(outputFile, tests);
    }

    memory.writeBytesVector(RAMTestAddressMiddle, testBytes2);
    if (memory.memory[RAMTestAddressMiddle] == testBytes2[0] &&
        memory.memory[RAMTestAddressMiddle + 1] == testBytes2[1] &&
        memory.memory[RAMTestAddressMiddle + 2] == testBytes2[2] &&
        memory.memory[RAMTestAddressMiddle + 3] == testBytes2[3] &&
        memory.memory[RAMTestAddressMiddle + 4] == testBytes2[4] &&
        memory.memory[RAMTestAddressMiddle + 5] == testBytes2[5] &&
        memory.memory[RAMTestAddressMiddle + 6] == testBytes2[6] &&
        memory.memory[RAMTestAddressMiddle + 7] == testBytes2[7]) {
            tests.push_back({getTimestamp(), "2/6", hex8(RAMTestAddressMiddle + Motherboard::RAM_START), formStringFromBytes(testBytes2), formStringFromBytes(testBytes2), "PASS", "RAM Writing 64-bit (writeBytesVector)", "~"});
    } else {
            tests.push_back({getTimestamp(), "2/6", hex8(RAMTestAddressMiddle + Motherboard::RAM_START), formStringFromBytes(testBytes2), getStringGetBytes(RAMTestAddressMiddle, 8), "FAIL", "RAM Writing 64-bit (writeBytesVector)", "~"});
            testFailed(outputFile, tests);
    }

    memory.writeBytesVector(RAMTestAddressEnd - 7, testBytes3);
    if (memory.memory[RAMTestAddressEnd - 7] == testBytes3[0] &&
        memory.memory[RAMTestAddressEnd - 6] == testBytes3[1] &&
        memory.memory[RAMTestAddressEnd - 5] == testBytes3[2] &&
        memory.memory[RAMTestAddressEnd - 4] == testBytes3[3] &&
        memory.memory[RAMTestAddressEnd - 3] == testBytes3[4] &&
        memory.memory[RAMTestAddressEnd - 2] == testBytes3[5] &&
        memory.memory[RAMTestAddressEnd - 1] == testBytes3[6] &&
        memory.memory[RAMTestAddressEnd] == testBytes3[7]) {
            tests.push_back({getTimestamp(), "3/6", hex8(Motherboard::RAM_END - 7), formStringFromBytes(testBytes3), formStringFromBytes(testBytes3), "PASS", "RAM Writing 64-bit (writeBytesVector)", "~"});
    } else {
            tests.push_back({getTimestamp(), "3/6", hex8(Motherboard::RAM_END - 7), formStringFromBytes(testBytes3), getStringGetBytes(RAMTestAddressEnd - 7, 8), "FAIL", "RAM Writing 64-bit (writeBytesVector)", "~"});
            testFailed(outputFile, tests);
    }

    memory.writeBytesVector(RAMTestAddressStart, testBytes4);
    if (memory.memory[RAMTestAddressStart] == testBytes4[0] &&
        memory.memory[RAMTestAddressStart + 1] == testBytes4[1] &&
        memory.memory[RAMTestAddressStart + 2] == testBytes4[2] &&
        memory.memory[RAMTestAddressStart + 3] == testBytes4[3] &&
        memory.memory[RAMTestAddressStart + 4] == testBytes4[4] &&
        memory.memory[RAMTestAddressStart + 5] == testBytes4[5] &&
        memory.memory[RAMTestAddressStart + 6] == testBytes4[6] &&
        memory.memory[RAMTestAddressStart + 7] == testBytes4[7]) {
            tests.push_back({getTimestamp(), "4/6", hex8(Motherboard::RAM_START), formStringFromBytes(testBytes4), formStringFromBytes(testBytes4), "PASS", "RAM Writing 64-bit (writeBytesVector)", "Overwriting"});
    } else {
            tests.push_back({getTimestamp(), "4/6", hex8(Motherboard::RAM_START), formStringFromBytes(testBytes4), getStringGetBytes(RAMTestAddressStart, 8), "FAIL", "RAM Writing 64-bit (writeBytesVector)", "Overwriting"});
            testFailed(outputFile, tests);
    }

    memory.writeBytesVector(RAMTestAddressMiddle, testBytes5);
    if (memory.memory[RAMTestAddressMiddle] == testBytes5[0] &&
        memory.memory[RAMTestAddressMiddle + 1] == testBytes5[1] &&
        memory.memory[RAMTestAddressMiddle + 2] == testBytes5[2] &&
        memory.memory[RAMTestAddressMiddle + 3] == testBytes5[3] &&
        memory.memory[RAMTestAddressMiddle + 4] == testBytes5[4] &&
        memory.memory[RAMTestAddressMiddle + 5] == testBytes5[5] &&
        memory.memory[RAMTestAddressMiddle + 6] == testBytes5[6] &&
        memory.memory[RAMTestAddressMiddle + 7] == testBytes5[7]) {
            tests.push_back({getTimestamp(), "5/6", hex8(RAMTestAddressMiddle + Motherboard::RAM_START), formStringFromBytes(testBytes5), formStringFromBytes(testBytes5), "PASS", "RAM Writing 64-bit (writeBytesVector)", "Overwriting"});
    } else {
            tests.push_back({getTimestamp(), "5/6", hex8(RAMTestAddressMiddle + Motherboard::RAM_START), formStringFromBytes(testBytes5), getStringGetBytes(RAMTestAddressMiddle, 8), "FAIL", "RAM Writing 64-bit (writeBytesVector)", "Overwriting"});
            testFailed(outputFile, tests);
    }

    memory.writeBytesVector(RAMTestAddressEnd - 7, testBytes6);
    if (memory.memory[RAMTestAddressEnd - 7] == testBytes6[0] &&
        memory.memory[RAMTestAddressEnd - 6] == testBytes6[1] &&
        memory.memory[RAMTestAddressEnd - 5] == testBytes6[2] &&
        memory.memory[RAMTestAddressEnd - 4] == testBytes6[3] &&
        memory.memory[RAMTestAddressEnd - 3] == testBytes6[4] &&
        memory.memory[RAMTestAddressEnd - 2] == testBytes6[5] &&
        memory.memory[RAMTestAddressEnd - 1] == testBytes6[6] &&
        memory.memory[RAMTestAddressEnd] == testBytes6[7]) {
            tests.push_back({getTimestamp(), "6/6   PASS", hex8(Motherboard::RAM_END - 7), formStringFromBytes(testBytes6), formStringFromBytes(testBytes6), "PASS", "RAM Writing 64-bit (writeBytesVector)", "Overwriting"});
    } else {
            tests.push_back({getTimestamp(), "6/6   FAIL", hex8(Motherboard::RAM_END - 7), formStringFromBytes(testBytes6), getStringGetBytes(RAMTestAddressEnd - 7, 8), "FAIL", "RAM Writing 64-bit (writeBytesVector)", "Overwriting"});
            testFailed(outputFile, tests);
    }

    // CLEANING

    memory.writeBytesVector(RAMTestAddressStart, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
    memory.writeBytesVector(RAMTestAddressMiddle + Motherboard::RAM_START, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
    memory.writeBytesVector(RAMTestAddressEnd - 7, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});


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

    memory.writeBytesVector(RAMTestAddressStart, testBytes1);
    bool pass1 = true;
    for (int i = 0; i < 64; i++) {
        if (memory.memory[RAMTestAddressStart + i] != testBytes1[i]) {
            pass1 = false;
            break;
        }
    }
    tests.push_back({
        getTimestamp(),
        "1/6",
        hex8(Motherboard::RAM_START),
        formStringFromBytes(testBytes1),
        pass1 ? formStringFromBytes(testBytes1) : getStringGetBytes(RAMTestAddressStart, 64),
        pass1 ? "PASS" : "FAIL",
        "RAM Writing 512-bit (writeBytesVector)",
        "~"
    });

    if (!pass1) {
        testFailed(outputFile, tests);
    }

    memory.writeBytesVector(RAMTestAddressMiddle, testBytes2);
    bool pass2 = true;
    for (int i = 0; i < 64; i++) {
        if (memory.memory[RAMTestAddressMiddle + i] != testBytes2[i]) {
            pass2 = false;
            break;
        }
    }

    tests.push_back({
        getTimestamp(),
        "2/6",
        hex8(RAMTestAddressMiddle + Motherboard::RAM_START),
        formStringFromBytes(testBytes2),
        pass2 ? formStringFromBytes(testBytes2) : getStringGetBytes(RAMTestAddressMiddle, 64),
        pass2 ? "PASS" : "FAIL",
        "RAM Writing 512-bit (writeBytesVector)",
        "~"
    });

    if (!pass2) {
        testFailed(outputFile, tests);
    }

    memory.writeBytesVector(RAMTestAddressEnd - 63, testBytes3);
    bool pass3 = true;
    for (int i = 0; i < 64; i++) {
        if (memory.memory[RAMTestAddressEnd - 63 + i] != testBytes3[i]) {
            pass3 = false;
            break;
        }
    }
    tests.push_back({
        getTimestamp(),
        "3/6",
        hex8(Motherboard::RAM_END - 63),
        formStringFromBytes(testBytes3),
        pass3 ? formStringFromBytes(testBytes3) : getStringGetBytes(RAMTestAddressEnd - 63, 64),
        pass3 ? "PASS" : "FAIL",
        "RAM Writing 512-bit (writeBytesVector)",
        "~"
    });

    if (!pass3) {
        testFailed(outputFile, tests);
    }

    memory.writeBytesVector(RAMTestAddressStart, testBytes4);
    bool pass4 = true;
    for (int i = 0; i < 64; i++) {
        if (memory.memory[RAMTestAddressStart + i] != testBytes4[i]) {
            pass4 = false;
            break;
        }
    }
    tests.push_back({
        getTimestamp(),
        "4/6",
        hex8(Motherboard::RAM_START),
        formStringFromBytes(testBytes4),
        pass4 ? formStringFromBytes(testBytes4) : getStringGetBytes(RAMTestAddressStart, 64),
        pass4 ? "PASS" : "FAIL",
        "RAM Writing 512-bit (writeBytesVector)",
        "Overwriting"
    });

    if (!pass4) {
        testFailed(outputFile, tests);
    }

    memory.writeBytesVector(RAMTestAddressMiddle, testBytes5);
    bool pass5 = true;
    for (int i = 0; i < 64; i++) {
        if (memory.memory[RAMTestAddressMiddle + i] != testBytes5[i]) {
            pass5 = false;
            break;
        }
    }
    tests.push_back({
        getTimestamp(),
        "5/6",
        hex8(RAMTestAddressMiddle + Motherboard::RAM_START),
        formStringFromBytes(testBytes5),
        pass5 ? formStringFromBytes(testBytes5) : getStringGetBytes(RAMTestAddressMiddle, 64),
        pass5 ? "PASS" : "FAIL",
        "RAM Writing 512-bit (writeBytesVector)",
        "Overwriting"
    });

    if (!pass5) {
        testFailed(outputFile, tests);
    }

    memory.writeBytesVector(RAMTestAddressEnd - 63, testBytes6);
    bool pass6 = true;
    for (int i = 0; i < 64; i++) {
        if (memory.memory[RAMTestAddressEnd - 63 + i] != testBytes6[i]) {
            pass6 = false;
            break;
        }
    }
    tests.push_back({
        getTimestamp(),
        pass6 ? "6/6   PASS" : "6/6   FAIL",
        hex8(Motherboard::RAM_END - 63),
        formStringFromBytes(testBytes6),
        pass6 ? formStringFromBytes(testBytes6) : getStringGetBytes(RAMTestAddressEnd - 63, 64),
        pass6 ? "PASS" : "FAIL",
        "RAM Writing 512-bit (writeBytesVector)",
        "Overwriting"
    });

    if (!pass6) {
        testFailed(outputFile, tests);
    }

    // CLEANING

    memory.writeBytesVector(RAMTestAddressStart, std::vector<uint8_t>(64, 0x00));
    memory.writeBytesVector(RAMTestAddressMiddle, std::vector<uint8_t>(64, 0x00));
    memory.writeBytesVector(RAMTestAddressEnd - 63, std::vector<uint8_t>(64, 0x00));


    // RAM READING

    testBytes1 = {0xAB};
    testBytes2 = {0x34};
    testBytes3 = {0xF2};
    testBytes4 = {0x7D};
    testBytes5 = {0x19};
    testBytes6 = {0xC6};

    memory.write8(RAMTestAddressStart, testBytes1[0]);

    if (memory.read8(RAMTestAddressStart) == testBytes1[0]) {
        tests.push_back({getTimestamp(), "1/6", hex8(Motherboard::RAM_START), hex2(testBytes1[0]), hex2(testBytes1[0]), "PASS", "RAM Reading 8-bit (read8)", "~"});
    } else {
        tests.push_back({getTimestamp(), "1/6", hex8(Motherboard::RAM_START), hex2(testBytes1[0]), hex2(memory.memory[RAMTestAddressStart]), "FAIL", "RAM Reading 8-bit (read8)", "~"});
        testFailed(outputFile, tests);
    }

    memory.write8(RAMTestAddressMiddle, testBytes2[0]);
    if (memory.read8(RAMTestAddressMiddle) == testBytes2[0]) {
        tests.push_back({getTimestamp(), "2/6", hex8(RAMTestAddressMiddle + Motherboard::RAM_START), hex2(testBytes2[0]), hex2(testBytes2[0]), "PASS", "RAM Reading 8-bit (read8)", "~"});
    } else {
        tests.push_back({getTimestamp(), "2/6", hex8(RAMTestAddressMiddle + Motherboard::RAM_START), hex2(testBytes2[0]), hex2(memory.memory[RAMTestAddressMiddle]), "FAIL", "RAM Reading 8-bit (read8)", "~"});
        testFailed(outputFile, tests);
    }

    memory.write8(RAMTestAddressEnd, testBytes3[0]);
    if (memory.read8(RAMTestAddressEnd) == testBytes3[0]) {
        tests.push_back({getTimestamp(), "3/6", hex8(Motherboard::RAM_END), hex2(testBytes3[0]), hex2(testBytes3[0]), "PASS", "RAM Reading 8-bit (read8)", "~"});
    } else {
        tests.push_back({getTimestamp(), "3/6", hex8(Motherboard::RAM_END), hex2(testBytes3[0]), hex2(memory.memory[RAMTestAddressEnd]), "FAIL", "RAM Reading 8-bit (read8)", "~"});
        testFailed(outputFile, tests);
    }

    memory.write8(RAMTestAddressStart, testBytes4[0]);
    if (memory.read8(RAMTestAddressStart) == testBytes4[0]) {
        tests.push_back({getTimestamp(), "4/6", hex8(Motherboard::RAM_START), hex2(testBytes4[0]), hex2(testBytes4[0]), "PASS", "RAM Reading 8-bit (read8)", "Overwriting"});
    } else {
        tests.push_back({getTimestamp(), "4/6", hex8(Motherboard::RAM_START), hex2(testBytes4[0]), hex2(memory.memory[RAMTestAddressStart]), "FAIL", "RAM Reading 8-bit (read8)", "Overwriting"});
        testFailed(outputFile, tests);
    }

    memory.write8(RAMTestAddressMiddle, testBytes5[0]);
    if (memory.read8(RAMTestAddressMiddle) == testBytes5[0]) {
        tests.push_back({getTimestamp(), "5/6", hex8(RAMTestAddressMiddle + Motherboard::RAM_START), hex2(testBytes5[0]), hex2(testBytes5[0]), "PASS", "RAM Reading 8-bit (read8)", "Overwriting"});
    } else {
        tests.push_back({getTimestamp(), "5/6", hex8(RAMTestAddressMiddle + Motherboard::RAM_START), hex2(testBytes5[0]), hex2(memory.memory[RAMTestAddressMiddle]), "FAIL", "RAM Reading 8-bit (read8)", "Overwriting"});
        testFailed(outputFile, tests);
    }

    memory.write8(RAMTestAddressEnd, testBytes6[0]);
    if (memory.read8(RAMTestAddressEnd) == testBytes6[0]) {
        tests.push_back({getTimestamp(), "6/6   PASS", hex8(Motherboard::RAM_END), hex2(testBytes6[0]), hex2(testBytes6[0]), "PASS", "RAM Reading 8-bit (read8)", "Overwriting"});
    } else {
        tests.push_back({getTimestamp(), "6/6   FAIL", hex8(Motherboard::RAM_END), hex2(testBytes6[0]), hex2(memory.memory[RAMTestAddressEnd]), "FAIL", "RAM Reading 8-bit (read8)", "Overwriting"});
        testFailed(outputFile, tests);
    }

    // CLEANING

    memory.write8(RAMTestAddressStart, 0x00);
    memory.write8(RAMTestAddressMiddle + Motherboard::RAM_START, 0x00);
    memory.write8(RAMTestAddressEnd, 0x00);

    fillLog(outputFile, tests);

    return 0;
};