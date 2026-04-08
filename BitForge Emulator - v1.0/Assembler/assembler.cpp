#include "assembler.h"
#include <fstream>
#include <iostream>
#include <cctype>
#include <cstdlib>

Assembler assembler;

std::string Assembler::trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace(s[a])) ++a;
    while (b > a && std::isspace(s[b-1])) --b;
    return s.substr(a, b - a);
}

std::vector<std::string> Assembler::split(const std::string& s) {
    std::vector<std::string> out;
    size_t i = 0;
    while (i < s.size()) {
        while (i < s.size() && std::isspace(s[i])) ++i;
        if (i >= s.size()) break;
        if (s[i] == '"') {
            size_t start = ++i;
            while (i < s.size() && s[i] != '"') ++i;
            out.push_back(s.substr(start, i - start));
            if (i < s.size()) ++i;
        } else {
            size_t start = i;
            while (i < s.size() && !std::isspace(s[i])) ++i;
            std::string token = s.substr(start, i - start);
            if (token.find('<') != std::string::npos) {
                auto parts = parseDescriptor(token);
                for (const auto& p : parts) out.push_back(p);
            } else {
                out.push_back(token);
            }
        }
    }
    return out;
}

std::vector<std::string> Assembler::parseDescriptor(const std::string& desc) {
    std::vector<std::string> parts;
    std::string current;
    for (char c : desc) {
        if (c == '<') {
            parts.push_back(current);
            current = "";
        } else {
            current += c;
        }
    }

    std::vector<std::string> result;
    std::string last;
    for (const auto& part : parts) {
        if (part.empty()) {
            result.push_back(last);
        } else {
            result.push_back(part);
            last = part;
        }
    }
    return result;
}

std::vector<std::string> Assembler::readFile(const std::string& filename) {
    std::ifstream f(filename);
    if (!f) throw std::runtime_error("Cannot open: " + filename);
    std::vector<std::string> lines;
    std::string line;
    bool inBlock = false;
    while (std::getline(f, line)) {
        std::string result;
        bool inStr = false;
        for (size_t i = 0; i < line.size(); ++i) {
            if (line[i] == '"' && !inBlock) { inStr = !inStr; result += line[i]; continue; }
            if (!inStr && !inBlock && i+1 < line.size() && line[i] == '<' && line[i+1] == '*') { inBlock = true; ++i; continue; }
            if (!inStr && inBlock  && i+1 < line.size() && line[i] == '*' && line[i+1] == '>') { inBlock = false; ++i; continue; }
            if (inBlock) continue;
            if (!inStr && i+1 < line.size() && line[i] == '*' && line[i+1] == '*') break;
            result += line[i];
        }
        result = trim(result);
        if (!result.empty()) lines.push_back(result);
    }
    return lines;
}

void Assembler::error(const std::string& type, const std::string& info) const {
    std::cout << "ERROR [" << type << "]";
    if (!info.empty()) std::cout << " - " << info;
    std::cout << "\nStopping...\n";
    exit(0);
}

uint8_t Assembler::descriptorByte(const std::string& type) {
    if (type == "r8")  return 0x00;
    if (type == "i8")  return 0x01;
    if (type == "i16") return 0x02;
    if (type == "i32") return 0x03;
    if (type == "i64") return 0x04;
    if (type == "mi8")  return 0x05;
    if (type == "mi16") return 0x06;
    if (type == "mi32") return 0x07;
    if (type == "mi64") return 0x08;
    if (type == "mr8")  return 0x09;
    if (type == "i")    return 0x0A;
    if (type == "lbl") return 0x04;
    if (type == "str")    return 0x0A;
    error("ASM00005", "Unknown operand type: " + type);
    return 0;
}

int Assembler::operandSize(const std::string& type) {
    if (type == "r8" || type == "i8" || type == "mi8" || type == "mr8") return 1;
    if (type == "i16" || type == "mi16") return 2;
    if (type == "i32" || type == "mi32") return 4;
    if (type == "i64" || type == "mi64") return 8;
    if (type == "i") return 0;
    if (type == "lbl") return 8;
    if (type == "str") return 0;
    error("ASM00005", "Unknown operand size: " + type);
    return 0;
}

void Assembler::encodeOperand(const std::string& type, const std::string& valueToken) {
    int size = operandSize(type);
    uint64_t value = std::stoull(valueToken, nullptr, 0);
    for (int i = 0; i < size; i++) {
        output.push_back((value >> (i * 8)) & 0xFF);
    }
}

std::vector<uint8_t> Assembler::autoSizeBytes(const std::string& valueToken) {
    std::vector<uint8_t> bytes;
    std::string num = valueToken;

    while (num != "0" && !num.empty()) {
        int rem = 0;
        std::string next;
        for (char c : num) {
            rem = rem * 10 + (c - '0');
            if (!next.empty() || rem / 256) next += (char)('0' + rem / 256);
            rem %= 256;
        }
        bytes.push_back(rem);
        num = next.empty() ? "0" : next;
    }

    return bytes;
}

void Assembler::secondPass(const std::vector<std::string>& lines) {
    size_t lineNumber = 0;
    for (const auto& line : lines) {
        ++lineNumber;
        if (line.empty()) continue;

        std::vector<std::string> tokens = split(line);
        if (tokens.empty()) continue;

        if (tokens[0][0] == '!') {
            std::string name = tokens[0].substr(1, tokens[0].size() - 2);
            labels.push_back({name, (uint32_t)output.size()});
            continue;
        }

        else if (general0Operands.count(tokens[0])) {
            output.push_back(general0Operands.at(tokens[0]));
        }

        else if (general1Operands.count(tokens[0])) {
            output.push_back(general1Operands.at(tokens[0]));
            output.push_back(descriptorByte(tokens[1]));
            encodeOperand(tokens[1], tokens[2]);
        }

        else if (general3Operands.count(tokens[0])) {
            const auto& opcode = general3Operands.at(tokens[0]);
            for (auto b : opcode) output.push_back(b);
            output.push_back(descriptorByte(tokens[1]));
            output.push_back(descriptorByte(tokens[2]));
            output.push_back(descriptorByte(tokens[3]));
            encodeOperand(tokens[1], tokens[4]);
            encodeOperand(tokens[2], tokens[5]);
            encodeOperand(tokens[3], tokens[6]);
        }

        else if (general2Operands.count(tokens[0])) {
            const auto& opcode = general2Operands.at(tokens[0]);
            for (auto b : opcode) output.push_back(b);
            output.push_back(descriptorByte(tokens[1]));
            output.push_back(descriptorByte(tokens[2]));
            encodeOperand(tokens[1], tokens[3]);
            encodeOperand(tokens[2], tokens[4]);
        }

        else if (tokens[0] == "mval64plus") {
            output.push_back(0x14);
            output.push_back(descriptorByte(tokens[1]));
            output.push_back(descriptorByte(tokens[2]));
            encodeOperand(tokens[1], tokens[3]);

            if (tokens[4] == "auto") {
                auto bytes = autoSizeBytes(tokens[5]);
                uint16_t size = bytes.size();
                output.push_back(size & 0xFF);
                output.push_back((size >> 8) & 0xFF);
                for (auto b : bytes) output.push_back(b);

            } else if (tokens[4] == "set") {
                uint16_t size = (uint16_t)std::stoull(tokens[5], nullptr, 0) / 8;
                output.push_back(size & 0xFF);
                output.push_back((size >> 8) & 0xFF);
                auto bytes = autoSizeBytes(tokens[6]);
                bytes.resize(size, 0);
                for (auto b : bytes) output.push_back(b);
            }
        }

        else if (tokens[0] == "mvalstr") {
            output.push_back(0x14);
            output.push_back(descriptorByte(tokens[1]));
            output.push_back(descriptorByte(tokens[2]));

            encodeOperand(tokens[1], tokens[3]);

            std::string str = tokens[4];
            uint16_t size = str.size();
            output.push_back(size & 0xFF);
            output.push_back((size >> 8) & 0xFF);
            for (char c : str) output.push_back((uint8_t)c);
        }

        else if (generalJumps.count(tokens[0])) {
            output.push_back(generalJumps.at(tokens[0]));
            output.push_back(descriptorByte(tokens[1]));
            if (tokens[2][0] == '!') {
                std::string name = tokens[2].substr(1);
                uint64_t addr = 0;
                for (const auto& l : labels)
                    if (l.name == name) { addr = l.address; break; }
                for (int b = 0; b < 8; b++) output.push_back((addr >> (b * 8)) & 0xFF);
            } else {
                encodeOperand(tokens[1], tokens[2]);
            }
        }

        else if (general4Operands.count(tokens[0])) {
            output.push_back(general4Operands.at(tokens[0]));
            output.push_back(descriptorByte(tokens[1]));
            output.push_back(descriptorByte(tokens[2]));
            output.push_back(descriptorByte(tokens[3]));
            output.push_back(descriptorByte(tokens[4]));
            encodeOperand(tokens[1], tokens[5]);
            encodeOperand(tokens[2], tokens[6]);
            encodeOperand(tokens[3], tokens[7]);
            encodeOperand(tokens[4], tokens[8]);
        }
    }
}

void Assembler::writeOutput(const std::string& filename) {
    std::ofstream f(filename, std::ios::binary | std::ios::trunc);
    if (!f) error("ASM00007", filename);
    f.write(reinterpret_cast<const char*>(output.data()), output.size());
    f.close();
}

int main() {
    std::string inputFile, outputFile;

    std::cout << "Enter assembly file path: ";
    std::getline(std::cin, inputFile);
    if (inputFile.empty()) inputFile = "asmFile.trasm";

    std::cout << "Enter output file path: ";
    std::getline(std::cin, outputFile);
    if (outputFile.empty()) outputFile = "rom.bin";

    std::vector<std::string> lines;
    try {
        lines = assembler.readFile(inputFile);
    } catch (const std::exception& e) {
        assembler.error("ASM00001", e.what());
    }

    std::cout << "Assembling...\n";

    assembler.secondPass(lines);
    const size_t ROM_SIZE = 32 * 1024;
    while (assembler.output.size() < ROM_SIZE)
        assembler.output.push_back(0x00);

    assembler.writeOutput(outputFile);
    std::cout << "Done! Written to " << outputFile << " (padded to 32KB)\n";

    std::cout << "Done! " << assembler.output.size() << " bytes written to " << outputFile << "\n";
    std::cin.get();
    return 0;
}