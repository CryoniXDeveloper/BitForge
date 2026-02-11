#include "cpu.h"
#include "motherboard.h"
#include "rom.h"
#include "ram.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <iomanip>
#include <Windows.h>

void CPU::start() {
    running = true;
    sleepCounter = 0;

    timer.start();

    while (running) {
        if (sleepCounter == 0) {
            setFlagBit(FLAG_SLEEP, false);
        }

        if (getFlagBit(FLAG_SLEEP)) {
            sleepCounter--;
            instructionCounter++;
            continue;
        }
        fetch();
        decode();
        execute();
        instructionCounter++;
    }
 
    CPURunTime = timer.end();

    std::cout << std::fixed << std::setprecision(10)
          << "CPU Finished in " << instructionCounter
          << " cycles in " << CPURunTime << " seconds.\n";

    std::cin.get();
}

void CPU::fetch() {
    opcode = read8(instructionPointer);
    instructionSize = 0;

    if (opcode == 0x00 || opcode >= 0x10) {
        instructionPointer++;
        return;
    }

    else if (opcode >= 0x01 && opcode <= 0x0F) {
        instructionPointer++;

        uint8_t secondByte = read8(instructionPointer);
        opcode = (opcode << 8) | secondByte;

        instructionPointer++;
        return;
    }
}

void CPU::decode() {
    operands8.clear();
    operands16.clear();
    operands32.clear();
    operands64.clear();
    operandsVector.clear();

    op1Type = "";
    op2Type = "";
    op1Size = 0;
    op2Size = 0;

    switch (opcode) {
        case 0x00:
            break;

        case 0x10:
            mnemonicType = read8(instructionPointer);

            op1Type = mnemonicMoveOperand1Types.at(mnemonicType);
            op1Size = mnemonicMoveOperand1Bytes.at(mnemonicType);
            
            op2Type = mnemonicMoveOperand2Types.at(mnemonicType);
            op2Size = mnemonicMoveOperand2Bytes.at(mnemonicType);

            instructionPointer++;

            switch (op1Size) {
                case 1:
                    operands8.push_back(read8(instructionPointer));
                    instructionSize++;

                case 2:
                    operands16.push_back(read16(instructionPointer));
                    instructionSize += 2;

                case 4:
                    operands32.push_back(read32(instructionPointer));
                    instructionSize += 4;

                case 8:
                    operands64.push_back(read64(instructionPointer));
                    instructionSize += 8;
            }

            instructionPointer += instructionSize;
            instructionSize = 0;

            switch (op2Size) {
                case 1:
                    operands8.push_back(read8(instructionPointer));
                    instructionSize++;

                case 2:
                    operands16.push_back(read16(instructionPointer));
                    instructionSize += 2;

                case 4:
                    operands32.push_back(read32(instructionPointer));
                    instructionSize += 4;

                case 8:
                    operands64.push_back(read64(instructionPointer));
                    instructionSize += 8;
            }

            instructionPointer += instructionSize;
            instructionSize = 0;
            break;

        case 0xFD:
            break;

        case 0xFE:
            break;

        case 0xFF:
            break;
    }
}

void CPU::execute() {
    switch (opcode) {
        case 0x00:
            break;

        case 0x10:
            resetOpIndexes();

            if (op1Type == "r") {
            }
            else if (op1Type == "mi") {
            }
            else if (op1Type == "mr") {
                }

        case 0xFD:
            running = false;
            break;

        case 0xFE:
            break;

        case 0xFF:
            break;
    }
}

uint8_t CPU::read8(uint64_t address) {
    if (address >= Motherboard::ROM_START && address <= Motherboard::ROM_END) {
        return rom->read8(address);
    }
    
    else if (address >= Motherboard::RAM_START && address <= Motherboard::RAM_END) {
        return memory->read8(address);
    }

    else {
        error("CP03AOOB", "Absolute address: " + std::to_string(address));
    }
}

uint16_t CPU::read16(uint64_t address) {
    if (address >= Motherboard::ROM_START && address <= Motherboard::ROM_END) {
        return rom->read16(address);
    }
    
    else if (address >= Motherboard::RAM_START && address <= Motherboard::RAM_END) {
        return memory->read16(address);
    }
    
    else {
        error("CP04AOOB", "Absolute address: " + std::to_string(address));
    }
}

uint32_t CPU::read32(uint64_t address) {
    if (address >= Motherboard::ROM_START && address <= Motherboard::ROM_END) {
        return rom->read32(address);
    }
    
    else if (address >= Motherboard::RAM_START && address <= Motherboard::RAM_END) {
        return memory->read32(address);
    }
    
    else {
        error("CP05AOOB", "Absolute address: " + std::to_string(address));
    }
}

uint64_t CPU::read64(uint64_t address) {
    if (address >= Motherboard::ROM_START && address <= Motherboard::ROM_END) {
        return rom->read64(address);
    }
    
    else if (address >= Motherboard::RAM_START && address <= Motherboard::RAM_END) {
        return memory->read64(address);
    }
    
    else {
        error("CP06AOOB", "Absolute address: " + std::to_string(address));
    }
}

std::vector<uint8_t> CPU::readBytesVector(uint64_t start, size_t length) {
    if (start >= Motherboard::ROM_START && start <= Motherboard::ROM_END) {
        return rom->readBytesVector(start, length);
    }
    
    else if (start >= Motherboard::RAM_START && start <= Motherboard::RAM_END) {
        return memory->readBytesVector(start, length);
    }
    
    else {
        error("CP07AOOB", "Absolute address: " + std::to_string(start));
    }
}

void CPU::write8(uint64_t address, uint8_t value) {
    if (address >= Motherboard::ROM_START && address <= Motherboard::ROM_END) {
        error("CP01CWTR", "Absolute address: " + std::to_string(address));
    }
    
    else if (address >= Motherboard::RAM_START && address <= Motherboard::RAM_END) {
        memory->write8(address, value);
    }
    
    else {
        error("CP08AOOB", "Absolute address: " + std::to_string(address));
    }
}

void CPU::writeBytesVector(uint64_t start, const std::vector<uint8_t>& data) {
    if (start >= Motherboard::ROM_START && start <= Motherboard::ROM_END) {
        error("CP02CWTR", "Absolute address: " + std::to_string(start));
    }
    
    else if (start >= Motherboard::RAM_START && start <= Motherboard::RAM_END) {
        memory->writeBytesVector(start, data);
    }
    
    else {
        error("CP09AOOB", "Absolute address: " + std::to_string(start));
    }
}

void CPU::error(std::string errorType, std::string info) const {
    std::string returnString = "ERROR [" + errorType + "]";
    if (info != "")
        returnString += " - More info: " + info;
    returnString += "\n\nFind out more in errorTypes.txt.\nStopping execution...";
    std::cout << returnString;
    std::cin.get();
    exit(0);
}