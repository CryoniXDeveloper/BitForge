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

    op1Size = 0;
    op2Size = 0;

    switch (opcode) {
        case 0x00:
            break;

        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
        case 0x14:
            mnemonicType1 = read8(instructionPointer);
            instructionPointer++;
            mnemonicType2 = read8(instructionPointer);
            instructionPointer++;

            op1Type = mnemonicMoveOperandTypes[mnemonicType1];
            op1Size = mnemonicMoveOperandBytes[mnemonicType1];
            
            op2Type = mnemonicMoveOperandTypes[mnemonicType2];
            op2Size = mnemonicMoveOperandBytes[mnemonicType2];

            if (opcode == 0x14) {
                if (op2Size == 0) {
                    op2Size = read16(instructionPointer);
                }
                
                else {
                    op2VectorSize = read16(instructionPointer);
                }

                instructionPointer += 2;
            }

            if (op1Size == 1) {
                operands8.push_back(read8(instructionPointer));

            } else if (op1Size == 2) {
                operands16.push_back(read16(instructionPointer));

            } else if (op1Size == 4) {
                operands32.push_back(read32(instructionPointer));

            } else if (op1Size == 8) {
                operands64.push_back(read64(instructionPointer));
            }

            instructionPointer += op1Size;

            if (op2Size == 1) {
                operands8.push_back(read8(instructionPointer));

            } else if (op2Size == 2) {
                operands16.push_back(read16(instructionPointer));

            } else if (op2Size == 4) {
                operands32.push_back(read32(instructionPointer));

            } else if (op2Size == 8) {
                operands64.push_back(read64(instructionPointer));

            } else {
                operandsVector.push_back(readBytesVector(instructionPointer, op2Size));
            }

            instructionPointer += op2Size;
            break;

        case 0xFD:
            break;

        case 0xFE:
            mnemonicType1 = read8(instructionPointer);
            instructionPointer++;

            op1Type = mnemonicMoveOperandTypes[mnemonicType1];
            op1Size = mnemonicMoveOperandBytes[mnemonicType1];

            if (op1Size == 1) {
                operands8.push_back(read8(instructionPointer));

            } else if (op1Size == 2) {
                operands16.push_back(read16(instructionPointer));

            } else if (op1Size == 4) {
                operands32.push_back(read32(instructionPointer));

            } else if (op1Size == 8) {
                operands64.push_back(read64(instructionPointer));

            } else {
                operandsVector.push_back(readBytesVector(instructionPointer, op1Size));
            }

            instructionPointer += op1Size;
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
            
            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index]; op8Index++;
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index]; op8Index++; break;
                        case 2: dest = operands16[op16Index]; op16Index++; break;
                        case 4: dest = operands32[op32Index]; op32Index++; break;
                        case 8: dest = operands64[op64Index]; op64Index++; break;
                    }

                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index]]; op8Index++; break;
                        case 2: dest = registers[operands16[op16Index]]; op16Index++; break;
                        case 4: dest = registers[operands32[op32Index]]; op32Index++; break;
                        case 8: dest = registers[operands64[op64Index]]; op64Index++; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value = registers[operands8[op8Index]] & 0xFF;
                    break;

                case imm:
                    value = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value = read8(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value = read8(addr);
                    break;
            }

            switch (op1Type) {
                case reg:
                    registers[dest] = value;
                    break;

                case mem_imm:
                case mem_reg:
                    write8(dest, value);
                    break;
            }

            break;

        case 0x11:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index]; op8Index++;
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index]; op8Index++; break;
                        case 2: dest = operands16[op16Index]; op16Index++; break;
                        case 4: dest = operands32[op32Index]; op32Index++; break;
                        case 8: dest = operands64[op64Index]; op64Index++; break;
                    }

                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index]]; op8Index++; break;
                        case 2: dest = registers[operands16[op16Index]]; op16Index++; break;
                        case 4: dest = registers[operands32[op32Index]]; op32Index++; break;
                        case 8: dest = registers[operands64[op64Index]]; op64Index++; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value = registers[operands8[op8Index]] & 0xFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value = operands8[op8Index]; break;
                        case 2: value = operands16[op16Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value = read16(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value = read16(addr);
                    break;
            }

            switch (op1Type) {
                case reg:
                    registers[dest] = value;
                    break;

                case mem_imm:
                case mem_reg:
                    write16(dest, value);
                    break;
            }

            break;

        case 0x12:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index]; op8Index++;
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index]; op8Index++; break;
                        case 2: dest = operands16[op16Index]; op16Index++; break;
                        case 4: dest = operands32[op32Index]; op32Index++; break;
                        case 8: dest = operands64[op64Index]; op64Index++; break;
                    }

                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index]]; op8Index++; break;
                        case 2: dest = registers[operands16[op16Index]]; op16Index++; break;
                        case 4: dest = registers[operands32[op32Index]]; op32Index++; break;
                        case 8: dest = registers[operands64[op64Index]]; op64Index++; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value = registers[operands8[op8Index]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value = operands8[op8Index]; break;
                        case 2: value = operands16[op16Index]; break;
                        case 4: value = operands32[op32Index]; break;
                    }

                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value = read32(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value = read32(addr);
                    break;
            }

            switch (op1Type) {
                case reg:
                    registers[dest] = value;
                    break;

                case mem_imm:
                case mem_reg:
                    write32(dest, value);
                    break;
            }

            break;

        case 0x13:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index]; op8Index++;
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index]; op8Index++; break;
                        case 2: dest = operands16[op16Index]; op16Index++; break;
                        case 4: dest = operands32[op32Index]; op32Index++; break;
                        case 8: dest = operands64[op64Index]; op64Index++; break;
                    }

                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index]]; op8Index++; break;
                        case 2: dest = registers[operands16[op16Index]]; op16Index++; break;
                        case 4: dest = registers[operands32[op32Index]]; op32Index++; break;
                        case 8: dest = registers[operands64[op64Index]]; op64Index++; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value = operands8[op8Index]; break;
                        case 2: value = operands16[op16Index]; break;
                        case 4: value = operands32[op32Index]; break;
                        case 8: value = operands64[op64Index]; break;
                    }

                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value = read64(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value = read64(addr);
                    break;
            }

            switch (op1Type) {
                case reg:
                    registers[dest] = value;
                    break;

                case mem_imm:
                case mem_reg:
                    write64(dest, value);
                    break;
            }

            break;

        case 0x14:
            resetOpIndexes();

            switch (op1Type) {
                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index]; op8Index++; break;
                        case 2: dest = operands16[op16Index]; op16Index++; break;
                        case 4: dest = operands32[op32Index]; op32Index++; break;
                        case 8: dest = operands64[op64Index]; op64Index++; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index]]; op8Index++; break;
                        case 2: dest = registers[operands16[op16Index]]; op16Index++; break;
                        case 4: dest = registers[operands32[op32Index]]; op32Index++; break;
                        case 8: dest = registers[operands64[op64Index]]; op64Index++; break;
                    }
                    break;
            }

            switch (op2Type) {
                case imm:
                    VectorOfBytesPointer = &operandsVector[opVectorIndex]; opVectorIndex++; break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    VectorOfBytes = readBytesVector(addr, op2VectorSize);
                    VectorOfBytesPointer = &VectorOfBytes;
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    VectorOfBytes = readBytesVector(addr, op2VectorSize);
                    VectorOfBytesPointer = &VectorOfBytes;
                    break;
            }

            switch (op1Type) {
                case mem_imm:
                case mem_reg:
                    writeBytesVector(dest, *VectorOfBytesPointer);
                    break;
            }

            break;

        case 0xFD:
            running = false;
            break;

        case 0xFE:
            resetOpIndexes();
            
            switch (op1Type) {
                case reg:
                    value = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op1Size) {
                        case 1: value = operands8[op8Index]; break;
                        case 2: value = operands16[op16Index]; break;
                        case 4: value = operands32[op32Index]; break;
                        case 8: value = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value = read64(addr);
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value = read64(addr);
                    break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(value));
            break;

        case 0xFF:
            resetOpIndexes();
            
            switch (op1Type) {
                case reg:
                    value = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op1Size) {
                        case 1: value = operands8[op8Index]; break;
                        case 2: value = operands16[op16Index]; break;
                        case 4: value = operands32[op32Index]; break;
                        case 8: value = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value = read64(addr);
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value = read64(addr);
                    break;
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(value));
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
}

uint16_t CPU::read16(uint64_t address) {
    if (address >= Motherboard::ROM_START && address <= Motherboard::ROM_END) {
        return rom->read16(address);
    }
    
    else if (address >= Motherboard::RAM_START && address <= Motherboard::RAM_END) {
        return memory->read16(address);
    }
}

uint32_t CPU::read32(uint64_t address) {
    if (address >= Motherboard::ROM_START && address <= Motherboard::ROM_END) {
        return rom->read32(address);
    }
    
    else if (address >= Motherboard::RAM_START && address <= Motherboard::RAM_END) {
        return memory->read32(address);
    }
}

uint64_t CPU::read64(uint64_t address) {
    if (address >= Motherboard::ROM_START && address <= Motherboard::ROM_END) {
        return rom->read64(address);
    }
    
    else if (address >= Motherboard::RAM_START && address <= Motherboard::RAM_END) {
        return memory->read64(address);
    }
}

std::vector<uint8_t> CPU::readBytesVector(uint64_t start, size_t length) {
    if (start >= Motherboard::ROM_START && start <= Motherboard::ROM_END) {
        return rom->readBytesVector(start, length);
    }
    
    else if (start >= Motherboard::RAM_START && start <= Motherboard::RAM_END) {
        return memory->readBytesVector(start, length);
    }
}

void CPU::write8(uint64_t address, uint8_t value) {
    if (address >= Motherboard::ROM_START && address <= Motherboard::ROM_END) {
        error("CP01CWTR", "Absolute address: " + std::to_string(address));
    }
    
    else if (address >= Motherboard::RAM_START && address <= Motherboard::RAM_END) {
        memory->write8(address, value);
    }
}

void CPU::write16(uint64_t address, uint16_t value) {
    if (address >= Motherboard::ROM_START && address <= Motherboard::ROM_END) {
        error("CP01CWTR", "Absolute address: " + std::to_string(address));
    }
    
    else if (address >= Motherboard::RAM_START && address <= Motherboard::RAM_END) {
        memory->write16(address, value);
    }
}

void CPU::write32(uint64_t address, uint32_t value) {
    if (address >= Motherboard::ROM_START && address <= Motherboard::ROM_END) {
        error("CP01CWTR", "Absolute address: " + std::to_string(address));
    }
    
    else if (address >= Motherboard::RAM_START && address <= Motherboard::RAM_END) {
        memory->write32(address, value);
    }
}

void CPU::write64(uint64_t address, uint64_t value) {
    if (address >= Motherboard::ROM_START && address <= Motherboard::ROM_END) {
        error("CP01CWTR", "Absolute address: " + std::to_string(address));
    }
    
    else if (address >= Motherboard::RAM_START && address <= Motherboard::RAM_END) {
        memory->write64(address, value);
    }
}

void CPU::writeBytesVector(uint64_t start, const std::vector<uint8_t>& data) {
    if (start >= Motherboard::ROM_START && start <= Motherboard::ROM_END) {
        error("CP02CWTR", "Absolute address: " + std::to_string(start));
    }
    
    else if (start >= Motherboard::RAM_START && start <= Motherboard::RAM_END) {
        memory->writeBytesVector(start, data);
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