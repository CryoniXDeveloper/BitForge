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
        std::cerr << "\nError: File has no extension.";
        exit(0);
    }

    std::string extractedString = fileName.substr(dotPosition + 1);

    if (extractedString != fileExtension) {
        std::cerr << "\nError: File extension is incorrect. (" + fileName + "), extension expected: ." + fileExtension;
        exit(0);
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

int main() {
    std::cout << "Enter the path of the assembly file to be converted into binary: ";

    std::string assemblyFile;
    std::getline(std::cin, assemblyFile);

    std::ifstream inFile(assemblyFile);
    if (!inFile) {
        std::cerr << "\nError: Assembly file does not exist or cannot be opened.\n";
        return 1;
    }

    assembler.checkFileExtension(assemblyFile, "trasm");

    std::cout << std::endl << "Enter the path of the output file where the binary should be saved: ";

    std::string outputFile;
    std::getline(std::cin, outputFile);

    assembler.checkFileExtension(outputFile, "bin");

    std::cout << "Turning assembly into binary..." << std::endl;

    std::ifstream asmFile(assemblyFile);
    if (!asmFile) {
        std::cerr << "Error: Could not open assembly file for reading." << std::endl;
        return 1;
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

        if (tokens[currentIndex] == "mval" && tokens.size() > 1 && tokens[1].rfind("mb", 0) == 0) {
            tokens[currentIndex] = tokens[currentIndex] + " " + tokens[currentIndex + 1];
            toAdd = 2;
        } else {
            toAdd = 1;
        }

        std::vector<uint8_t> currentData;
        try {
            currentData = assembler.getOpcode(tokens[currentIndex]);
            std::cout << "[Line " << lineNumber << "] Opcode for \"" << tokens[currentIndex] << "\":";
            for (auto b : currentData) std::cout << " " << std::hex << (int)b;
            std::cout << std::dec << std::endl;
        } catch (const std::out_of_range&) {
            std::cerr << "[Line " << lineNumber << "] ERROR: Opcode \"" << tokens[currentIndex] << "\" not found in map!" << std::endl;
            continue;
        }

        assembler.binaryToWrite.insert(
            assembler.binaryToWrite.end(),
            currentData.begin(),
            currentData.end()
        );

        if (Assembler::operandInfoMnemonics.find(tokens[0]) != Assembler::operandInfoMnemonics.end()) {
            currentIndex += toAdd;
            if (currentIndex >= tokens.size()) {
                std::cerr << "[Line " << lineNumber << "] ERROR: No operand descriptor after mnemonic!" << std::endl;
                continue;
            }

            try {
                currentData = assembler.getOperandInfoBytes(tokens[currentIndex]);
                std::cout << "[Line " << lineNumber << "] Operand bytes for \"" << tokens[currentIndex] << "\":";
                for (auto b : currentData) std::cout << " " << std::hex << (int)b;
                std::cout << std::dec << std::endl;
            } catch (const std::out_of_range&) {
                std::cerr << "[Line " << lineNumber << "] ERROR: Operand \"" << tokens[currentIndex] << "\" not found in map!" << std::endl;
                continue;
            }

            assembler.binaryToWrite.insert(
                assembler.binaryToWrite.end(),
                currentData.begin(),
                currentData.end()
            );
        }
    }

    std::ofstream outFile(outputFile, std::ios::binary);
    if (!outFile) {
        std::cerr << "Error: Could not open output file for writing." << std::endl;
        return 1;
    }
    outFile.write(reinterpret_cast<const char*>(assembler.binaryToWrite.data()), assembler.binaryToWrite.size());
    outFile.close();

    std::cout << "Binary successfully written to " << outputFile << std::endl;
    return 0;
}