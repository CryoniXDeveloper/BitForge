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

std::vector<uint8_t> Assembler::getOperandInfoBytes(std::string operandDescriptor) {
    return operandByte.at(operandDescriptor);
}

void Assembler::error(std::string errorType, std::string info) const {
    std::string returnString = "ERROR [" + errorType + "]";
    if (info != "")
        returnString += " - More info: " + info;
    returnString += "\n\nFind out more in errorTypes.txt.\nStopping execution...";
    std::cout << returnString;
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

        try {

            if (tokens[currentIndex] == "nac") {
                currentData = assembler.getOpcode(tokens[currentIndex]);
                pushData(currentData);
            }

            else if (tokens[currentIndex] == "mval" &&
                    Assembler::operandInfoMnemonics.find(tokens[currentIndex] + " " + tokens[currentIndex + 1]) != Assembler::operandInfoMnemonics.end()) {
                
                mnemonic = tokens[currentIndex] + " " + tokens[currentIndex + 1];
                        
                currentData = assembler.getOpcode(mnemonic);
                pushData(currentData);

                currentIndex += 2;

                currentData = assembler.getOperandInfoBytes(tokens[currentIndex]);
                pushData(currentData);
                
                int size1 = Assembler::operandMapBytes.at(tokens[currentIndex]).size1;
                int size2 = Assembler::operandMapBytes.at(tokens[currentIndex]).size2;

                currentIndex += 1;

                currentData = assembler.getOperandInfoBytes(tokens[currentIndex]);
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
            }

            else if (tokens[currentIndex] == "sleepins") {
                currentData = assembler.getOpcode(tokens[currentIndex]);
                pushData(currentData);
            }

            else {
                assembler.error("ASM00005", assemblyFile + "; line: " + std::to_string(lineNumber));
            }
        }
        catch (const std::exception& e) {
            assembler.error("ASM00006", assemblyFile + "; line: " + std::to_string(lineNumber));
        }
    }

    std::ofstream outFile(outputFile, std::ios::binary);
    if (!outFile) {
        assembler.error("ASM00007", outputFile);
    }

    outFile.write(reinterpret_cast<const char*>(assembler.binaryToWrite.data()), assembler.binaryToWrite.size());
    outFile.close();

    std::cout << "Binary successfully written to " << outputFile << std::endl;
    return 0;
}