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
    if (amountOfBytes <= 0) return std::vector<uint8_t>();

    std::string clean = valueStr;
    int base = 10;
    bool isNeg = false;
    std::vector<uint8_t> result(amountOfBytes, 0);

    try {
        if (clean.empty()) throw 2;
        if (clean[0] == '-') { 
            isNeg = true; 
            clean = clean.substr(1); 
            if (clean.empty()) throw 2;
        }

        if (clean.size() > 2 && clean[0] == '0' && (tolower(clean[1]) == 'x' || tolower(clean[1]) == 'b')) {
            base = (tolower(clean[1]) == 'x') ? 16 : 2;
            clean = clean.substr(2);
            if (clean.empty()) throw 2;
        } else if (!isdigit(clean[0])) {
            throw 2;
        }

        for (char c : clean) {
            if (base == 16 && !isxdigit(c)) throw 2;
            if (base == 2 && (c != '0' && c != '1')) throw 2;
            if (base == 10 && !isdigit(c)) throw 2;
        }

        if (base == 16) {
            if (((int)clean.size() + 1) / 2 != amountOfBytes) throw 1;
            if (clean.size() % 2 != 0) clean = "0" + clean;
            int tidx = 0;
            for (int i = (int)clean.size() - 2; i >= 0; i -= 2) {
                result[tidx++] = (uint8_t)std::stoul(clean.substr(i, 2), nullptr, 16);
            }
        } 
        else if (base == 2) {
            if (((int)clean.size() + 7) / 8 != amountOfBytes) throw 1;
            int tidx = 0;
            for (int i = (int)clean.size(); i > 0; i -= 8) {
                int start = std::max(0, i - 8);
                result[tidx++] = (uint8_t)std::stoi(clean.substr(start, i - start), nullptr, 2);
            }
        } 
        else {
            for (char c : clean) {
                uint32_t carry = c - '0';
                for (int i = 0; i < amountOfBytes; i++) {
                    uint32_t val = (uint32_t)result[i] * 10 + carry;
                    result[i] = val & 0xFF;
                    carry = val >> 8;
                }
                if (carry > 0) throw 1;
            }

            if (isNeg) {
                uint16_t carry = 1;
                for (int i = 0; i < amountOfBytes; i++) {
                    uint16_t val = (uint16_t)(result[i] ^ 0xFF) + carry;
                    result[i] = val & 0xFF;
                    carry = val >> 8;
                }
            }

            if (amountOfBytes > 1 && result[amountOfBytes - 1] == 0) {
                bool allZero = true;
                for (uint8_t b : result) if (b != 0) allZero = false;
                if (!allZero) throw 1;
            }
        }

        return result;

    } catch (int e) {
        if (e == 2) {
            assembler.error("ASM00006", "Invalid value: " + valueStr);
        } else {
            assembler.error("ASM00005", "Size mismatch; size expected: " + std::to_string(amountOfBytes) + "; value: " + valueStr);
        }
        return std::vector<uint8_t>(amountOfBytes, 0); 
    }
}

std::vector<std::string> removeCommentsFromFile(const std::string& filename) {
    std::ifstream inFile(filename);
    if (!inFile) {
        throw std::runtime_error("Cannot open file: " + filename);
    }

    std::vector<std::string> cleanedLines;
    std::string line;
    bool inMultiLineComment = false;

    while (std::getline(inFile, line)) {
        std::string result;
        bool inString = false;

        for (size_t i = 0; i < line.size(); ++i) {
            if (line[i] == '"' && !inMultiLineComment) {
                inString = !inString;
                result += line[i];
                continue;
            }

            if (!inString && !inMultiLineComment && i + 1 < line.size() && line[i] == '<' && line[i+1] == '*') {
                inMultiLineComment = true;
                ++i;
                continue;
            }

            if (!inString && inMultiLineComment && i + 1 < line.size() && line[i] == '*' && line[i+1] == '>') {
                inMultiLineComment = false;
                ++i;
                continue;
            }

            if (inMultiLineComment) continue;

            if (!inString && i + 1 < line.size() && line[i] == '*' && line[i+1] == '*') {
                break;
            }

            result += line[i];
        }

        size_t start = 0;
        while (start < result.size() && std::isspace(result[start])) ++start;

        size_t end = result.size();
        while (end > start && std::isspace(result[end-1])) --end;

        if (start < end) cleanedLines.push_back(result.substr(start, end - start));
    }

    return cleanedLines;
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

    size_t lineNumber = 0;
    std::vector<std::string> lines;

    try {
        lines = removeCommentsFromFile(assemblyFile);
    } catch (const std::exception& e) {
        assembler.error("ASM00004", e.what());
    }

    for (const auto& line : lines) {
        ++lineNumber;

        if (line.empty()) continue;

        std::vector<std::string> tokens = assembler.split(line);
        if (tokens.empty()) continue;

        size_t currentIndex = 0;
        size_t toAdd = 0;
        std::vector<uint8_t> currentData;
        std::string mnemonic;

        std::string operandDescriptor1;
        std::string operandDescriptor2;

        try {

            if (tokens.at(currentIndex) == "nac") {
                currentData = assembler.getOpcode(tokens.at(currentIndex));
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
                
                if (size2 == 0) {
                    assembler.error("ASM00005", "For moving immediates bigger than 64 bits use mvalplus only; line " + std::to_string(lineNumber));
                }

                std::string type2 = Assembler::operandMapBytes.at(operandDescriptor2).type;

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

            else if (tokens.at(currentIndex) == "mvalplus") {
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

                if (type2 == "i") {
                    if (size2 != 0) {
                        assembler.error("ASM00005", "Immediate must have no size; line " + std::to_string(lineNumber));
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

            else if (tokens.at(currentIndex) == "wait") {
                currentData = assembler.getOpcode(tokens.at(currentIndex));
                pushData(currentData);
            }

            else if (tokens.at(currentIndex) == "stop") {
                currentData = assembler.getOpcode(tokens.at(currentIndex));
                pushData(currentData);
            }

            else if (tokens.at(currentIndex) == "sleepms") {
                currentData = assembler.getOpcode(tokens.at(currentIndex));
                pushData(currentData);

                currentIndex += 1;

                currentData = assembler.getOperandInfoByte(tokens.at(currentIndex));
                pushData(currentData);
                
                int size1 = Assembler::operandMapBytes.at(tokens.at(currentIndex)).size;

                currentIndex += 1;
                
                currentData = intToBytes(tokens.at(currentIndex), size1);
                pushData(currentData);
            }

            else if (tokens.at(currentIndex) == "sleepsec") {
                currentData = assembler.getOpcode(tokens.at(currentIndex));
                pushData(currentData);

                currentIndex += 1;

                currentData = assembler.getOperandInfoByte(tokens.at(currentIndex));
                pushData(currentData);
                
                int size1 = Assembler::operandMapBytes.at(tokens.at(currentIndex)).size;

                currentIndex += 1;
                
                currentData = intToBytes(tokens.at(currentIndex), size1);
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