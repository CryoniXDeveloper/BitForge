#include "assembler.h"
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cctype>
#include <cstdint>
#include <cstdlib>

Assembler assembler;

void Assembler::checkFileExtension(std::string fileName, std::string fileExtension) {
    size_t dotPosition = fileName.rfind('.');

    if (dotPosition == std::string::npos) {
        assembler.error("ASM00002", fileName);
    }

    std::string extractedString = fileName.substr(dotPosition + 1);

    if (extractedString != fileExtension) {
        assembler.error("ASM00003", fileName + "; extension expected: ." + fileExtension);
    }
}

std::string Assembler::trim(const std::string& str) {
    size_t start = 0;
    while (start < str.size() && std::isspace(str[start])) ++start;

    size_t end = str.size();
    while (end > start && std::isspace(str[end - 1])) --end;

    return str.substr(start, end - start);
}

std::vector<std::string> Assembler::splitOperandDescriptor(const std::string& str) {
    std::vector<std::string> parts;
    size_t start = 0;
    size_t pos = str.find('<');

    while (pos != std::string::npos) {
        parts.push_back(str.substr(start, pos - start + 1));
        start = pos + 1;
        pos = str.find('<', start);
    }

    if (start < str.size()) {
        parts.push_back(str.substr(start));
    }

    return parts;
}

std::vector<std::string> Assembler::split(const std::string& str) {
    std::vector<std::string> tokens;
    size_t i = 0;

    while (i < str.size()) {
        while (i < str.size() && std::isspace(str[i])) ++i;
        if (i >= str.size()) break;

        if (str[i] == '"') {
            ++i;
            size_t start = i;
            while (i < str.size() && str[i] != '"') ++i;
            tokens.push_back(str.substr(start, i - start));
            if (i < str.size() && str[i] == '"') ++i;

        } else {
            size_t start = i;
            while (i < str.size() && !std::isspace(str[i])) ++i;
            tokens.push_back(str.substr(start, i - start));
        }
    }

    return tokens;
}

std::vector<uint8_t> Assembler::getOpcode(std::string mnemonic) {
    return mnemonicOpcode.at(mnemonic);
}

std::vector<uint8_t> Assembler::getOperandInfoByte(std::string operandDescriptor) {
    return operandByte.at(operandDescriptor);
}

void Assembler::error(std::string errorType, std::string info) const {
    std::string returnString = "ERROR [" + errorType + "]";
    if (info != "")
        returnString += " - More info: " + info;
    returnString += "\n\nFind out more in errorTypes.txt.\nStopping execution...\n";
    
    std::cout << returnString << std::endl; 
    
    std::cin.get();
    exit(0);
}

void pushData(std::vector<uint8_t> data) {
    assembler.binaryToWrite.insert(
        assembler.binaryToWrite.end(),
        data.begin(),
        data.end()
    );
}

std::vector<uint8_t> intToBytes(const std::string& valueStr, int amountOfBytes) {
    std::vector<uint8_t> result;
    uint64_t value;

    try {
        if (valueStr.size() > 2 && valueStr[0] == '0' && valueStr[1] == 'b') {
            // Binary
            value = std::stoull(valueStr.substr(2), nullptr, 2);
        } else if (valueStr.size() > 2 && valueStr[0] == '0' && valueStr[1] == 'x') {
            // Hex
            value = std::stoull(valueStr.substr(2), nullptr, 16);
        } else {
            // Decimal
            value = std::stoull(valueStr, nullptr, 10);
        }
    } catch (const std::exception& e) {
        assembler.error("ASM00005", "Invalid number: " + valueStr);
    }

    uint64_t maxValue = (amountOfBytes >= 8) ? 0xFFFFFFFFFFFFFFFF : ((1ULL << (8 * amountOfBytes)) - 1);

    if (value > maxValue) {
        assembler.error("ASM00005", "Operand value " + valueStr + " too large for " + std::to_string(amountOfBytes) + " byte(s)");
    }

    for (int x = 0; x < amountOfBytes; x++) {
        result.push_back(static_cast<uint8_t>(value & 0xFF));
        value >>= 8;
    }

    return result;
}

int main() {
    std::cout << "Enter the path of the assembly file to be converted into binary: ";

    std::string assemblyFile;
    std::getline(std::cin, assemblyFile);

    if (assemblyFile.empty()) {
        assemblyFile = "asmFile.trasm";
    }

    std::ifstream inFile(assemblyFile);
    if (!inFile) {
        assembler.error("ASM00001", assemblyFile);
    }

    assembler.checkFileExtension(assemblyFile, "trasm");

    std::cout << std::endl << "Enter the path of the output file where the binary should be saved: ";

    std::string outputFile;
    std::getline(std::cin, outputFile);

    if (outputFile.empty()) {
        outputFile = "output.bin";
    }

    assembler.checkFileExtension(outputFile, "bin");

    std::cout << "Turning assembly into binary..." << std::endl;

    std::ifstream asmFile(assemblyFile);
    if (!asmFile) {
        assembler.error("ASM00004", assemblyFile);
    }

    std::string line;
    size_t lineNumber = 0;

    while (std::getline(asmFile, line)) {
        ++lineNumber;

        if (line.find_first_not_of(" \t\r\n") == std::string::npos) continue;

        std::vector<std::string> tokens = assembler.split(assembler.trim(line));
        if (tokens.empty()) continue;

        size_t currentIndex = 0;
        size_t toAdd = 0;
        std::vector<uint8_t> currentData;
        std::string mnemonic;

        std::string operandDescriptor1;
        std::string operandDescriptor2;

        try {

            if (tokens[currentIndex] == "nac") {
                currentData = assembler.getOpcode(tokens[currentIndex]);
                pushData(currentData);
            }

            else if (Assembler::mvalMnemonics.find(tokens.at(currentIndex)) != Assembler::mvalMnemonics.end()) {
                mnemonic = tokens.at(currentIndex);
                currentData = assembler.getOpcode(mnemonic);
                pushData(currentData);

                currentIndex += 1;
                if (currentIndex >= tokens.size()) {
                    assembler.error("ASM00005", "Missing descriptors at line " + std::to_string(lineNumber));
                }

                std::vector<std::string> descriptors = assembler.splitOperandDescriptor(tokens.at(currentIndex));
                
                if (descriptors.empty()) {
                    assembler.error("ASM00005", "Invalid descriptor format at line " + std::to_string(lineNumber));
                }

                operandDescriptor1 = descriptors.at(0);
                
                if (descriptors.size() > 1) {
                    operandDescriptor2 = descriptors.at(1);
                } else {
                    operandDescriptor2 = operandDescriptor1;
                }

                if (operandDescriptor2 == "<" || operandDescriptor2.empty()) {
                    operandDescriptor2 = operandDescriptor1;
                }

                currentData = assembler.getOperandInfoByte(operandDescriptor1);
                pushData(currentData);

                int size1 = Assembler::operandMapBytes.at(operandDescriptor1).size;
                std::string type1 = Assembler::operandMapBytes.at(operandDescriptor1).type;

                currentData = assembler.getOperandInfoByte(operandDescriptor2);
                pushData(currentData);

                int size2 = Assembler::operandMapBytes.at(operandDescriptor2).size;
                std::string type2 = Assembler::operandMapBytes.at(operandDescriptor2).type;

                if (mnemonic.rfind("mval", 0) == 0 && type2 == "i") {
                    int expectedSize = std::stoi(mnemonic.substr(4)) / 8;
                    if (size2 != expectedSize) {
                        assembler.error("ASM00005", "Immediate type mismatch at line " + std::to_string(lineNumber));
                    }
                }

                currentIndex += 1;
                if (currentIndex >= tokens.size()) {
                    assembler.error("ASM00005", "Missing 1st operand value at line " + std::to_string(lineNumber));
                }

                currentData = intToBytes(tokens.at(currentIndex), size1);

                if ((type1 == "r" || type1 == "mr") && static_cast<int>(currentData[0]) > assembler.registers64max) {
                    assembler.error("ASM00005", "No such register; line " + std::to_string(lineNumber));
                }

                pushData(currentData);

                currentIndex += 1;
                if (currentIndex >= tokens.size()) {
                    assembler.error("ASM00005", "Missing 2nd operand value at line " + std::to_string(lineNumber));
                }

                currentData = intToBytes(tokens.at(currentIndex), size2);

                if ((type2 == "r" || type2 == "mr") && static_cast<int>(currentData[0]) > assembler.registers64max) {
                    assembler.error("ASM00005", "No such register; line " + std::to_string(lineNumber));
                }

                pushData(currentData);
            }

            else if (tokens[currentIndex] == "wait") {
                currentData = assembler.getOpcode(tokens[currentIndex]);
                pushData(currentData);
            }

            else if (tokens[currentIndex] == "stop") {
                currentData = assembler.getOpcode(tokens[currentIndex]);
                pushData(currentData);
            }

            else if (tokens[currentIndex] == "sleepms") {
                currentData = assembler.getOpcode(tokens[currentIndex]);
                pushData(currentData);

                currentIndex += 1;

                currentData = assembler.getOperandInfoByte(tokens[currentIndex]);
                pushData(currentData);
                
                int size1 = Assembler::operandMapBytes.at(tokens[currentIndex]).size;

                currentIndex += 1;
                
                currentData = intToBytes(tokens[currentIndex], size1);
                pushData(currentData);
            }

            else if (tokens[currentIndex] == "sleepsec") {
                currentData = assembler.getOpcode(tokens[currentIndex]);
                pushData(currentData);

                currentIndex += 1;

                currentData = assembler.getOperandInfoByte(tokens[currentIndex]);
                pushData(currentData);
                
                int size1 = Assembler::operandMapBytes.at(tokens[currentIndex]).size;

                currentIndex += 1;
                
                currentData = intToBytes(tokens[currentIndex], size1);
                pushData(currentData);
            }

            else {
                assembler.error("ASM00005", assemblyFile + "; line: " + std::to_string(lineNumber));
            }
        }

        catch (const std::exception& e) {
            assembler.error("ASM00006", assemblyFile + "; line: " + std::to_string(lineNumber) + "; " + e.what());
        }
    }

    std::ofstream outFile(outputFile, std::ios::binary | std::ios::trunc);
    if (!outFile) {
        assembler.error("ASM00007", outputFile);
    }

    outFile.write(reinterpret_cast<const char*>(assembler.binaryToWrite.data()), assembler.binaryToWrite.size());
    outFile.close();

    std::cout << "Binary successfully written to " << outputFile << std::endl;
    return 0;
}