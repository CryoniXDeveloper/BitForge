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
#include <intrin.h>
#include <immintrin.h>

void CPU::start() {
    size_t STACK_SIZE = Motherboard::STACK_SIZE;
    size_t STACK_START = Motherboard::STACK_START;
    size_t STACK_END = Motherboard::STACK_END;
    stackPointer = STACK_START;
    running = true;
    sleepCounter = 0;

    for (int i = 0; i < 1000; i++) {
        warmup = rom->read8(Motherboard::ROM_START + (i % Motherboard::ROM_SIZE));
    }

    timer.start();

    while (running) {
        if (sleepCounter > 0) {
            sleepCounter--;
        } else {
            fetch();
            decode();
            execute();
        }
        instructionCounter++;
        cycles++;
    }
 
    CPURunTime = timer.end();

    std::cout << std::fixed << std::setprecision(10)
          << "CPU Finished in " << cycles
          << " cycles in " << CPURunTime << " seconds.\n";

    std::cout << "CPS: " << (uint64_t)(cycles / CPURunTime) << "\n";

    std::cout << "\n=== FLAGS ===\n";
    std::cout << "CARRY:      " << getFlagBit(FLAG_CARRY)      << "\n";
    std::cout << "ZERO:       " << getFlagBit(FLAG_ZERO)       << "\n";
    std::cout << "NEGATIVE:   " << getFlagBit(FLAG_NEGATIVE)   << "\n";
    std::cout << "OVERFLOW:   " << getFlagBit(FLAG_OVERFLOW)   << "\n";
    std::cout << "INTERRUPT:  " << getFlagBit(FLAG_INTERRUPT)  << "\n";
    std::cout << "SLEEP:      " << getFlagBit(FLAG_SLEEP)      << "\n";
    std::cout << "DIRECTION:  " << getFlagBit(FLAG_DIRECTION)  << "\n";
    std::cout << "SUPERVISOR: " << getFlagBit(FLAG_SUPERVISOR) << "\n";

    std::cout << "\n=== REGISTER DUMP ===\n";

    for (size_t i = 0; i < 64; i++) {
        std::cout 
            << "R" << std::setw(2) << std::setfill('0') << i << ": 0x"
            << std::hex << std::setw(16) << std::setfill('0') 
            << registers[i]
            << std::dec << "\n";
    }

    std::cout << "\nEnter memory start address (hex like 0x100 or decimal): ";
    std::string addrStr;
    std::cin >> addrStr;

    uint64_t address = 0;

    try {
        address = std::stoull(addrStr, nullptr, 0);
    } catch (...) {
        error("CP_MEM", "Invalid address input");
    }

    std::cout << "\n=== MEMORY DUMP ===\n";

    for (int i = 0; i < 100; i++) {
        uint64_t currentAddr = address + i;

        uint8_t value = read8(currentAddr);

        if (i % 16 == 0) {
            std::cout << "\n0x"
                    << std::hex << std::setw(8) << std::setfill('0')
                    << currentAddr << ": ";
        }

        std::cout << std::hex << std::setw(2) << std::setfill('0')
                << (int)value << " ";
    }

    std::cout << std::dec << "\n";

    std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
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
    resetOpIndexes();

    op1Size = 0;
    op2Size = 0;
    op3Size = 0;

    switch (opcode) {
        case 0x00: case 0xAF: case 0xB8:
        case 0xB9: case 0xBA: case 0xBB:
            break;

        case 0x10: case 0x11: case 0x12: case 0x13:
        case 0x49: case 0x4A: case 0x4B: case 0x4C:
        case 0x4D: case 0x4E: case 0x4F: case 0x50:
        case 0x51: case 0x52: case 0x53: case 0x54:
        case 0x81: case 0x82: case 0x83: case 0x84:
        case 0x0114: case 0x0115: case 0x0116: case 0x0117:
        case 0x0118: case 0x0119: case 0x011A: case 0x011B:
        case 0x011C: case 0x011D: case 0x011E: case 0x011F:
        case 0x0120: case 0x0121: case 0x0122: case 0x0123:
        case 0x0124: case 0x0125: case 0x0126: case 0x0127:
        case 0xAD:
            mnemonicType1 = read8(instructionPointer);
            instructionPointer++;
            mnemonicType2 = read8(instructionPointer);
            instructionPointer++;

            op1Type = mnemonicMoveOperandTypes[mnemonicType1];
            op1Size = mnemonicMoveOperandBytes[mnemonicType1];

            op2Type = mnemonicMoveOperandTypes[mnemonicType2];
            op2Size = mnemonicMoveOperandBytes[mnemonicType2];
            op2VectorSize = 0;

            if (op1Size == 1) {
                operands8[op8Index++] = read8(instructionPointer);

            } else if (op1Size == 2) {
                operands16[op16Index++] = read16(instructionPointer);

            } else if (op1Size == 4) {
                operands32[op32Index++] = read32(instructionPointer);

            } else if (op1Size == 8) {
                operands64[op64Index++] = read64(instructionPointer);
            }

            instructionPointer += op1Size;

            if (op2Size == 1) {
                operands8[op8Index++] = read8(instructionPointer);

            } else if (op2Size == 2) {
                operands16[op16Index++] = read16(instructionPointer);

            } else if (op2Size == 4) {
                operands32[op32Index++] = read32(instructionPointer);

            } else if (op2Size == 8) {
                operands64[op64Index++] = read64(instructionPointer);

            } else {
                operandsVector.push_back(readBytesVector(instructionPointer, op2Size));
            }

            instructionPointer += op2Size + op2VectorSize;
            break;

        case 0x14:
            mnemonicType1 = read8(instructionPointer);
            instructionPointer++;
            mnemonicType2 = read8(instructionPointer);
            instructionPointer++;

            op1Type = mnemonicMoveOperandTypes[mnemonicType1];
            op1Size = mnemonicMoveOperandBytes[mnemonicType1];

            op2Type = mnemonicMoveOperandTypes[mnemonicType2];
            op2Size = mnemonicMoveOperandBytes[mnemonicType2];
            op2VectorSize = 0;

            if (op1Size == 1) {
                operands8[op8Index++] = read8(instructionPointer);

            } else if (op1Size == 2) {
                operands16[op16Index++] = read16(instructionPointer);

            } else if (op1Size == 4) {
                operands32[op32Index++] = read32(instructionPointer);

            } else if (op1Size == 8) {
                operands64[op64Index++] = read64(instructionPointer);
            }

            instructionPointer += op1Size;

            operandsVector.clear();

            if (op2Size == 0) {
                op2Size = read16(instructionPointer);
            }

            else {
                op2VectorSize = read16(instructionPointer);
            }

            instructionPointer += 2;

            if (op2Size == 1) {
                operands8[op8Index++] = read8(instructionPointer);

            } else if (op2Size == 2) {
                operands16[op16Index++] = read16(instructionPointer);

            } else if (op2Size == 4) {
                operands32[op32Index++] = read32(instructionPointer);

            } else if (op2Size == 8) {
                operands64[op64Index++] = read64(instructionPointer);

            } else {
                operandsVector.push_back(readBytesVector(instructionPointer, op2Size));
            }

            instructionPointer += op2Size + op2VectorSize;
            break;

        case 0x15: case 0x16: case 0x17: case 0x18: case 0x19: case 0x1A:
        case 0x1B: case 0x1C: case 0x1D: case 0x1E: case 0x1F: case 0x20:
        case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26:
        case 0x27: case 0x28: case 0x29: case 0x2A: case 0x2B: case 0x2C:
        case 0x2D: case 0x2E: case 0x2F: case 0x30: case 0x31: case 0x32:
        case 0x33: case 0x34: case 0x3D: case 0x3E: case 0x3F: case 0x40:
        case 0x41: case 0x42: case 0x43: case 0x44:
        case 0x55: case 0x56: case 0x57: case 0x58:
        case 0x59: case 0x5A: case 0x5B: case 0x5C:
        case 0x5D: case 0x5E: case 0x5F: case 0x60:
        case 0x65: case 0x66: case 0x67: case 0x68:
        case 0x69: case 0x6A: case 0x6B: case 0x6C:
        case 0x6D: case 0x6E: case 0x6F: case 0x70:
        case 0x71: case 0x72: case 0x73: case 0x74:
        case 0x75: case 0x76: case 0x77: case 0x78:
        case 0x79: case 0x7A: case 0x7B: case 0x7C:
        case 0x7D: case 0x7E: case 0x7F: case 0x80:
        case 0x0100: case 0x0101: case 0x0102: case 0x0103:
        case 0x0104: case 0x0105: case 0x0106: case 0x0107:
        case 0x0108: case 0x0109: case 0x010A: case 0x010B:
        case 0x010C: case 0x010D: case 0x010E: case 0x010F:
        case 0x0110: case 0x0111: case 0x0112: case 0x0113:
        case 0x88: case 0x89: case 0x8A: case 0x8B:
        case 0x8C: case 0x8D: case 0x8E: case 0x8F:
        case 0x90: case 0x91: case 0x92: case 0x93:
        case 0x94: case 0x95: case 0x96: case 0x97:
            mnemonicType1 = read8(instructionPointer);
            instructionPointer++;
            mnemonicType2 = read8(instructionPointer);
            instructionPointer++;
            mnemonicType3 = read8(instructionPointer);
            instructionPointer++;

            op1Type = mnemonicMoveOperandTypes[mnemonicType1];
            op1Size = mnemonicMoveOperandBytes[mnemonicType1];

            op2Type = mnemonicMoveOperandTypes[mnemonicType2];
            op2Size = mnemonicMoveOperandBytes[mnemonicType2];

            op3Type = mnemonicMoveOperandTypes[mnemonicType3];
            op3Size = mnemonicMoveOperandBytes[mnemonicType3];

            if (op1Size == 1) {
                operands8[op8Index++] = read8(instructionPointer);

            } else if (op1Size == 2) {
                operands16[op16Index++] = read16(instructionPointer);

            } else if (op1Size == 4) {
                operands32[op32Index++] = read32(instructionPointer);

            } else if (op1Size == 8) {
                operands64[op64Index++] = read64(instructionPointer);
            }

            instructionPointer += op1Size;

            if (op2Size == 1) {
                operands8[op8Index++] = read8(instructionPointer);

            } else if (op2Size == 2) {
                operands16[op16Index++] = read16(instructionPointer);

            } else if (op2Size == 4) {
                operands32[op32Index++] = read32(instructionPointer);

            } else if (op2Size == 8) {
                operands64[op64Index++] = read64(instructionPointer);
            }

            instructionPointer += op2Size;

            if (op3Size == 1) {
                operands8[op8Index++] = read8(instructionPointer);

            } else if (op3Size == 2) {
                operands16[op16Index++] = read16(instructionPointer);

            } else if (op3Size == 4) {
                operands32[op32Index++] = read32(instructionPointer);

            } else if (op3Size == 8) {
                operands64[op64Index++] = read64(instructionPointer);
            }

            instructionPointer += op3Size;
            break;

        case 0x35: case 0x36: case 0x37: case 0x38:
        case 0x39: case 0x3A: case 0x3B: case 0x3C:
        case 0x45: case 0x46: case 0x47: case 0x48:
        case 0x61: case 0x62: case 0x63: case 0x64:
        case 0x85: case 0x86: case 0x87:
        case 0x98: case 0x99: case 0x9A: case 0x9B:
        case 0x9C: case 0x9D: case 0x9E: case 0x9F:
        case 0xA0: case 0xA1: case 0xA2: case 0xA3:
        case 0xA4: case 0xA5: case 0xA6: case 0xA7:
        case 0xA8: case 0xA9: case 0xAA: case 0xAB:
        case 0xAC: case 0xAE: case 0xB0: case 0xB1:
        case 0xB2: case 0xB3: case 0xB4: case 0xB5:
        case 0xB6: case 0xB7: case 0xFE: case 0xFF:
            mnemonicType1 = read8(instructionPointer);
            instructionPointer++;

            op1Type = mnemonicMoveOperandTypes[mnemonicType1];
            op1Size = mnemonicMoveOperandBytes[mnemonicType1];

            if (op1Size == 1) {
                operands8[op8Index++] = read8(instructionPointer);

            } else if (op1Size == 2) {
                operands16[op16Index++] = read16(instructionPointer);

            } else if (op1Size == 4) {
                operands32[op32Index++] = read32(instructionPointer);

            } else if (op1Size == 8) {
                operands64[op64Index++] = read64(instructionPointer);
            }

            instructionPointer += op1Size;
            break;

        case 0xFD:
            running = false;
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
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
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
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
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
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
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
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
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
                case reg:
                    dest = operands8[op8Index]; op8Index++;
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
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
                case reg:
                    registersNeeded = (VectorOfBytesPointer->size() + 7) / 8;
                    for (int i = 0; i < registersNeeded; i++) {
                        value = 0;

                        for (int b = 0; b < 8; b++) {
                            int idx = i * 8 + b;
                            if (idx < VectorOfBytesPointer->size())
                                value |= (uint64_t)(*VectorOfBytesPointer)[idx] << (b * 8);
                        }
                        registers[dest + i] = value;
                    }

                    break;

                case mem_imm:
                case mem_reg:
                    writeBytesVector(dest, *VectorOfBytesPointer);

                    break;
            }

            break;

        case 0x15:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }

                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFF;
                    break;

                case imm:
                    value1 = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read8(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read8(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFF;
                    break;

                case imm:
                    value2 = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read8(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read8(addr);
                    break;
            }

            value = (value1 + value2) & 0xFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80);
            setFlagBit(FLAG_OVERFLOW, ((~(value1 ^ value2)) & (value1 ^ value) & 0x80) != 0);

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

        case 0x16:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }

                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFF;
                    break;

                case imm:
                    value1 = operands8[op8Index++];
                    break;
 
                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read8(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read8(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFF;
                    break;

                case imm:
                    value2 = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read8(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read8(addr);
                    break;
            }

            value = (value1 + value2) & 0xFF;
            setFlagBit(FLAG_ZERO,  value == 0);
            setFlagBit(FLAG_CARRY, value < (value1 & 0xFF));

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

        case 0x17:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }

                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                    }

                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read16(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read16(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                    }

                    break;       

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read16(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read16(addr);
                    break;
            }

            value = (value1 + value2) & 0xFFFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000);
            setFlagBit(FLAG_OVERFLOW, ((~(value1 ^ value2)) & (value1 ^ value) & 0x8000) != 0);

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

        case 0x18:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }

                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                    }

                    break; 
 
                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read16(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read16(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                    }

                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read16(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read16(addr);
                    break;
            }

            value = (value1 + value2) & 0xFFFF;
            setFlagBit(FLAG_ZERO,  value == 0);
            setFlagBit(FLAG_CARRY, value < (value1 & 0xFFFF));

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

        case 0x19:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read32(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read32(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read32(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read32(addr);
                    break;
            }

            value = (value1 + value2) & 0xFFFFFFFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80000000);
            setFlagBit(FLAG_OVERFLOW, ((~(value1 ^ value2)) & (value1 ^ value) & 0x80000000) != 0);

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

        case 0x1A:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read32(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read32(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read32(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read32(addr);
                    break;
            }

            value = (value1 + value2) & 0xFFFFFFFF;
            setFlagBit(FLAG_ZERO,  value == 0);
            setFlagBit(FLAG_CARRY, value < (value1 & 0xFFFFFFFF));

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

        case 0x1B:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]];
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                        case 8: value1 = operands64[op64Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read64(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read64(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                        case 8: value2 = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read64(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read64(addr);
                    break;
            }

            value = value1 + value2;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000000000000000ULL);
            setFlagBit(FLAG_OVERFLOW, ((~(value1 ^ value2)) & (value1 ^ value) & 0x8000000000000000ULL) != 0);

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

        case 0x1C:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]];
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                        case 8: value1 = operands64[op64Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read64(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read64(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                        case 8: value2 = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read64(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read64(addr);
                    break;
            }

            value = value1 + value2;
            setFlagBit(FLAG_ZERO,  value == 0);
            setFlagBit(FLAG_CARRY, value < value1);

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

        case 0x1D:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFF;
                    break;

                case imm:
                    value1 = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read8(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read8(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFF;
                    break;

                case imm:
                    value2 = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read8(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read8(addr);
                    break;
            }

            value = (value1 - value2) & 0xFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80);
            setFlagBit(FLAG_OVERFLOW, ((value1 ^ value2) & (value1 ^ value) & 0x80) != 0);

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

        case 0x1E:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFF;
                    break;

                case imm:
                    value1 = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read8(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read8(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFF;
                    break;

                case imm:
                    value2 = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read8(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read8(addr);
                    break;
            }

            value = (value1 - value2) & 0xFF;
            setFlagBit(FLAG_ZERO,  value == 0);
            setFlagBit(FLAG_CARRY, value1 < value2);

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

        case 0x1F:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read16(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read16(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read16(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read16(addr);
                    break;
            }

            value = (value1 - value2) & 0xFFFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000);
            setFlagBit(FLAG_OVERFLOW, ((value1 ^ value2) & (value1 ^ value) & 0x8000) != 0);

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

        case 0x20:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read16(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read16(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read16(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read16(addr);
                    break;
            }

            value = (value1 - value2) & 0xFFFF;
            setFlagBit(FLAG_ZERO,  value == 0);
            setFlagBit(FLAG_CARRY, value1 < value2);

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

        case 0x21:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read32(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read32(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read32(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read32(addr);
                    break;
            }

            value = (value1 - value2) & 0xFFFFFFFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80000000);
            setFlagBit(FLAG_OVERFLOW, ((value1 ^ value2) & (value1 ^ value) & 0x80000000) != 0);

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

        case 0x22:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read32(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read32(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read32(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read32(addr);
                    break;
            }

            value = (value1 - value2) & 0xFFFFFFFF;
            setFlagBit(FLAG_ZERO,  value == 0);
            setFlagBit(FLAG_CARRY, value1 < value2);

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

        case 0x23:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]];
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                        case 8: value1 = operands64[op64Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read64(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read64(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                        case 8: value2 = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read64(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read64(addr);
                    break;
            }

            value = value1 - value2;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000000000000000ULL);
            setFlagBit(FLAG_OVERFLOW, ((value1 ^ value2) & (value1 ^ value) & 0x8000000000000000ULL) != 0);

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

        case 0x24:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]];
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                        case 8: value1 = operands64[op64Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read64(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read64(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                        case 8: value2 = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read64(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read64(addr);
                    break;
            }

            value = value1 - value2;
            setFlagBit(FLAG_ZERO,  value == 0);
            setFlagBit(FLAG_CARRY, value1 < value2);

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

        case 0x25:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFF;
                    break;

                case imm:
                    value1 = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read8(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read8(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFF;
                    break;

                case imm:
                    value2 = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read8(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read8(addr);
                    break;
            }

            value = (int8_t)value1 * (int8_t)value2;
            setFlagBit(FLAG_ZERO,     (value & 0xFF) == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80);
            setFlagBit(FLAG_OVERFLOW, (int64_t)value < -128 || (int64_t)value > 127);
            value &= 0xFF;

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

        case 0x26:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFF;
                    break;

                case imm:
                    value1 = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read8(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read8(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFF;
                    break;

                case imm:
                    value2 = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read8(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read8(addr);
                    break;
            }

            value = value1 * value2;
            setFlagBit(FLAG_ZERO,  (value & 0xFF) == 0);
            setFlagBit(FLAG_CARRY, value > 0xFF);
            value &= 0xFF;

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

        case 0x27:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read16(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read16(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read16(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read16(addr);
                    break;
            }

            value = (int16_t)value1 * (int16_t)value2;
            setFlagBit(FLAG_ZERO,     (value & 0xFFFF) == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000);
            setFlagBit(FLAG_OVERFLOW, (int64_t)value < -32768 || (int64_t)value > 32767);
            value &= 0xFFFF;

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

        case 0x28:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read16(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read16(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read16(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read16(addr);
                    break;
            }

            value = value1 * value2;
            setFlagBit(FLAG_ZERO,  (value & 0xFFFF) == 0);
            setFlagBit(FLAG_CARRY, value > 0xFFFF);
            value &= 0xFFFF;

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

        case 0x29:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read32(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read32(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read32(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read32(addr);
                    break;
            }

            result64 = (int64_t)(int32_t)value1 * (int64_t)(int32_t)value2;
            setFlagBit(FLAG_ZERO,     (value & 0xFFFFFFFF) == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80000000);
            setFlagBit(FLAG_OVERFLOW, result64 < -2147483648LL || result64 > 2147483647LL);
            value = (uint64_t)(uint32_t)result64;

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

        case 0x2A:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read32(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read32(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read32(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read32(addr);
                    break;
            }

            value = value1 * value2;
            setFlagBit(FLAG_ZERO,  (value & 0xFFFFFFFF) == 0);
            setFlagBit(FLAG_CARRY, value > 0xFFFFFFFF);
            value &= 0xFFFFFFFF;

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

        case 0x2B:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]];
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                        case 8: value1 = operands64[op64Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read64(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read64(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                        case 8: value2 = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read64(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read64(addr);
                    break;
            }

            high64;
            low64 = _mul128((int64_t)value1, (int64_t)value2, &high64);
            value = (uint64_t)low64;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000000000000000ULL);
            setFlagBit(FLAG_OVERFLOW, high64 != ((int64_t)value >> 63));

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

        case 0x2C:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]];
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                        case 8: value1 = operands64[op64Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read64(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read64(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                        case 8: value2 = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read64(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read64(addr);
                    break;
            }

            value = value1 * value2;
            setFlagBit(FLAG_ZERO,  value == 0);
            setFlagBit(FLAG_CARRY, value2 != 0 && value / value2 != value1);

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

        case 0x2D:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFF;
                    break;

                case imm:
                    value1 = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read8(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read8(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFF;
                    break;

                case imm:
                    value2 = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read8(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read8(addr);
                    break;
            }

            value = (int8_t)value1 / (int8_t)value2;
            setFlagBit(FLAG_ZERO,     (value & 0xFF) == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80);
            setFlagBit(FLAG_OVERFLOW, (int8_t)value1 == -128 && (int8_t)value2 == -1);
            value &= 0xFF;

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

        case 0x2E:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFF;
                    break;

                case imm:
                    value1 = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read8(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read8(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFF;
                    break;

                case imm:
                    value2 = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read8(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read8(addr);
                    break;
            }

            value = (value1 & 0xFF) / (value2 & 0xFF);
            setFlagBit(FLAG_ZERO, (value & 0xFF) == 0);
            value &= 0xFF;

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

        case 0x2F:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read16(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read16(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read16(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read16(addr);
                    break;
            }

            value = (int16_t)value1 / (int16_t)value2;
            setFlagBit(FLAG_ZERO,     (value & 0xFFFF) == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000);
            setFlagBit(FLAG_OVERFLOW, (int16_t)value1 == -32768 && (int16_t)value2 == -1);
            value &= 0xFFFF;

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

        case 0x30:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read16(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read16(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read16(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read16(addr);
                    break;
            }

            value = (value1 & 0xFFFF) / (value2 & 0xFFFF);
            setFlagBit(FLAG_ZERO, (value & 0xFFFF) == 0);
            value &= 0xFFFF;

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

        case 0x31:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read32(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read32(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read32(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read32(addr);
                    break;
            }

            value = (int32_t)value1 / (int32_t)value2;
            setFlagBit(FLAG_ZERO,     (value & 0xFFFFFFFF) == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80000000);
            setFlagBit(FLAG_OVERFLOW, (int32_t)value1 == INT32_MIN && (int32_t)value2 == -1);
            value &= 0xFFFFFFFF;

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

        case 0x32:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read32(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read32(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read32(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read32(addr);
                    break;
            }

            value = (value1 & 0xFFFFFFFF) / (value2 & 0xFFFFFFFF);
            setFlagBit(FLAG_ZERO, (value & 0xFFFFFFFF) == 0);
            value &= 0xFFFFFFFF;

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

        case 0x33:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]];
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                        case 8: value1 = operands64[op64Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read64(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read64(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                        case 8: value2 = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read64(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read64(addr);
                    break;
            }

            value = (int64_t)value1 / (int64_t)value2;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000000000000000ULL);
            setFlagBit(FLAG_OVERFLOW, (int64_t)value1 == INT64_MIN && (int64_t)value2 == -1);

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

        case 0x34:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]];
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                        case 8: value1 = operands64[op64Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read64(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read64(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                        case 8: value2 = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read64(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read64(addr);
                    break;
            }

            value = value1 / value2;
            setFlagBit(FLAG_ZERO, value == 0);

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

        case 0x35:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index];
                    registers[dest] = (registers[dest] & ~0xFFULL) | ((registers[dest] + 1) & 0xFF);
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index]; break;
                        case 2: dest = operands16[op16Index]; break;
                        case 4: dest = operands32[op32Index]; break;
                        case 8: dest = operands64[op64Index]; break;
                    }

                    write8(dest, read8(dest) + 1);
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index]]; break;
                        case 2: dest = registers[operands16[op16Index]]; break;
                        case 4: dest = registers[operands32[op32Index]]; break;
                        case 8: dest = registers[operands64[op64Index]]; break;
                    }
                    
                    write8(dest, read8(dest) + 1);
                    break;
            }

            break;

        case 0x36:
            resetOpIndexes();
            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index];
                    registers[dest] = (registers[dest] & ~0xFFFFULL) | ((registers[dest] + 1) & 0xFFFF);
                    break;
                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index]; break;
                        case 2: dest = operands16[op16Index]; break;
                        case 4: dest = operands32[op32Index]; break;
                        case 8: dest = operands64[op64Index]; break;
                    }
                    write16(dest, read16(dest) + 1);
                    break;
                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index]]; break;
                        case 2: dest = registers[operands16[op16Index]]; break;
                        case 4: dest = registers[operands32[op32Index]]; break;
                        case 8: dest = registers[operands64[op64Index]]; break;
                    }
                    write16(dest, read16(dest) + 1);
                    break;
            }
            break;

        case 0x37:
            resetOpIndexes();
            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index];
                    registers[dest] = (registers[dest] & ~0xFFFFFFFFULL) | ((registers[dest] + 1) & 0xFFFFFFFF);
                    break;
                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index]; break;
                        case 2: dest = operands16[op16Index]; break;
                        case 4: dest = operands32[op32Index]; break;
                        case 8: dest = operands64[op64Index]; break;
                    }
                    write32(dest, read32(dest) + 1);
                    break;
                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index]]; break;
                        case 2: dest = registers[operands16[op16Index]]; break;
                        case 4: dest = registers[operands32[op32Index]]; break;
                        case 8: dest = registers[operands64[op64Index]]; break;
                    }
                    write32(dest, read32(dest) + 1);
                    break;
            }
            break;

        case 0x38:
            resetOpIndexes();
            switch (op1Type) {
                case reg:
                    registers[operands8[op8Index]]++;
                    break;
                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index]; break;
                        case 2: dest = operands16[op16Index]; break;
                        case 4: dest = operands32[op32Index]; break;
                        case 8: dest = operands64[op64Index]; break;
                    }
                    write64(dest, read64(dest) + 1);
                    break;
                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index]]; break;
                        case 2: dest = registers[operands16[op16Index]]; break;
                        case 4: dest = registers[operands32[op32Index]]; break;
                        case 8: dest = registers[operands64[op64Index]]; break;
                    }
                    write64(dest, read64(dest) + 1);
                    break;
            }
            break;

        case 0x39:
            resetOpIndexes();
            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index];
                    registers[dest] = (registers[dest] & ~0xFFULL) | ((registers[dest] - 1) & 0xFF);
                    break;
                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index]; break;
                        case 2: dest = operands16[op16Index]; break;
                        case 4: dest = operands32[op32Index]; break;
                        case 8: dest = operands64[op64Index]; break;
                    }
                    write8(dest, read8(dest) - 1);
                    break;
                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index]]; break;
                        case 2: dest = registers[operands16[op16Index]]; break;
                        case 4: dest = registers[operands32[op32Index]]; break;
                        case 8: dest = registers[operands64[op64Index]]; break;
                    }
                    write8(dest, read8(dest) - 1);
                    break;
            }
            break;

        case 0x3A:
            resetOpIndexes();
            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index];
                    registers[dest] = (registers[dest] & ~0xFFFFULL) | ((registers[dest] - 1) & 0xFFFF);
                    break;
                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index]; break;
                        case 2: dest = operands16[op16Index]; break;
                        case 4: dest = operands32[op32Index]; break;
                        case 8: dest = operands64[op64Index]; break;
                    }
                    write16(dest, read16(dest) - 1);
                    break;
                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index]]; break;
                        case 2: dest = registers[operands16[op16Index]]; break;
                        case 4: dest = registers[operands32[op32Index]]; break;
                        case 8: dest = registers[operands64[op64Index]]; break;
                    }
                    write16(dest, read16(dest) - 1);
                    break;
            }
            break;

        case 0x3B:
            resetOpIndexes();
            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index];
                    registers[dest] = (registers[dest] & ~0xFFFFFFFFULL) | ((registers[dest] - 1) & 0xFFFFFFFF);
                    break;
                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index]; break;
                        case 2: dest = operands16[op16Index]; break;
                        case 4: dest = operands32[op32Index]; break;
                        case 8: dest = operands64[op64Index]; break;
                    }
                    write32(dest, read32(dest) - 1);
                    break;
                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index]]; break;
                        case 2: dest = registers[operands16[op16Index]]; break;
                        case 4: dest = registers[operands32[op32Index]]; break;
                        case 8: dest = registers[operands64[op64Index]]; break;
                    }
                    write32(dest, read32(dest) - 1);
                    break;
            }
            break;

        case 0x3C:
            resetOpIndexes();
            switch (op1Type) {
                case reg:
                    registers[operands8[op8Index]]--;
                    break;
                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index]; break;
                        case 2: dest = operands16[op16Index]; break;
                        case 4: dest = operands32[op32Index]; break;
                        case 8: dest = operands64[op64Index]; break;
                    }
                    write64(dest, read64(dest) - 1);
                    break;
                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index]]; break;
                        case 2: dest = registers[operands16[op16Index]]; break;
                        case 4: dest = registers[operands32[op32Index]]; break;
                        case 8: dest = registers[operands64[op64Index]]; break;
                    }
                    write64(dest, read64(dest) - 1);
                    break;
            }
            break;

        case 0x3D:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFF;
                    break;

                case imm:
                    value1 = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read8(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read8(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFF;
                    break;

                case imm:
                    value2 = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read8(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read8(addr);
                    break;
            }

            value = (int8_t)value1 % (int8_t)value2;
            setFlagBit(FLAG_ZERO, (value & 0xFF) == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80);
            value &= 0xFF;

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

        case 0x3E:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }

                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFF;
                    break;

                case imm:
                    value1 = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read8(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read8(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFF;
                    break;

                case imm:
                    value2 = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read8(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read8(addr);
                    break;
            }

            value = (value1 & 0xFF) % (value2 & 0xFF);
            setFlagBit(FLAG_ZERO, (value & 0xFF) == 0);
            value &= 0xFF;

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

        case 0x3F:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read16(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read16(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read16(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read16(addr);
                    break;
            }

            value = (int16_t)value1 % (int16_t)value2;
            setFlagBit(FLAG_ZERO, (value & 0xFFFF) == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000);
            value &= 0xFFFF;

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

        case 0x40:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }

                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read16(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read16(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read16(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read16(addr);
                    break;
            }

            value = (value1 & 0xFFFF) % (value2 & 0xFFFF);
            setFlagBit(FLAG_ZERO, (value & 0xFFFF) == 0);
            value &= 0xFFFF;

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

        case 0x41:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read32(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read32(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read32(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read32(addr);
                    break;
            }

            value = (int32_t)value1 % (int32_t)value2;
            setFlagBit(FLAG_ZERO, (value & 0xFFFFFFFF) == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80000000);
            value &= 0xFFFFFFFF;

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

        case 0x42:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read32(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read32(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read32(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read32(addr);
                    break;
            }

            value = (value1 & 0xFFFFFFFF) % (value2 & 0xFFFFFFFF);
            setFlagBit(FLAG_ZERO, (value & 0xFFFFFFFF) == 0);
            value &= 0xFFFFFFFF;

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

        case 0x43:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]];
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                        case 8: value1 = operands64[op64Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read64(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read64(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                        case 8: value2 = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read64(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read64(addr);
                    break;
            }

            value = (int64_t)value1 % (int64_t)value2;
            setFlagBit(FLAG_ZERO, value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000000000000000);

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

        case 0x44:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]];
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                        case 8: value1 = operands64[op64Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read64(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read64(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                        case 8: value2 = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read64(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read64(addr);
                    break;
            }

            value = (uint64_t)value1 % (uint64_t)value2;
            setFlagBit(FLAG_ZERO, value == 0);

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

        case 0x45:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    value1 = registers[dest] & 0xFF;
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }

                    value1 = read8(dest);
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read8(dest);
                    break;
            }

            value = (uint64_t)(-(int8_t)value1) & 0xFF;
            setFlagBit(FLAG_ZERO, value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80);
            setFlagBit(FLAG_OVERFLOW, value1 == 0x80);
            setFlagBit(FLAG_CARRY, value1 != 0);

            switch (op1Type) {
                case reg:
                    registers[dest] = value;
                    break;

                case mem_imm:
                case mem_reg:
                    write8(dest, (uint8_t)value);
                    break;
            }

            break;

        case 0x46:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    value1 = registers[dest] & 0xFFFF;
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }

                    value1 = read16(dest);
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read16(dest);
                    break;
            }

            value = (uint64_t)(-(int16_t)value1) & 0xFFFF;
            setFlagBit(FLAG_ZERO, value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000);
            setFlagBit(FLAG_OVERFLOW, value1 == 0x8000);
            setFlagBit(FLAG_CARRY, value1 != 0);

            switch (op1Type) {
                case reg:
                    registers[dest] = value;
                    break;

                case mem_imm:
                case mem_reg:
                    write16(dest, (uint16_t)value);
                    break;
            }

            break;

        case 0x47:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    value1 = registers[dest] & 0xFFFFFFFF;
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }

                    value1 = read32(dest);
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read32(dest);
                    break;
            }

            value = (uint64_t)(-(int32_t)value1) & 0xFFFFFFFF;
            setFlagBit(FLAG_ZERO, value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80000000);
            setFlagBit(FLAG_OVERFLOW, value1 == 0x80000000);
            setFlagBit(FLAG_CARRY, value1 != 0);

            switch (op1Type) {
                case reg:
                    registers[dest] = value;
                    break;

                case mem_imm:
                case mem_reg:
                    write32(dest, (uint32_t)value);
                    break;
            }

            break;

        case 0x48:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    value1 = registers[dest];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }

                    value1 = read64(dest);
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read64(dest);
                    break;
            }

            value = (uint64_t)(-(int64_t)value1);
            setFlagBit(FLAG_ZERO, value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000000000000000);
            setFlagBit(FLAG_OVERFLOW, value1 == 0x8000000000000000);
            setFlagBit(FLAG_CARRY, value1 != 0);

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

        case 0x49:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFF;
                    break;

                case imm:
                    value1 = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read8(addr);
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read8(addr);
                    break;
            }

            switch (op2Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFF;
                    break;

                case imm:
                    value2 = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read8(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read8(addr);
                    break;
            }

            value = (value1 - value2) & 0xFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80);
            setFlagBit(FLAG_OVERFLOW, ((value1 ^ value2) & (value1 ^ value) & 0x80) != 0);

            break;

        case 0x4A:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFF;
                    break;

                case imm:
                    value1 = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read8(addr);
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read8(addr);
                    break;
            }

            switch (op2Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFF;
                    break;

                case imm:
                    value2 = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read8(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read8(addr);
                    break;
            }

            value = (value1 - value2) & 0xFF;
            setFlagBit(FLAG_ZERO,  value == 0);
            setFlagBit(FLAG_CARRY, value1 < value2);

            break;

        case 0x4B:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFF;
                    break;

                case imm:
                    switch (op1Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read16(addr);
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read16(addr);
                    break;
            }

            switch (op2Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read16(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read16(addr);
                    break;
            }

            value = (value1 - value2) & 0xFFFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000);
            setFlagBit(FLAG_OVERFLOW, ((value1 ^ value2) & (value1 ^ value) & 0x8000) != 0);

            break;

        case 0x4C:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFF;
                    break;

                case imm:
                    switch (op1Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read16(addr);
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read16(addr);
                    break;
            }

            switch (op2Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read16(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read16(addr);
                    break;
            }

            value = (value1 - value2) & 0xFFFF;
            setFlagBit(FLAG_ZERO,  value == 0);
            setFlagBit(FLAG_CARRY, value1 < value2);

            break;

        case 0x4D:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op1Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read32(addr);
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read32(addr);
                    break;
            }

            switch (op2Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read32(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read32(addr);
                    break;
            }

            value = (value1 - value2) & 0xFFFFFFFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80000000);
            setFlagBit(FLAG_OVERFLOW, ((value1 ^ value2) & (value1 ^ value) & 0x80000000) != 0);

            break;

        case 0x4E:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op1Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read32(addr);
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read32(addr);
                    break;
            }

            switch (op2Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read32(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read32(addr);
                    break;
            }

            value = (value1 - value2) & 0xFFFFFFFF;
            setFlagBit(FLAG_ZERO,  value == 0);
            setFlagBit(FLAG_CARRY, value1 < value2);

            break;

        case 0x4F:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]];
                    break;

                case imm:
                    switch (op1Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                        case 8: value1 = operands64[op64Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read64(addr);
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read64(addr);
                    break;
            }

            switch (op2Type) {
                case reg:
                    value2 = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                        case 8: value2 = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read64(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read64(addr);
                    break;
            }

            value = value1 - value2;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000000000000000ULL);
            setFlagBit(FLAG_OVERFLOW, ((value1 ^ value2) & (value1 ^ value) & 0x8000000000000000ULL) != 0);

            break;

        case 0x50:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]];
                    break;

                case imm:
                    switch (op1Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                        case 8: value1 = operands64[op64Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read64(addr);
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read64(addr);
                    break;
            }

            switch (op2Type) {
                case reg:
                    value2 = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                        case 8: value2 = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read64(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read64(addr);
                    break;
            }

            value = value1 - value2;
            setFlagBit(FLAG_ZERO,  value == 0);
            setFlagBit(FLAG_CARRY, value1 < value2);

            break;

        case 0x51:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFF;
                    break;
                case imm:
                    value1 = operands8[op8Index++];
                    break;
                case mem_imm:
                    switch (op1Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read8(addr);
                    break;
                case mem_reg:
                    switch (op1Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read8(addr);
                    break;
            }

            switch (op2Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFF;
                    break;
                case imm:
                    value2 = operands8[op8Index];
                    break;
                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read8(addr);
                    break;
                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read8(addr);
                    break;
            }

            value = (value1 & value2) & 0xFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

            break;

        case 0x52:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFF;
                    break;
                case imm:
                    switch (op1Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                    }
                    break;
                case mem_imm:
                    switch (op1Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read16(addr);
                    break;
                case mem_reg:
                    switch (op1Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read16(addr);
                    break;
            }

            switch (op2Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFF;
                    break;
                case imm:
                    switch (op2Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                    }
                    break;
                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read16(addr);
                    break;
                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read16(addr);
                    break;
            }

            value = (value1 & value2) & 0xFFFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);
            break;

        case 0x53:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFFFFFF;
                    break;
                case imm:
                    switch (op1Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                    }
                    break;
                case mem_imm:
                    switch (op1Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read32(addr);
                    break;
                case mem_reg:
                    switch (op1Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read32(addr);
                    break;
            }

            switch (op2Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFFFFFF;
                    break;
                case imm:
                    switch (op2Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                    }
                    break;
                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read32(addr);
                    break;
                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read32(addr);
                    break;
            }

            value = (value1 & value2) & 0xFFFFFFFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80000000);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);
            break;

        case 0x54:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]];
                    break;
                case imm:
                    switch (op1Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                        case 8: value1 = operands64[op64Index++]; break;
                    }
                    break;
                case mem_imm:
                    switch (op1Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read64(addr);
                    break;
                case mem_reg:
                    switch (op1Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read64(addr);
                    break;
            }

            switch (op2Type) {
                case reg:
                    value2 = registers[operands8[op8Index]];
                    break;
                case imm:
                    switch (op2Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                        case 8: value2 = operands64[op64Index]; break;
                    }
                    break;
                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read64(addr);
                    break;
                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read64(addr);
                    break;
            }

            value = value1 & value2;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000000000000000ULL);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

            break;

        case 0x55:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFF;
                    break;

                case imm:
                    value1 = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read8(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read8(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFF;
                    break;

                case imm:
                    value2 = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read8(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read8(addr);
                    break;
            }

            value = (value1 & value2) & 0xFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x56:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read16(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read16(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read16(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read16(addr);
                    break;
            }

            value = (value1 & value2) & 0xFFFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x57:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read32(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read32(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read32(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read32(addr);
                    break;
            }

            value = (value1 & value2) & 0xFFFFFFFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80000000);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x58:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]];
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                        case 8: value1 = operands64[op64Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read64(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read64(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                        case 8: value2 = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read64(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read64(addr);
                    break;
            }

            value = value1 & value2;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000000000000000ULL);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x59:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFF;
                    break;

                case imm:
                    value1 = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read8(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read8(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFF;
                    break;

                case imm:
                    value2 = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read8(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read8(addr);
                    break;
            }

            value = (value1 | value2) & 0xFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x5A:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read16(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read16(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read16(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read16(addr);
                    break;
            }

            value = (value1 | value2) & 0xFFFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x5B:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read32(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read32(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read32(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read32(addr);
                    break;
            }

            value = (value1 | value2) & 0xFFFFFFFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80000000);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x5C:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]];
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                        case 8: value1 = operands64[op64Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read64(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read64(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                        case 8: value2 = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read64(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read64(addr);
                    break;
            }

            value = value1 | value2;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000000000000000ULL);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x5D:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFF;
                    break;

                case imm:
                    value1 = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read8(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read8(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFF;
                    break;

                case imm:
                    value2 = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read8(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read8(addr);
                    break;
            }

            value = (value1 ^ value2) & 0xFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x5E:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read16(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read16(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read16(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read16(addr);
                    break;
            }

            value = (value1 ^ value2) & 0xFFFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x5F:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read32(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read32(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read32(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read32(addr);
                    break;
            }

            value = (value1 ^ value2) & 0xFFFFFFFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80000000);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x60:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]];
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                        case 8: value1 = operands64[op64Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read64(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read64(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                        case 8: value2 = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read64(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read64(addr);
                    break;
            }

            value = value1 ^ value2;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000000000000000ULL);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x61:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index];
                    value1 = registers[dest] & 0xFF;
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index]; break;
                        case 2: dest = operands16[op16Index]; break;
                        case 4: dest = operands32[op32Index]; break;
                        case 8: dest = operands64[op64Index]; break;
                    }

                    value1 = read8(dest);
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index]]; break;
                        case 2: dest = registers[operands16[op16Index]]; break;
                        case 4: dest = registers[operands32[op32Index]]; break;
                        case 8: dest = registers[operands64[op64Index]]; break;
                    }

                    value1 = read8(dest);
                    break;
            }

            value = (~value1) & 0xFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

            switch (op1Type) {
                case reg:
                    registers[dest] = value;
                    break;

                case mem_imm:
                case mem_reg:
                    write8(dest, (uint8_t)value);
                    break;
            }

            break;

        case 0x62:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index];
                    value1 = registers[dest] & 0xFFFF;
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index]; break;
                        case 2: dest = operands16[op16Index]; break;
                        case 4: dest = operands32[op32Index]; break;
                        case 8: dest = operands64[op64Index]; break;
                    }

                    value1 = read16(dest);
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index]]; break;
                        case 2: dest = registers[operands16[op16Index]]; break;
                        case 4: dest = registers[operands32[op32Index]]; break;
                        case 8: dest = registers[operands64[op64Index]]; break;
                    }

                    value1 = read16(dest);
                    break;
            }

            value = (~value1) & 0xFFFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

            switch (op1Type) {
                case reg:
                    registers[dest] = value;
                    break;

                case mem_imm:
                case mem_reg:
                    write16(dest, (uint16_t)value);
                    break;
            }

            break;

        case 0x63:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index];
                    value1 = registers[dest] & 0xFFFFFFFF;
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index]; break;
                        case 2: dest = operands16[op16Index]; break;
                        case 4: dest = operands32[op32Index]; break;
                        case 8: dest = operands64[op64Index]; break;
                    }

                    value1 = read32(dest);
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index]]; break;
                        case 2: dest = registers[operands16[op16Index]]; break;
                        case 4: dest = registers[operands32[op32Index]]; break;
                        case 8: dest = registers[operands64[op64Index]]; break;
                    }

                    value1 = read32(dest);
                    break;
            }

            value = (~value1) & 0xFFFFFFFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80000000);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

            switch (op1Type) {
                case reg:
                    registers[dest] = value;
                    break;

                case mem_imm:
                case mem_reg:
                    write32(dest, (uint32_t)value);
                    break;
            }

            break;

        case 0x64:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index];
                    value1 = registers[dest];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index]; break;
                        case 2: dest = operands16[op16Index]; break;
                        case 4: dest = operands32[op32Index]; break;
                        case 8: dest = operands64[op64Index]; break;
                    }

                    value1 = read64(dest);
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index]]; break;
                        case 2: dest = registers[operands16[op16Index]]; break;
                        case 4: dest = registers[operands32[op32Index]]; break;
                        case 8: dest = registers[operands64[op64Index]]; break;
                    }

                    value1 = read64(dest);
                    break;
            }

            value = ~value1;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000000000000000ULL);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x65:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFF;
                    break;

                case imm:
                    value1 = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read8(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read8(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFF;
                    break;

                case imm:
                    value2 = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read8(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read8(addr);
                    break;
            }

            value = (value1 << (value2 & 7)) & 0xFF;
            sh = value2 & 7;
            setFlagBit(FLAG_CARRY, sh != 0 && ((value1 << (sh - 1)) & 0x80));
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80);

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

        case 0x66:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read16(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read16(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read16(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read16(addr);
                    break;
            }

            value = (value1 << (value2 & 15)) & 0xFFFF;
            sh = value2 & 15;
            setFlagBit(FLAG_CARRY, sh != 0 && ((value1 << (sh - 1)) & 0x8000));
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000);

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

        case 0x67:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read32(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read32(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read32(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read32(addr);
                    break;
            }

            value = (value1 << (value2 & 31)) & 0xFFFFFFFF;
            sh = value2 & 31;
            setFlagBit(FLAG_CARRY, sh != 0 && ((value1 << (sh - 1)) & 0x80000000));
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80000000);

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

        case 0x68:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]];
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                        case 8: value1 = operands64[op64Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read64(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read64(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                        case 8: value2 = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read64(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read64(addr);
                    break;
            }

            value = value1 << (value2 & 63);
            sh = value2 & 63;
            setFlagBit(FLAG_CARRY, sh != 0 && ((value1 << (sh - 1)) & 0x8000000000000000ULL));
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000000000000000ULL);

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

        case 0x69:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFF;
                    break;

                case imm:
                    value1 = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read8(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read8(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFF;
                    break;

                case imm:
                    value2 = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read8(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read8(addr);
                    break;
            }

            value = (value1 & 0xFF) >> (value2 & 7);
            sh = value2 & 7;
            setFlagBit(FLAG_CARRY, sh != 0 && ((value1 >> (sh - 1)) & 1));
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80);

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

        case 0x6A:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read16(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read16(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read16(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read16(addr);
                    break;
            }

            value = (value1 & 0xFFFF) >> (value2 & 15);
            sh = value2 & 15;
            setFlagBit(FLAG_CARRY, sh != 0 && ((value1 >> (sh - 1)) & 1));
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000);

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

        case 0x6B:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read32(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read32(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read32(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read32(addr);
                    break;
            }

            value = (value1 & 0xFFFFFFFF) >> (value2 & 31);
            sh = value2 & 31;
            setFlagBit(FLAG_CARRY, sh != 0 && ((value1 >> (sh - 1)) & 1));
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80000000);

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

        case 0x6C:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]];
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                        case 8: value1 = operands64[op64Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read64(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read64(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                        case 8: value2 = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read64(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read64(addr);
                    break;
            }

            value = value1 >> (value2 & 63);
            sh = value2 & 63;
            setFlagBit(FLAG_CARRY, sh != 0 && ((value1 >> (sh - 1)) & 1));
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000000000000000ULL);

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

        case 0x6D:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFF;
                    break;

                case imm:
                    value1 = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read8(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read8(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFF;
                    break;

                case imm:
                    value2 = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read8(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read8(addr);
                    break;
            }

            value = ((int8_t)value1 >> (value2 & 7)) & 0xFF;
            sh = value2 & 7;
            setFlagBit(FLAG_CARRY, sh != 0 && ((value1 >> (sh - 1)) & 1));
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80);

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

        case 0x6E:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read16(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read16(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read16(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read16(addr);
                    break;
            }

            value = ((int16_t)value1 >> (value2 & 15)) & 0xFFFF;
            sh = value2 & 15;
            setFlagBit(FLAG_CARRY, sh != 0 && ((value1 >> (sh - 1)) & 1));
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000);

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

        case 0x6F:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read32(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read32(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read32(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read32(addr);
                    break;
            }

            value = ((int32_t)value1 >> (value2 & 31)) & 0xFFFFFFFF;
            sh = value2 & 31;
            setFlagBit(FLAG_CARRY, sh != 0 && ((value1 >> (sh - 1)) & 1));
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80000000);

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

        case 0x70:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]];
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                        case 8: value1 = operands64[op64Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read64(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read64(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                        case 8: value2 = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read64(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read64(addr);
                    break;
            }

            value = (uint64_t)((int64_t)value1 >> (value2 & 63));
            sh = value2 & 63;
            setFlagBit(FLAG_CARRY, sh != 0 && ((value1 >> (sh - 1)) & 1));
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000000000000000ULL);

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

        case 0x71:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFF;
                    break;

                case imm:
                    value1 = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read8(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read8(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFF;
                    break;

                case imm:
                    value2 = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read8(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read8(addr);
                    break;
            }

            value = (~value1 & value2) & 0xFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x72:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read16(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read16(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read16(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read16(addr);
                    break;
            }

            value = (~value1 & value2) & 0xFFFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x73:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read32(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read32(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read32(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read32(addr);
                    break;
            }

            value = (~value1 & value2) & 0xFFFFFFFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80000000);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x74:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]];
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                        case 8: value1 = operands64[op64Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read64(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read64(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                        case 8: value2 = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read64(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read64(addr);
                    break;
            }

            value = ~value1 & value2;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000000000000000ULL);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x75:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFF;
                    break;

                case imm:
                    value1 = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read8(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read8(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFF;
                    break;

                case imm:
                    value2 = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read8(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read8(addr);
                    break;
            }

            value = (value1 | (1ULL << (value2 & 7))) & 0xFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x76:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read16(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read16(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read16(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read16(addr);
                    break;
            }

            value = (value1 | (1ULL << (value2 & 15))) & 0xFFFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x77:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read32(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read32(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read32(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read32(addr);
                    break;
            }

            value = (value1 | (1ULL << (value2 & 31))) & 0xFFFFFFFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80000000);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x78:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]];
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                        case 8: value1 = operands64[op64Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read64(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read64(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                        case 8: value2 = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read64(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read64(addr);
                    break;
            }

            value = value1 | (1ULL << (value2 & 63));
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000000000000000ULL);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x79:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFF;
                    break;

                case imm:
                    value1 = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read8(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read8(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFF;
                    break;

                case imm:
                    value2 = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read8(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read8(addr);
                    break;
            }

            value = (value1 & ~(1ULL << (value2 & 7))) & 0xFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x7A:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read16(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read16(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read16(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read16(addr);
                    break;
            }

            value = (value1 & ~(1ULL << (value2 & 15))) & 0xFFFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x7B:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read32(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read32(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read32(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read32(addr);
                    break;
            }

            value = (value1 & ~(1ULL << (value2 & 31))) & 0xFFFFFFFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80000000);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x7C:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]];
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                        case 8: value1 = operands64[op64Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read64(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read64(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                        case 8: value2 = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read64(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read64(addr);
                    break;
            }

            value = value1 & ~(1ULL << (value2 & 63));
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000000000000000ULL);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x7D:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFF;
                    break;

                case imm:
                    value1 = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read8(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read8(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFF;
                    break;

                case imm:
                    value2 = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read8(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read8(addr);
                    break;
            }

            value = (value1 ^ (1ULL << (value2 & 7))) & 0xFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x7E:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read16(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read16(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read16(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read16(addr);
                    break;
            }

            value = (value1 ^ (1ULL << (value2 & 15))) & 0xFFFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x7F:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read32(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read32(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read32(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read32(addr);
                    break;
            }

            value = (value1 ^ (1ULL << (value2 & 31))) & 0xFFFFFFFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80000000);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x80:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]];
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                        case 8: value1 = operands64[op64Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read64(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read64(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                        case 8: value2 = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read64(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read64(addr);
                    break;
            }

            value = value1 ^ (1ULL << (value2 & 63));
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000000000000000ULL);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x81:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFF;
                    break;

                case imm:
                    value1 = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read8(addr);
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read8(addr);
                    break;
            }

            switch (op2Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFF;
                    break;

                case imm:
                    value2 = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read8(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read8(addr);
                    break;
            }

            value = (value1 >> (value2 & 7)) & 1;
            setFlagBit(FLAG_ZERO, value == 0);
            setFlagBit(FLAG_CARRY, value != 0);
            setFlagBit(FLAG_NEGATIVE, false);
            setFlagBit(FLAG_OVERFLOW, false);

            break;

        case 0x82:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFF;
                    break;

                case imm:
                    switch (op1Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read16(addr);
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read16(addr);
                    break;
            }

            switch (op2Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read16(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read16(addr);
                    break;
            }

            value = (value1 >> (value2 & 15)) & 1;
            setFlagBit(FLAG_ZERO, value == 0);
            setFlagBit(FLAG_CARRY, value != 0);
            setFlagBit(FLAG_NEGATIVE, false);
            setFlagBit(FLAG_OVERFLOW, false);

            break;

        case 0x83:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op1Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read32(addr);
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read32(addr);
                    break;
            }

            switch (op2Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read32(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read32(addr);
                    break;
            }

            value = (value1 >> (value2 & 31)) & 1;
            setFlagBit(FLAG_ZERO, value == 0);
            setFlagBit(FLAG_CARRY, value != 0);
            setFlagBit(FLAG_NEGATIVE, false);
            setFlagBit(FLAG_OVERFLOW, false);

            break;

        case 0x84:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]];
                    break;

                case imm:
                    switch (op1Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                        case 8: value1 = operands64[op64Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read64(addr);
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read64(addr);
                    break;
            }

            switch (op2Type) {
                case reg:
                    value2 = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                        case 8: value2 = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read64(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read64(addr);
                    break;
            }

            value = (value1 >> (value2 & 63)) & 1;
            setFlagBit(FLAG_ZERO, value == 0);
            setFlagBit(FLAG_CARRY, value != 0);
            setFlagBit(FLAG_NEGATIVE, false);
            setFlagBit(FLAG_OVERFLOW, false);

            break;

        case 0x85:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index];
                    value1 = registers[dest] & 0xFFFF;
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index]; break;
                        case 2: dest = operands16[op16Index]; break;
                        case 4: dest = operands32[op32Index]; break;
                        case 8: dest = operands64[op64Index]; break;
                    }

                    value1 = read16(dest);
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index]]; break;
                        case 2: dest = registers[operands16[op16Index]]; break;
                        case 4: dest = registers[operands32[op32Index]]; break;
                        case 8: dest = registers[operands64[op64Index]]; break;
                    }

                    value1 = read16(dest);
                    break;
            }

            value = ((value1 & 0xFF) << 8) | ((value1 >> 8) & 0xFF);

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

        case 0x86:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index];
                    value1 = registers[dest] & 0xFFFFFFFF;
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index]; break;
                        case 2: dest = operands16[op16Index]; break;
                        case 4: dest = operands32[op32Index]; break;
                        case 8: dest = operands64[op64Index]; break;
                    }

                    value1 = read32(dest);
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index]]; break;
                        case 2: dest = registers[operands16[op16Index]]; break;
                        case 4: dest = registers[operands32[op32Index]]; break;
                        case 8: dest = registers[operands64[op64Index]]; break;
                    }

                    value1 = read32(dest);
                    break;
            }


            value = _byteswap_ulong(value1);

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

        case 0x87:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index];
                    value1 = registers[dest];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index]; break;
                        case 2: dest = operands16[op16Index]; break;
                        case 4: dest = operands32[op32Index]; break;
                        case 8: dest = operands64[op64Index]; break;
                    }

                    value1 = read64(dest);
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index]]; break;
                        case 2: dest = registers[operands16[op16Index]]; break;
                        case 4: dest = registers[operands32[op32Index]]; break;
                        case 8: dest = registers[operands64[op64Index]]; break;
                    }

                    value1 = read64(dest);
                    break;
            }

            value = _byteswap_uint64(value1);

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

        case 0x88:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }

                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFF;
                    break;

                case imm:
                    value1 = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read8(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read8(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFF;
                    break;

                case imm:
                    value2 = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read8(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read8(addr);
                    break;
            }

            carry = getFlagBit(FLAG_CARRY);
            sum = value1 + value2 + carry;
            value = sum & 0xFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80);
            setFlagBit(FLAG_OVERFLOW, ((~(value1 ^ value2)) & (value1 ^ value) & 0x80) != 0);

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

        case 0x89:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }

                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFF;
                    break;

                case imm:
                    value1 = operands8[op8Index++];
                    break;
 
                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read8(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read8(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFF;
                    break;

                case imm:
                    value2 = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read8(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read8(addr);
                    break;
            }

            carry = getFlagBit(FLAG_CARRY);
            sum = value1 + value2 + carry;
            value = sum & 0xFF;
            setFlagBit(FLAG_ZERO,  value == 0);
            setFlagBit(FLAG_CARRY, sum > 0xFF);

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

        case 0x8A:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }

                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                    }

                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read16(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read16(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                    }

                    break;       

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read16(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read16(addr);
                    break;
            }

            carry = getFlagBit(FLAG_CARRY);
            sum = value1 + value2 + carry;
            value = sum & 0xFFFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000);
            setFlagBit(FLAG_OVERFLOW, ((~(value1 ^ value2)) & (value1 ^ value) & 0x8000) != 0);

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

        case 0x8B:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }

                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                    }

                    break; 
 
                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read16(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read16(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                    }

                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read16(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read16(addr);
                    break;
            }

            carry = getFlagBit(FLAG_CARRY);
            sum = value1 + value2 + carry;
            value = sum & 0xFFFF;
            setFlagBit(FLAG_ZERO,  value == 0);
            setFlagBit(FLAG_CARRY, sum > 0xFFFF);

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

        case 0x8C:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read32(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read32(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read32(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read32(addr);
                    break;
            }

            carry = getFlagBit(FLAG_CARRY);
            sum = value1 + value2 + carry;
            value = sum & 0xFFFFFFFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80000000);
            setFlagBit(FLAG_OVERFLOW, ((~(value1 ^ value2)) & (value1 ^ value) & 0x80000000) != 0);

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

        case 0x8D:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read32(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read32(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read32(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read32(addr);
                    break;
            }

            carry = getFlagBit(FLAG_CARRY);
            sum = value1 + value2 + carry;
            value = sum & 0xFFFFFFFF;
            setFlagBit(FLAG_ZERO,  value == 0);
            setFlagBit(FLAG_CARRY, sum > 0xFFFFFFFF);

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

        case 0x8E:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]];
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                        case 8: value1 = operands64[op64Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read64(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read64(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                        case 8: value2 = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read64(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read64(addr);
                    break;
            }

            carry = getFlagBit(FLAG_CARRY);
            value = value1 + value2 + carry;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000000000000000ULL);
            setFlagBit(FLAG_OVERFLOW, ((~(value1 ^ value2)) & (value1 ^ value) & 0x8000000000000000ULL) != 0);

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

        case 0x8F:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]];
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                        case 8: value1 = operands64[op64Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read64(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read64(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                        case 8: value2 = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read64(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read64(addr);
                    break;
            }

            carry = getFlagBit(FLAG_CARRY);
            c_out = _addcarry_u64((unsigned char)carry, value1, value2, (unsigned long long*)&value);
            setFlagBit(FLAG_ZERO,  value == 0);
            setFlagBit(FLAG_CARRY, c_out);

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

        case 0x90:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }

                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFF;
                    break;

                case imm:
                    value1 = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read8(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read8(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFF;
                    break;

                case imm:
                    value2 = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read8(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read8(addr);
                    break;
            }

            carry = getFlagBit(FLAG_CARRY);
            value = (value1 - value2 - carry) & 0xFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80);
            setFlagBit(FLAG_OVERFLOW, ((value1 ^ value2) & (value1 ^ value) & 0x80) != 0);

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

        case 0x91:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }

                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFF;
                    break;

                case imm:
                    value1 = operands8[op8Index++];
                    break;
 
                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read8(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read8(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFF;
                    break;

                case imm:
                    value2 = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read8(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read8(addr);
                    break;
            }

            carry = getFlagBit(FLAG_CARRY);
            value = (value1 - value2 - carry) & 0xFF;
            setFlagBit(FLAG_ZERO,  value == 0);
            setFlagBit(FLAG_CARRY, value1 < (value2 + carry));

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

        case 0x92:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }

                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                    }

                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read16(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read16(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                    }

                    break;       

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read16(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read16(addr);
                    break;
            }

            carry = getFlagBit(FLAG_CARRY);
            value = (value1 - value2 - carry) & 0xFFFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000);
            setFlagBit(FLAG_OVERFLOW, ((value1 ^ value2) & (value1 ^ value) & 0x8000) != 0);

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

        case 0x93:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }

                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                    }

                    break; 
 
                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read16(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read16(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                    }

                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read16(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read16(addr);
                    break;
            }

            carry = getFlagBit(FLAG_CARRY);
            value = (value1 - value2 - carry) & 0xFFFF;
            setFlagBit(FLAG_ZERO,  value == 0);
            setFlagBit(FLAG_CARRY, value1 < (value2 + carry));

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

        case 0x94:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read32(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read32(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read32(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read32(addr);
                    break;
            }

            carry = getFlagBit(FLAG_CARRY);
            value = (value1 - value2 - carry) & 0xFFFFFFFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80000000);
            setFlagBit(FLAG_OVERFLOW, ((value1 ^ value2) & (value1 ^ value) & 0x80000000) != 0);

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

        case 0x95:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read32(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read32(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read32(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read32(addr);
                    break;
            }

            carry = getFlagBit(FLAG_CARRY);
            value = (value1 - value2 - carry) & 0xFFFFFFFF;
            setFlagBit(FLAG_ZERO,  value == 0);
            setFlagBit(FLAG_CARRY, value1 < (value2 + carry));

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

        case 0x96:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]];
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                        case 8: value1 = operands64[op64Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read64(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read64(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                        case 8: value2 = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read64(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read64(addr);
                    break;
            }

            carry = getFlagBit(FLAG_CARRY);
            value = value1 - value2 - carry;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000000000000000ULL);
            setFlagBit(FLAG_OVERFLOW, ((value1 ^ value2) & (value1 ^ value) & 0x8000000000000000ULL) != 0);

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

        case 0x97:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]];
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                        case 8: value1 = operands64[op64Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read64(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read64(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                        case 8: value2 = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read64(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read64(addr);
                    break;
            }

            carry = getFlagBit(FLAG_CARRY);
            c_out = _subborrow_u64((unsigned char)carry, value1, value2, (unsigned long long*)&value);
            setFlagBit(FLAG_ZERO,  value == 0);
            setFlagBit(FLAG_CARRY, c_out);

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

        case 0x98:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index];
                    value1 = registers[dest] & 0xFF;
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index]; break;
                        case 2: dest = operands16[op16Index]; break;
                        case 4: dest = operands32[op32Index]; break;
                        case 8: dest = operands64[op64Index]; break;
                    }

                    value1 = read8(dest);
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index]]; break;
                        case 2: dest = registers[operands16[op16Index]]; break;
                        case 4: dest = registers[operands32[op32Index]]; break;
                        case 8: dest = registers[operands64[op64Index]]; break;
                    }

                    value1 = read8(dest);
                    break;
            }

            value = ((int8_t)value1 < 0) ? (uint64_t)(-(int8_t)value1) & 0xFF : value1 & 0xFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, false);
            setFlagBit(FLAG_OVERFLOW, value1 == 0x80);
            setFlagBit(FLAG_CARRY,    false);

            switch (op1Type) {
                case reg:
                    registers[dest] = value;
                    break;

                case mem_imm:
                case mem_reg:
                    write8(dest, (uint8_t)value);
                    break;
            }

            break;

        case 0x99:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index];
                    value1 = registers[dest] & 0xFFFF;
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index]; break;
                        case 2: dest = operands16[op16Index]; break;
                        case 4: dest = operands32[op32Index]; break;
                        case 8: dest = operands64[op64Index]; break;
                    }

                    value1 = read16(dest);
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index]]; break;
                        case 2: dest = registers[operands16[op16Index]]; break;
                        case 4: dest = registers[operands32[op32Index]]; break;
                        case 8: dest = registers[operands64[op64Index]]; break;
                    }

                    value1 = read16(dest);
                    break;
            }

            value = ((int16_t)value1 < 0) ? (uint64_t)(-(int16_t)value1) & 0xFFFF : value1 & 0xFFFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, false);
            setFlagBit(FLAG_OVERFLOW, value1 == 0x8000);
            setFlagBit(FLAG_CARRY,    false);

            switch (op1Type) {
                case reg:
                    registers[dest] = value;
                    break;

                case mem_imm:
                case mem_reg:
                    write16(dest, (uint16_t)value);
                    break;
            }

            break;

        case 0x9A:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index];
                    value1 = registers[dest] & 0xFFFFFFFF;
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index]; break;
                        case 2: dest = operands16[op16Index]; break;
                        case 4: dest = operands32[op32Index]; break;
                        case 8: dest = operands64[op64Index]; break;
                    }

                    value1 = read32(dest);
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index]]; break;
                        case 2: dest = registers[operands16[op16Index]]; break;
                        case 4: dest = registers[operands32[op32Index]]; break;
                        case 8: dest = registers[operands64[op64Index]]; break;
                    }

                    value1 = read32(dest);
                    break;
            }

            value = ((int32_t)value1 < 0) ? (uint64_t)(-(int32_t)value1) & 0xFFFFFFFF : value1 & 0xFFFFFFFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, false);
            setFlagBit(FLAG_OVERFLOW, value1 == 0x80000000);
            setFlagBit(FLAG_CARRY,    false);

            switch (op1Type) {
                case reg:
                    registers[dest] = value;
                    break;

                case mem_imm:
                case mem_reg:
                    write32(dest, (uint32_t)value);
                    break;
            }

            break;

        case 0x9B:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index];
                    value1 = registers[dest];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index]; break;
                        case 2: dest = operands16[op16Index]; break;
                        case 4: dest = operands32[op32Index]; break;
                        case 8: dest = operands64[op64Index]; break;
                    }

                    value1 = read64(dest);
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index]]; break;
                        case 2: dest = registers[operands16[op16Index]]; break;
                        case 4: dest = registers[operands32[op32Index]]; break;
                        case 8: dest = registers[operands64[op64Index]]; break;
                    }

                    value1 = read64(dest);
                    break;
            }

            value = ((int64_t)value1 < 0) ? (uint64_t)(-(int64_t)value1) : value1;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, false);
            setFlagBit(FLAG_OVERFLOW, value1 == 0x8000000000000000ULL);
            setFlagBit(FLAG_CARRY,    false);

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

        case 0x9C:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]];
                    break;

                case imm:
                    switch (op1Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                        case 8: value1 = operands64[op64Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }

                    value1 = read64(dest);
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read64(dest);
                    break;
            }

            instructionPointer = value1;

            break;

        case 0x9D:
            if (getFlagBit(FLAG_ZERO)) {
                resetOpIndexes();
                
                switch (op1Type) {
                    case reg:
                        value1 = registers[operands8[op8Index++]];
                        break;

                    case imm:
                        switch (op1Size) {
                            case 1: value1 = operands8[op8Index++]; break;
                            case 2: value1 = operands16[op16Index++]; break;
                            case 4: value1 = operands32[op32Index++]; break;
                            case 8: value1 = operands64[op64Index++]; break;
                        }
                        break;

                    case mem_imm:
                        switch (op1Size) {
                            case 1: dest = operands8[op8Index++]; break;
                            case 2: dest = operands16[op16Index++]; break;
                            case 4: dest = operands32[op32Index++]; break;
                            case 8: dest = operands64[op64Index++]; break;
                        }

                        value1 = read64(dest);
                        break;

                    case mem_reg:
                        switch (op1Size) {
                            case 1: dest = registers[operands8[op8Index++]]; break;
                            case 2: dest = registers[operands16[op16Index++]]; break;
                            case 4: dest = registers[operands32[op32Index++]]; break;
                            case 8: dest = registers[operands64[op64Index++]]; break;
                        }

                        value1 = read64(dest);
                        break;
                }

                instructionPointer = value1;
            }

            break;

        case 0x9E:
            if (!getFlagBit(FLAG_ZERO)) {
                resetOpIndexes();
                
                switch (op1Type) {
                    case reg:
                        value1 = registers[operands8[op8Index++]];
                        break;

                    case imm:
                        switch (op1Size) {
                            case 1: value1 = operands8[op8Index++]; break;
                            case 2: value1 = operands16[op16Index++]; break;
                            case 4: value1 = operands32[op32Index++]; break;
                            case 8: value1 = operands64[op64Index++]; break;
                        }
                        break;

                    case mem_imm:
                        switch (op1Size) {
                            case 1: dest = operands8[op8Index++]; break;
                            case 2: dest = operands16[op16Index++]; break;
                            case 4: dest = operands32[op32Index++]; break;
                            case 8: dest = operands64[op64Index++]; break;
                        }

                        value1 = read64(dest);
                        break;

                    case mem_reg:
                        switch (op1Size) {
                            case 1: dest = registers[operands8[op8Index++]]; break;
                            case 2: dest = registers[operands16[op16Index++]]; break;
                            case 4: dest = registers[operands32[op32Index++]]; break;
                            case 8: dest = registers[operands64[op64Index++]]; break;
                        }

                        value1 = read64(dest);
                        break;
                }

                instructionPointer = value1;
            }

            break;

        case 0x9F:
            if (getFlagBit(FLAG_NEGATIVE) != getFlagBit(FLAG_OVERFLOW)) {
                resetOpIndexes();
                
                switch (op1Type) {
                    case reg:
                        value1 = registers[operands8[op8Index++]];
                        break;

                    case imm:
                        switch (op1Size) {
                            case 1: value1 = operands8[op8Index++]; break;
                            case 2: value1 = operands16[op16Index++]; break;
                            case 4: value1 = operands32[op32Index++]; break;
                            case 8: value1 = operands64[op64Index++]; break;
                        }
                        break;

                    case mem_imm:
                        switch (op1Size) {
                            case 1: dest = operands8[op8Index++]; break;
                            case 2: dest = operands16[op16Index++]; break;
                            case 4: dest = operands32[op32Index++]; break;
                            case 8: dest = operands64[op64Index++]; break;
                        }

                        value1 = read64(dest);
                        break;

                    case mem_reg:
                        switch (op1Size) {
                            case 1: dest = registers[operands8[op8Index++]]; break;
                            case 2: dest = registers[operands16[op16Index++]]; break;
                            case 4: dest = registers[operands32[op32Index++]]; break;
                            case 8: dest = registers[operands64[op64Index++]]; break;
                        }

                        value1 = read64(dest);
                        break;
                }

                instructionPointer = value1;
            }

            break;

        case 0xA0:
            if (!getFlagBit(FLAG_ZERO) && (getFlagBit(FLAG_NEGATIVE) == getFlagBit(FLAG_OVERFLOW))) {
                resetOpIndexes();
                
                switch (op1Type) {
                    case reg:
                        value1 = registers[operands8[op8Index++]];
                        break;

                    case imm:
                        switch (op1Size) {
                            case 1: value1 = operands8[op8Index++]; break;
                            case 2: value1 = operands16[op16Index++]; break;
                            case 4: value1 = operands32[op32Index++]; break;
                            case 8: value1 = operands64[op64Index++]; break;
                        }
                        break;

                    case mem_imm:
                        switch (op1Size) {
                            case 1: dest = operands8[op8Index++]; break;
                            case 2: dest = operands16[op16Index++]; break;
                            case 4: dest = operands32[op32Index++]; break;
                            case 8: dest = operands64[op64Index++]; break;
                        }

                        value1 = read64(dest);
                        break;

                    case mem_reg:
                        switch (op1Size) {
                            case 1: dest = registers[operands8[op8Index++]]; break;
                            case 2: dest = registers[operands16[op16Index++]]; break;
                            case 4: dest = registers[operands32[op32Index++]]; break;
                            case 8: dest = registers[operands64[op64Index++]]; break;
                        }

                        value1 = read64(dest);
                        break;
                }

                instructionPointer = value1;
            }

            break;

        case 0xA1:
            if (getFlagBit(FLAG_ZERO) || (getFlagBit(FLAG_NEGATIVE) != getFlagBit(FLAG_OVERFLOW))) {
                resetOpIndexes();
                
                switch (op1Type) {
                    case reg:
                        value1 = registers[operands8[op8Index++]];
                        break;

                    case imm:
                        switch (op1Size) {
                            case 1: value1 = operands8[op8Index++]; break;
                            case 2: value1 = operands16[op16Index++]; break;
                            case 4: value1 = operands32[op32Index++]; break;
                            case 8: value1 = operands64[op64Index++]; break;
                        }
                        break;

                    case mem_imm:
                        switch (op1Size) {
                            case 1: dest = operands8[op8Index++]; break;
                            case 2: dest = operands16[op16Index++]; break;
                            case 4: dest = operands32[op32Index++]; break;
                            case 8: dest = operands64[op64Index++]; break;
                        }

                        value1 = read64(dest);
                        break;

                    case mem_reg:
                        switch (op1Size) {
                            case 1: dest = registers[operands8[op8Index++]]; break;
                            case 2: dest = registers[operands16[op16Index++]]; break;
                            case 4: dest = registers[operands32[op32Index++]]; break;
                            case 8: dest = registers[operands64[op64Index++]]; break;
                        }

                        value1 = read64(dest);
                        break;
                }

                instructionPointer = value1;
            }

            break;

        case 0xA2:
            if (getFlagBit(FLAG_NEGATIVE) == getFlagBit(FLAG_OVERFLOW)) {
                resetOpIndexes();
                
                switch (op1Type) {
                    case reg:
                        value1 = registers[operands8[op8Index++]];
                        break;

                    case imm:
                        switch (op1Size) {
                            case 1: value1 = operands8[op8Index++]; break;
                            case 2: value1 = operands16[op16Index++]; break;
                            case 4: value1 = operands32[op32Index++]; break;
                            case 8: value1 = operands64[op64Index++]; break;
                        }
                        break;

                    case mem_imm:
                        switch (op1Size) {
                            case 1: dest = operands8[op8Index++]; break;
                            case 2: dest = operands16[op16Index++]; break;
                            case 4: dest = operands32[op32Index++]; break;
                            case 8: dest = operands64[op64Index++]; break;
                        }

                        value1 = read64(dest);
                        break;

                    case mem_reg:
                        switch (op1Size) {
                            case 1: dest = registers[operands8[op8Index++]]; break;
                            case 2: dest = registers[operands16[op16Index++]]; break;
                            case 4: dest = registers[operands32[op32Index++]]; break;
                            case 8: dest = registers[operands64[op64Index++]]; break;
                        }

                        value1 = read64(dest);
                        break;
                }

                instructionPointer = value1;
            }

            break;

        case 0xA3:
            if (getFlagBit(FLAG_CARRY)) {
                resetOpIndexes();
                
                switch (op1Type) {
                    case reg:
                        value1 = registers[operands8[op8Index++]];
                        break;

                    case imm:
                        switch (op1Size) {
                            case 1: value1 = operands8[op8Index++]; break;
                            case 2: value1 = operands16[op16Index++]; break;
                            case 4: value1 = operands32[op32Index++]; break;
                            case 8: value1 = operands64[op64Index++]; break;
                        }
                        break;

                    case mem_imm:
                        switch (op1Size) {
                            case 1: dest = operands8[op8Index++]; break;
                            case 2: dest = operands16[op16Index++]; break;
                            case 4: dest = operands32[op32Index++]; break;
                            case 8: dest = operands64[op64Index++]; break;
                        }

                        value1 = read64(dest);
                        break;

                    case mem_reg:
                        switch (op1Size) {
                            case 1: dest = registers[operands8[op8Index++]]; break;
                            case 2: dest = registers[operands16[op16Index++]]; break;
                            case 4: dest = registers[operands32[op32Index++]]; break;
                            case 8: dest = registers[operands64[op64Index++]]; break;
                        }

                        value1 = read64(dest);
                        break;
                }

                instructionPointer = value1;
            }

            break;
        
        case 0xA4:
            if (!getFlagBit(FLAG_CARRY) && !getFlagBit(FLAG_ZERO)) {
                resetOpIndexes();
                
                switch (op1Type) {
                    case reg:
                        value1 = registers[operands8[op8Index++]];
                        break;

                    case imm:
                        switch (op1Size) {
                            case 1: value1 = operands8[op8Index++]; break;
                            case 2: value1 = operands16[op16Index++]; break;
                            case 4: value1 = operands32[op32Index++]; break;
                            case 8: value1 = operands64[op64Index++]; break;
                        }
                        break;

                    case mem_imm:
                        switch (op1Size) {
                            case 1: dest = operands8[op8Index++]; break;
                            case 2: dest = operands16[op16Index++]; break;
                            case 4: dest = operands32[op32Index++]; break;
                            case 8: dest = operands64[op64Index++]; break;
                        }

                        value1 = read64(dest);
                        break;

                    case mem_reg:
                        switch (op1Size) {
                            case 1: dest = registers[operands8[op8Index++]]; break;
                            case 2: dest = registers[operands16[op16Index++]]; break;
                            case 4: dest = registers[operands32[op32Index++]]; break;
                            case 8: dest = registers[operands64[op64Index++]]; break;
                        }

                        value1 = read64(dest);
                        break;
                }

                instructionPointer = value1;
            }

            break;

        case 0xA5:
            if (getFlagBit(FLAG_CARRY) || getFlagBit(FLAG_ZERO)) {
                resetOpIndexes();
                
                switch (op1Type) {
                    case reg:
                        value1 = registers[operands8[op8Index++]];
                        break;

                    case imm:
                        switch (op1Size) {
                            case 1: value1 = operands8[op8Index++]; break;
                            case 2: value1 = operands16[op16Index++]; break;
                            case 4: value1 = operands32[op32Index++]; break;
                            case 8: value1 = operands64[op64Index++]; break;
                        }
                        break;

                    case mem_imm:
                        switch (op1Size) {
                            case 1: dest = operands8[op8Index++]; break;
                            case 2: dest = operands16[op16Index++]; break;
                            case 4: dest = operands32[op32Index++]; break;
                            case 8: dest = operands64[op64Index++]; break;
                        }

                        value1 = read64(dest);
                        break;

                    case mem_reg:
                        switch (op1Size) {
                            case 1: dest = registers[operands8[op8Index++]]; break;
                            case 2: dest = registers[operands16[op16Index++]]; break;
                            case 4: dest = registers[operands32[op32Index++]]; break;
                            case 8: dest = registers[operands64[op64Index++]]; break;
                        }

                        value1 = read64(dest);
                        break;
                }

                instructionPointer = value1;
            }

            break;

        case 0xA6:
            if (!getFlagBit(FLAG_CARRY)) {
                resetOpIndexes();
                
                switch (op1Type) {
                    case reg:
                        value1 = registers[operands8[op8Index++]];
                        break;

                    case imm:
                        switch (op1Size) {
                            case 1: value1 = operands8[op8Index++]; break;
                            case 2: value1 = operands16[op16Index++]; break;
                            case 4: value1 = operands32[op32Index++]; break;
                            case 8: value1 = operands64[op64Index++]; break;
                        }
                        break;

                    case mem_imm:
                        switch (op1Size) {
                            case 1: dest = operands8[op8Index++]; break;
                            case 2: dest = operands16[op16Index++]; break;
                            case 4: dest = operands32[op32Index++]; break;
                            case 8: dest = operands64[op64Index++]; break;
                        }

                        value1 = read64(dest);
                        break;

                    case mem_reg:
                        switch (op1Size) {
                            case 1: dest = registers[operands8[op8Index++]]; break;
                            case 2: dest = registers[operands16[op16Index++]]; break;
                            case 4: dest = registers[operands32[op32Index++]]; break;
                            case 8: dest = registers[operands64[op64Index++]]; break;
                        }

                        value1 = read64(dest);
                        break;
                }

                instructionPointer = value1;
            }

            break;

        case 0xA7:
            if (getFlagBit(FLAG_OVERFLOW)) {
                resetOpIndexes();
                
                switch (op1Type) {
                    case reg:
                        value1 = registers[operands8[op8Index++]];
                        break;

                    case imm:
                        switch (op1Size) {
                            case 1: value1 = operands8[op8Index++]; break;
                            case 2: value1 = operands16[op16Index++]; break;
                            case 4: value1 = operands32[op32Index++]; break;
                            case 8: value1 = operands64[op64Index++]; break;
                        }
                        break;

                    case mem_imm:
                        switch (op1Size) {
                            case 1: dest = operands8[op8Index++]; break;
                            case 2: dest = operands16[op16Index++]; break;
                            case 4: dest = operands32[op32Index++]; break;
                            case 8: dest = operands64[op64Index++]; break;
                        }

                        value1 = read64(dest);
                        break;

                    case mem_reg:
                        switch (op1Size) {
                            case 1: dest = registers[operands8[op8Index++]]; break;
                            case 2: dest = registers[operands16[op16Index++]]; break;
                            case 4: dest = registers[operands32[op32Index++]]; break;
                            case 8: dest = registers[operands64[op64Index++]]; break;
                        }

                        value1 = read64(dest);
                        break;
                }

                instructionPointer = value1;
            }

            break;

        case 0xA8:
            if (!getFlagBit(FLAG_OVERFLOW)) {
                resetOpIndexes();
                
                switch (op1Type) {
                    case reg:
                        value1 = registers[operands8[op8Index++]];
                        break;

                    case imm:
                        switch (op1Size) {
                            case 1: value1 = operands8[op8Index++]; break;
                            case 2: value1 = operands16[op16Index++]; break;
                            case 4: value1 = operands32[op32Index++]; break;
                            case 8: value1 = operands64[op64Index++]; break;
                        }
                        break;

                    case mem_imm:
                        switch (op1Size) {
                            case 1: dest = operands8[op8Index++]; break;
                            case 2: dest = operands16[op16Index++]; break;
                            case 4: dest = operands32[op32Index++]; break;
                            case 8: dest = operands64[op64Index++]; break;
                        }

                        value1 = read64(dest);
                        break;

                    case mem_reg:
                        switch (op1Size) {
                            case 1: dest = registers[operands8[op8Index++]]; break;
                            case 2: dest = registers[operands16[op16Index++]]; break;
                            case 4: dest = registers[operands32[op32Index++]]; break;
                            case 8: dest = registers[operands64[op64Index++]]; break;
                        }

                        value1 = read64(dest);
                        break;
                }

                instructionPointer = value1;
            }

            break;

        case 0xA9:
            if (getFlagBit(FLAG_NEGATIVE)) {
                resetOpIndexes();
                
                switch (op1Type) {
                    case reg:
                        value1 = registers[operands8[op8Index++]];
                        break;

                    case imm:
                        switch (op1Size) {
                            case 1: value1 = operands8[op8Index++]; break;
                            case 2: value1 = operands16[op16Index++]; break;
                            case 4: value1 = operands32[op32Index++]; break;
                            case 8: value1 = operands64[op64Index++]; break;
                        }
                        break;

                    case mem_imm:
                        switch (op1Size) {
                            case 1: dest = operands8[op8Index++]; break;
                            case 2: dest = operands16[op16Index++]; break;
                            case 4: dest = operands32[op32Index++]; break;
                            case 8: dest = operands64[op64Index++]; break;
                        }

                        value1 = read64(dest);
                        break;

                    case mem_reg:
                        switch (op1Size) {
                            case 1: dest = registers[operands8[op8Index++]]; break;
                            case 2: dest = registers[operands16[op16Index++]]; break;
                            case 4: dest = registers[operands32[op32Index++]]; break;
                            case 8: dest = registers[operands64[op64Index++]]; break;
                        }

                        value1 = read64(dest);
                        break;
                }

                instructionPointer = value1;
            }

            break;

        case 0xAA:
            if (!getFlagBit(FLAG_NEGATIVE)) {
                resetOpIndexes();
                
                switch (op1Type) {
                    case reg:
                        value1 = registers[operands8[op8Index++]];
                        break;

                    case imm:
                        switch (op1Size) {
                            case 1: value1 = operands8[op8Index++]; break;
                            case 2: value1 = operands16[op16Index++]; break;
                            case 4: value1 = operands32[op32Index++]; break;
                            case 8: value1 = operands64[op64Index++]; break;
                        }
                        break;

                    case mem_imm:
                        switch (op1Size) {
                            case 1: dest = operands8[op8Index++]; break;
                            case 2: dest = operands16[op16Index++]; break;
                            case 4: dest = operands32[op32Index++]; break;
                            case 8: dest = operands64[op64Index++]; break;
                        }

                        value1 = read64(dest);
                        break;

                    case mem_reg:
                        switch (op1Size) {
                            case 1: dest = registers[operands8[op8Index++]]; break;
                            case 2: dest = registers[operands16[op16Index++]]; break;
                            case 4: dest = registers[operands32[op32Index++]]; break;
                            case 8: dest = registers[operands64[op64Index++]]; break;
                        }

                        value1 = read64(dest);
                        break;
                }

                instructionPointer = value1;
            }

            break;

        case 0xAB:
            if (getFlagBit(FLAG_CARRY)) {
                resetOpIndexes();

                switch (op1Type) {
                    case reg:
                        value1 = registers[operands8[op8Index++]];
                        break;

                    case imm:
                        switch (op1Size) {
                            case 1: value1 = operands8[op8Index++]; break;
                            case 2: value1 = operands16[op16Index++]; break;
                            case 4: value1 = operands32[op32Index++]; break;
                            case 8: value1 = operands64[op64Index++]; break;
                        }
                        break;

                    case mem_imm:
                        switch (op1Size) {
                            case 1: dest = operands8[op8Index++]; break;
                            case 2: dest = operands16[op16Index++]; break;
                            case 4: dest = operands32[op32Index++]; break;
                            case 8: dest = operands64[op64Index++]; break;
                        }

                        value1 = read64(dest);
                        break;

                    case mem_reg:
                        switch (op1Size) {
                            case 1: dest = registers[operands8[op8Index++]]; break;
                            case 2: dest = registers[operands16[op16Index++]]; break;
                            case 4: dest = registers[operands32[op32Index++]]; break;
                            case 8: dest = registers[operands64[op64Index++]]; break;
                        }

                        value1 = read64(dest);
                        break;
                }

                instructionPointer = value1;
            }

            break;

        case 0xAC:
            if (!getFlagBit(FLAG_CARRY)) {
                resetOpIndexes();

                switch (op1Type) {
                    case reg:
                        value1 = registers[operands8[op8Index++]];
                        break;

                    case imm:
                        switch (op1Size) {
                            case 1: value1 = operands8[op8Index++]; break;
                            case 2: value1 = operands16[op16Index++]; break;
                            case 4: value1 = operands32[op32Index++]; break;
                            case 8: value1 = operands64[op64Index++]; break;
                        }
                        break;

                    case mem_imm:
                        switch (op1Size) {
                            case 1: dest = operands8[op8Index++]; break;
                            case 2: dest = operands16[op16Index++]; break;
                            case 4: dest = operands32[op32Index++]; break;
                            case 8: dest = operands64[op64Index++]; break;
                        }

                        value1 = read64(dest);
                        break;

                    case mem_reg:
                        switch (op1Size) {
                            case 1: dest = registers[operands8[op8Index++]]; break;
                            case 2: dest = registers[operands16[op16Index++]]; break;
                            case 4: dest = registers[operands32[op32Index++]]; break;
                            case 8: dest = registers[operands64[op64Index++]]; break;
                        }

                        value1 = read64(dest);
                        break;
                }

                instructionPointer = value1;
            }

            break;

        case 0xAD:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]];
                    break;

                case imm:
                    value1 = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }

                    value1 = read8(dest);
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read8(dest);
                    break;
            }

            switch (op2Type) {
                case reg:
                    value2 = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                        case 8: value2 = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: dest = operands8[op8Index]; break;
                        case 2: dest = operands16[op16Index]; break;
                        case 4: dest = operands32[op32Index]; break;
                        case 8: dest = operands64[op64Index]; break;
                    }

                    value2 = read64(dest);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: dest = registers[operands8[op8Index]]; break;
                        case 2: dest = registers[operands16[op16Index]]; break;
                        case 4: dest = registers[operands32[op32Index]]; break;
                        case 8: dest = registers[operands64[op64Index]]; break;
                    }

                    value2 = read64(dest);
                    break;
            }

            registers[value1]--;

            if (registers[value1] != 0) {
                instructionPointer = value2;
            }

            break;

        case 0xAE:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]];
                    break;

                case imm:
                    switch (op1Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                        case 8: value1 = operands64[op64Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }

                    value1 = read64(dest);
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read64(dest);
                    break;
            }

            write64(stackPointer - 7, instructionPointer);
            stackPointer -= 8;
            instructionPointer = value1;

            break;

        case 0xAF:
            value1 = read64(stackPointer + 1);
            stackPointer += 8;
            instructionPointer = value1;

            break;

        case 0xB0:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    value1 = registers[operands8[op8Index]] & 0xFF;
                    break;

                case imm:
                    value1 = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index]; break;
                        case 2: dest = operands16[op16Index]; break;
                        case 4: dest = operands32[op32Index]; break;
                        case 8: dest = operands64[op64Index]; break;
                    }

                    value1 = read8(dest);
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index]]; break;
                        case 2: dest = registers[operands16[op16Index]]; break;
                        case 4: dest = registers[operands32[op32Index]]; break;
                        case 8: dest = registers[operands64[op64Index]]; break;
                    }

                    value1 = read8(dest);
                    break;
            }

            write8(stackPointer, value1);
            stackPointer -= 1;

            break;

        case 0xB1:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    value1 = registers[operands8[op8Index]] & 0xFFFF;
                    break;

                case imm:
                    switch (op1Size) {
                        case 1: value1 = operands8[op8Index]; break;
                        case 2: value1 = operands16[op16Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index]; break;
                        case 2: dest = operands16[op16Index]; break;
                        case 4: dest = operands32[op32Index]; break;
                        case 8: dest = operands64[op64Index]; break;
                    }

                    value1 = read16(dest);
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index]]; break;
                        case 2: dest = registers[operands16[op16Index]]; break;
                        case 4: dest = registers[operands32[op32Index]]; break;
                        case 8: dest = registers[operands64[op64Index]]; break;
                    }

                    value1 = read16(dest);
                    break;
            }

            write16(stackPointer - 1, value1);
            stackPointer -= 2;

            break;

        case 0xB2:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    value1 = registers[operands8[op8Index]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op1Size) {
                        case 1: value1 = operands8[op8Index]; break;
                        case 2: value1 = operands16[op16Index]; break;
                        case 4: value1 = operands32[op32Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index]; break;
                        case 2: dest = operands16[op16Index]; break;
                        case 4: dest = operands32[op32Index]; break;
                        case 8: dest = operands64[op64Index]; break;
                    }

                    value1 = read32(dest);
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index]]; break;
                        case 2: dest = registers[operands16[op16Index]]; break;
                        case 4: dest = registers[operands32[op32Index]]; break;
                        case 8: dest = registers[operands64[op64Index]]; break;
                    }

                    value1 = read32(dest);
                    break;
            }

            write32(stackPointer - 3, value1);
            stackPointer -= 4;

            break;

        case 0xB3:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    value1 = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op1Size) {
                        case 1: value1 = operands8[op8Index]; break;
                        case 2: value1 = operands16[op16Index]; break;
                        case 4: value1 = operands32[op32Index]; break;
                        case 8: value1 = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index]; break;
                        case 2: dest = operands16[op16Index]; break;
                        case 4: dest = operands32[op32Index]; break;
                        case 8: dest = operands64[op64Index]; break;
                    }

                    value1 = read64(dest);
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index]]; break;
                        case 2: dest = registers[operands16[op16Index]]; break;
                        case 4: dest = registers[operands32[op32Index]]; break;
                        case 8: dest = registers[operands64[op64Index]]; break;
                    }

                    value1 = read64(dest);
                    break;
            }

            write64(stackPointer - 7, value1);
            stackPointer -= 8;

            break;

        case 0xB4:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index]; break;
                        case 2: dest = operands16[op16Index]; break;
                        case 4: dest = operands32[op32Index]; break;
                        case 8: dest = operands64[op64Index]; break;
                    }

                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index]]; break;
                        case 2: dest = registers[operands16[op16Index]]; break;
                        case 4: dest = registers[operands32[op32Index]]; break;
                        case 8: dest = registers[operands64[op64Index]]; break;
                    }

                    break;
            }

            stackPointer += 1;
            value = read8(stackPointer);

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

        case 0xB5:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index]; break;
                        case 2: dest = operands16[op16Index]; break;
                        case 4: dest = operands32[op32Index]; break;
                        case 8: dest = operands64[op64Index]; break;
                    }

                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index]]; break;
                        case 2: dest = registers[operands16[op16Index]]; break;
                        case 4: dest = registers[operands32[op32Index]]; break;
                        case 8: dest = registers[operands64[op64Index]]; break;
                    }

                    break;
            }

            stackPointer += 2;
            value = read16(stackPointer - 1);

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

        case 0xB6:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index]; break;
                        case 2: dest = operands16[op16Index]; break;
                        case 4: dest = operands32[op32Index]; break;
                        case 8: dest = operands64[op64Index]; break;
                    }

                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index]]; break;
                        case 2: dest = registers[operands16[op16Index]]; break;
                        case 4: dest = registers[operands32[op32Index]]; break;
                        case 8: dest = registers[operands64[op64Index]]; break;
                    }

                    break;
            }

            stackPointer += 4;
            value = read32(stackPointer - 3);

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

        case 0xB7:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index]; break;
                        case 2: dest = operands16[op16Index]; break;
                        case 4: dest = operands32[op32Index]; break;
                        case 8: dest = operands64[op64Index]; break;
                    }

                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index]]; break;
                        case 2: dest = registers[operands16[op16Index]]; break;
                        case 4: dest = registers[operands32[op32Index]]; break;
                        case 8: dest = registers[operands64[op64Index]]; break;
                    }

                    break;
            }

            stackPointer += 8;
            value = read64(stackPointer - 7);

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

        case 0xB8:
            for (int i = 0; i < 64; i++) {
                write64(stackPointer - 7, registers[i]);
                stackPointer -= 8;
            }

            break;

        case 0xB9:
            for (int i = 63; i >= 0; i--) {
                stackPointer += 8;
                registers[i] = read64(stackPointer - 7);
            }

            break;

        case 0xBA:
            write8(stackPointer, flags);
            stackPointer -= 1;

            break;

        case 0xBB:
            stackPointer += 1;
            flags = read8(stackPointer);

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

        case 0x0100:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFF;
                    break;

                case imm:
                    value1 = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read8(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read8(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFF;
                    break;

                case imm:
                    value2 = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read8(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read8(addr);
                    break;
            }

            value = ~(value1 & value2) & 0xFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x0101:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read16(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read16(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read16(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read16(addr);
                    break;
            }

            value = ~(value1 & value2) & 0xFFFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x0102:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read32(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read32(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read32(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read32(addr);
                    break;
            }

            value = ~(value1 & value2) & 0xFFFFFFFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80000000);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x0103:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]];
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                        case 8: value1 = operands64[op64Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read64(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read64(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                        case 8: value2 = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read64(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read64(addr);
                    break;
            }

            value = ~(value1 & value2);
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000000000000000ULL);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x0104:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFF;
                    break;

                case imm:
                    value1 = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read8(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read8(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFF;
                    break;

                case imm:
                    value2 = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read8(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read8(addr);
                    break;
            }

            value = ~(value1 | value2) & 0xFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x0105:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read16(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read16(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read16(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read16(addr);
                    break;
            }

            value = ~(value1 | value2) & 0xFFFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x0106:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read32(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read32(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read32(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read32(addr);
                    break;
            }

            value = ~(value1 | value2) & 0xFFFFFFFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80000000);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x0107:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]];
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                        case 8: value1 = operands64[op64Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read64(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read64(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                        case 8: value2 = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read64(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read64(addr);
                    break;
            }

            value = ~(value1 | value2);
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000000000000000ULL);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x0108:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFF;
                    break;

                case imm:
                    value1 = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read8(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read8(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFF;
                    break;

                case imm:
                    value2 = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read8(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read8(addr);
                    break;
            }

            value = ~(value1 ^ value2) & 0xFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x0109:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read16(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read16(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read16(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read16(addr);
                    break;
            }

            value = ~(value1 ^ value2) & 0xFFFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x010A:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read32(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read32(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read32(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read32(addr);
                    break;
            }

            value = ~(value1 ^ value2) & 0xFFFFFFFF;
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80000000);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x010B:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]];
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                        case 8: value1 = operands64[op64Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read64(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read64(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                        case 8: value2 = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read64(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read64(addr);
                    break;
            }

            value = ~(value1 ^ value2);
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000000000000000ULL);
            setFlagBit(FLAG_CARRY,    false);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x010C:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFF;
                    break;

                case imm:
                    value1 = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read8(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read8(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFF;
                    break;

                case imm:
                    value2 = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read8(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read8(addr);
                    break;
            }

            sh = value2 & 7;
            value = sh == 0 ? value1 : ((value1 << sh) | (value1 >> (8 - sh))) & 0xFF;
            setFlagBit(FLAG_CARRY,    value & 1);
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x010D:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read16(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read16(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read16(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read16(addr);
                    break;
            }

            sh = value2 & 15;
            value = sh == 0 ? value1 : ((value1 << sh) | (value1 >> (16 - sh))) & 0xFFFF;
            setFlagBit(FLAG_CARRY,    value & 1);
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x010E:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read32(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read32(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read32(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read32(addr);
                    break;
            }

            sh = value2 & 31;
            value = sh == 0 ? value1 : ((value1 << sh) | (value1 >> (32 - sh))) & 0xFFFFFFFF;
            setFlagBit(FLAG_CARRY,    value & 1);
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80000000);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x010F:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]];
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                        case 8: value1 = operands64[op64Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read64(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read64(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                        case 8: value2 = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read64(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read64(addr);
                    break;
            }

            sh = value2 & 63;
            value = sh == 0 ? value1 : (value1 << sh) | (value1 >> (64 - sh));
            setFlagBit(FLAG_CARRY,    value & 1);
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000000000000000ULL);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x0110:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFF;
                    break;

                case imm:
                    value1 = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }
                    value1 = read8(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }
                    value1 = read8(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFF;
                    break;

                case imm:
                    value2 = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value2 = read8(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value2 = read8(addr);
                    break;
            }

            sh = value2 & 7;
            value = sh == 0 ? value1 : ((value1 >> sh) | (value1 << (8 - sh))) & 0xFF;
            setFlagBit(FLAG_CARRY,    value & 0x80);
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x0111:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read16(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read16(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read16(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read16(addr);
                    break;
            }

            sh = value2 & 15;
            value = sh == 0 ? value1 : ((value1 >> sh) | (value1 << (16 - sh))) & 0xFFFF;
            setFlagBit(FLAG_CARRY,    value & 0x8000);
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x0112:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read32(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read32(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read32(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read32(addr);
                    break;
            }

            sh = value2 & 31;
            value = sh == 0 ? value1 : ((value1 >> sh) | (value1 << (32 - sh))) & 0xFFFFFFFF;
            setFlagBit(FLAG_CARRY,    value & 0x80000000);
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x80000000);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x0113:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index++]];
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index++]; break;
                        case 2: value1 = operands16[op16Index++]; break;
                        case 4: value1 = operands32[op32Index++]; break;
                        case 8: value1 = operands64[op64Index++]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index++]; break;
                        case 2: addr = operands16[op16Index++]; break;
                        case 4: addr = operands32[op32Index++]; break;
                        case 8: addr = operands64[op64Index++]; break;
                    }

                    value1 = read64(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index++]]; break;
                        case 2: addr = registers[operands16[op16Index++]]; break;
                        case 4: addr = registers[operands32[op32Index++]]; break;
                        case 8: addr = registers[operands64[op64Index++]]; break;
                    }

                    value1 = read64(addr);
                    break;
            }

            switch (op3Type) {
                case reg:
                    value2 = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op3Size) {
                        case 1: value2 = operands8[op8Index]; break;
                        case 2: value2 = operands16[op16Index]; break;
                        case 4: value2 = operands32[op32Index]; break;
                        case 8: value2 = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op3Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value2 = read64(addr);
                    break;

                case mem_reg:
                    switch (op3Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value2 = read64(addr);
                    break;
            }

            sh = value2 & 63;
            value = sh == 0 ? value1 : (value1 >> sh) | (value1 << (64 - sh));
            setFlagBit(FLAG_CARRY,    value & 0x8000000000000000ULL);
            setFlagBit(FLAG_ZERO,     value == 0);
            setFlagBit(FLAG_NEGATIVE, value & 0x8000000000000000ULL);
            setFlagBit(FLAG_OVERFLOW, false);

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

        case 0x0114:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index]] & 0xFF;
                    break;

                case imm:
                    value1 = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value1 = read8(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value1 = read8(addr);
                    break;
            }

            value = __popcnt(value1 & 0xFF);
            setFlagBit(FLAG_ZERO, value == 0);
            setFlagBit(FLAG_CARRY, false);
            setFlagBit(FLAG_OVERFLOW, false);
            setFlagBit(FLAG_NEGATIVE, false);

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

        case 0x0115:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index]] & 0xFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index]; break;
                        case 2: value1 = operands16[op16Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value1 = read16(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value1 = read16(addr);
                    break;
            }

            value = __popcnt(value1 & 0xFFFF);
            setFlagBit(FLAG_ZERO, value == 0);
            setFlagBit(FLAG_CARRY, false);
            setFlagBit(FLAG_OVERFLOW, false);
            setFlagBit(FLAG_NEGATIVE, false);

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

        case 0x0116:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index]; break;
                        case 2: value1 = operands16[op16Index]; break;
                        case 4: value1 = operands32[op32Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value1 = read32(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value1 = read32(addr);
                    break;
            }

            value = __popcnt(value1 & 0xFFFFFFFF);
            setFlagBit(FLAG_ZERO, value == 0);
            setFlagBit(FLAG_CARRY, false);
            setFlagBit(FLAG_OVERFLOW, false);
            setFlagBit(FLAG_NEGATIVE, false);

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

        case 0x0117:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index]; break;
                        case 2: value1 = operands16[op16Index]; break;
                        case 4: value1 = operands32[op32Index]; break;
                        case 8: value1 = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value1 = read64(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value1 = read64(addr);
                    break;
            }

            value = __popcnt64(value1);
            setFlagBit(FLAG_ZERO, value == 0);
            setFlagBit(FLAG_CARRY, false);
            setFlagBit(FLAG_OVERFLOW, false);
            setFlagBit(FLAG_NEGATIVE, false);

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

        case 0x0118:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index]] & 0xFF;
                    break;

                case imm:
                    value1 = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value1 = read8(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value1 = read8(addr);
                    break;
            }

            value = value1 == 0 ? 8 : (__builtin_clz(value1 & 0xFF) - 24);
            setFlagBit(FLAG_ZERO, value == 0);
            setFlagBit(FLAG_CARRY, false);
            setFlagBit(FLAG_OVERFLOW, false);
            setFlagBit(FLAG_NEGATIVE, false);

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

        case 0x0119:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index]] & 0xFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index]; break;
                        case 2: value1 = operands16[op16Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value1 = read16(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value1 = read16(addr);
                    break;
            }

            value = value1 == 0 ? 16 : (__builtin_clz(value1 & 0xFFFF) - 16);
            setFlagBit(FLAG_ZERO, value == 0);
            setFlagBit(FLAG_CARRY, false);
            setFlagBit(FLAG_OVERFLOW, false);
            setFlagBit(FLAG_NEGATIVE, false);

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

        case 0x011A:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index]; break;
                        case 2: value1 = operands16[op16Index]; break;
                        case 4: value1 = operands32[op32Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value1 = read32(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value1 = read32(addr);
                    break;
            }

            value = value1 == 0 ? 32 : __builtin_clz(value1);
            setFlagBit(FLAG_ZERO, value == 0);
            setFlagBit(FLAG_CARRY, false);
            setFlagBit(FLAG_OVERFLOW, false);
            setFlagBit(FLAG_NEGATIVE, false);

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

        case 0x011B:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index]; break;
                        case 2: value1 = operands16[op16Index]; break;
                        case 4: value1 = operands32[op32Index]; break;
                        case 8: value1 = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value1 = read64(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value1 = read64(addr);
                    break;
            }

            value = value1 == 0 ? 64 : __builtin_clzll(value1);
            setFlagBit(FLAG_ZERO, value == 0);
            setFlagBit(FLAG_CARRY, false);
            setFlagBit(FLAG_OVERFLOW, false);
            setFlagBit(FLAG_NEGATIVE, false);

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

        case 0x011C:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index]] & 0xFF;
                    break;

                case imm:
                    value1 = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value1 = read8(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value1 = read8(addr);
                    break;
            }

            value = value1 == 0 ? 8 : __builtin_ctz(value1 & 0xFF);
            setFlagBit(FLAG_ZERO, value == 0);
            setFlagBit(FLAG_CARRY, false);
            setFlagBit(FLAG_OVERFLOW, false);
            setFlagBit(FLAG_NEGATIVE, false);

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

        case 0x011D:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index]] & 0xFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index]; break;
                        case 2: value1 = operands16[op16Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value1 = read16(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value1 = read16(addr);
                    break;
            }

            value = value1 == 0 ? 16 : __builtin_ctz(value1 & 0xFFFF);
            setFlagBit(FLAG_ZERO, value == 0);
            setFlagBit(FLAG_CARRY, false);
            setFlagBit(FLAG_OVERFLOW, false);
            setFlagBit(FLAG_NEGATIVE, false);

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

        case 0x011E:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index]; break;
                        case 2: value1 = operands16[op16Index]; break;
                        case 4: value1 = operands32[op32Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value1 = read32(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value1 = read32(addr);
                    break;
            }

            value = value1 == 0 ? 32 : __builtin_ctz(value1 & 0xFFFFFFFF);
            setFlagBit(FLAG_ZERO, value == 0);
            setFlagBit(FLAG_CARRY, false);
            setFlagBit(FLAG_OVERFLOW, false);
            setFlagBit(FLAG_NEGATIVE, false);

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

        case 0x011F:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index]; break;
                        case 2: value1 = operands16[op16Index]; break;
                        case 4: value1 = operands32[op32Index]; break;
                        case 8: value1 = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value1 = read64(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value1 = read64(addr);
                    break;
            }

            value = value1 == 0 ? 64 : __builtin_ctzll(value1);
            setFlagBit(FLAG_ZERO, value == 0);
            setFlagBit(FLAG_CARRY, false);
            setFlagBit(FLAG_OVERFLOW, false);
            setFlagBit(FLAG_NEGATIVE, false);

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

        case 0x0120:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index]] & 0xFF;
                    break;

                case imm:
                    value1 = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value1 = read8(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value1 = read8(addr);
                    break;
            }

            value = (value1 == 0) ? 0 : __builtin_ctz(value1);
            setFlagBit(FLAG_ZERO, value1 == 0);
            setFlagBit(FLAG_CARRY, false);
            setFlagBit(FLAG_OVERFLOW, false);
            setFlagBit(FLAG_NEGATIVE, false);

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

        case 0x0121:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index]] & 0xFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index]; break;
                        case 2: value1 = operands16[op16Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value1 = read16(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value1 = read16(addr);
                    break;
            }

            value = (value1 == 0) ? 0 : __builtin_ctz(value1);
            setFlagBit(FLAG_ZERO, value1 == 0);
            setFlagBit(FLAG_CARRY, false);
            setFlagBit(FLAG_OVERFLOW, false);
            setFlagBit(FLAG_NEGATIVE, false);

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

        case 0x0122:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index]; break;
                        case 2: value1 = operands16[op16Index]; break;
                        case 4: value1 = operands32[op32Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value1 = read32(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value1 = read32(addr);
                    break;
            }

            value = (value1 == 0) ? 0 : __builtin_ctz(value1);
            setFlagBit(FLAG_ZERO, value1 == 0);
            setFlagBit(FLAG_CARRY, false);
            setFlagBit(FLAG_OVERFLOW, false);
            setFlagBit(FLAG_NEGATIVE, false);

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

        case 0x0123:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index]; break;
                        case 2: value1 = operands16[op16Index]; break;
                        case 4: value1 = operands32[op32Index]; break;
                        case 8: value1 = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value1 = read64(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value1 = read64(addr);
                    break;
            }

            value = (value1 == 0) ? 0 : __builtin_ctzll(value1);
            setFlagBit(FLAG_ZERO, value1 == 0);
            setFlagBit(FLAG_CARRY, false);
            setFlagBit(FLAG_OVERFLOW, false);
            setFlagBit(FLAG_NEGATIVE, false);

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

        case 0x0124:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++];
                    break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }
                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index]] & 0xFF;
                    break;

                case imm:
                    value1 = operands8[op8Index];
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }
                    value1 = read8(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }
                    value1 = read8(addr);
                    break;
            }

            value = (value1 == 0) ? 0 : 7 - (__builtin_clz(value1) - 24);
            setFlagBit(FLAG_ZERO, value1 == 0);
            setFlagBit(FLAG_CARRY, false);
            setFlagBit(FLAG_OVERFLOW, false);
            setFlagBit(FLAG_NEGATIVE, false);

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

        case 0x0125:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index]] & 0xFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index]; break;
                        case 2: value1 = operands16[op16Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value1 = read16(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value1 = read16(addr);
                    break;
            }

            value = (value1 == 0) ? 0 : 15 - (__builtin_clz(value1) - 16);
            setFlagBit(FLAG_ZERO, value1 == 0);
            setFlagBit(FLAG_CARRY, false);
            setFlagBit(FLAG_OVERFLOW, false);
            setFlagBit(FLAG_NEGATIVE, false);

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

        case 0x0126:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index]] & 0xFFFFFFFF;
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index]; break;
                        case 2: value1 = operands16[op16Index]; break;
                        case 4: value1 = operands32[op32Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value1 = read32(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value1 = read32(addr);
                    break;
            }

            value = (value1 == 0) ? 0 : 31 - __builtin_clz(value1);
            setFlagBit(FLAG_ZERO, value1 == 0);
            setFlagBit(FLAG_CARRY, false);
            setFlagBit(FLAG_OVERFLOW, false);
            setFlagBit(FLAG_NEGATIVE, false);

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

        case 0x0127:
            resetOpIndexes();

            switch (op1Type) {
                case reg:
                    dest = operands8[op8Index++]; break;

                case mem_imm:
                    switch (op1Size) {
                        case 1: dest = operands8[op8Index++]; break;
                        case 2: dest = operands16[op16Index++]; break;
                        case 4: dest = operands32[op32Index++]; break;
                        case 8: dest = operands64[op64Index++]; break;
                    }
                    
                    break;

                case mem_reg:
                    switch (op1Size) {
                        case 1: dest = registers[operands8[op8Index++]]; break;
                        case 2: dest = registers[operands16[op16Index++]]; break;
                        case 4: dest = registers[operands32[op32Index++]]; break;
                        case 8: dest = registers[operands64[op64Index++]]; break;
                    }

                    break;
            }

            switch (op2Type) {
                case reg:
                    value1 = registers[operands8[op8Index]];
                    break;

                case imm:
                    switch (op2Size) {
                        case 1: value1 = operands8[op8Index]; break;
                        case 2: value1 = operands16[op16Index]; break;
                        case 4: value1 = operands32[op32Index]; break;
                        case 8: value1 = operands64[op64Index]; break;
                    }
                    break;

                case mem_imm:
                    switch (op2Size) {
                        case 1: addr = operands8[op8Index]; break;
                        case 2: addr = operands16[op16Index]; break;
                        case 4: addr = operands32[op32Index]; break;
                        case 8: addr = operands64[op64Index]; break;
                    }

                    value1 = read64(addr);
                    break;

                case mem_reg:
                    switch (op2Size) {
                        case 1: addr = registers[operands8[op8Index]]; break;
                        case 2: addr = registers[operands16[op16Index]]; break;
                        case 4: addr = registers[operands32[op32Index]]; break;
                        case 8: addr = registers[operands64[op64Index]]; break;
                    }

                    value1 = read64(addr);
                    break;
            }

            value = (value1 == 0) ? 0 : 63 - __builtin_clzll(value1);
            setFlagBit(FLAG_ZERO, value1 == 0);
            setFlagBit(FLAG_CARRY, false);
            setFlagBit(FLAG_OVERFLOW, false);
            setFlagBit(FLAG_NEGATIVE, false);

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
        return 0;
    }
}

uint16_t CPU::read16(uint64_t address) {
    if (address >= Motherboard::ROM_START && address + 1 <= Motherboard::ROM_END) {
        return rom->read16(address);
    }
    
    else if (address >= Motherboard::RAM_START && address + 1 <= Motherboard::RAM_END) {
        return memory->read16(address);
    }
    
    else {
        error("CP04AOOB", "Absolute address: " + std::to_string(address));
        return 0;
    }
}

uint32_t CPU::read32(uint64_t address) {
    if (address >= Motherboard::ROM_START && address + 3 <= Motherboard::ROM_END) {
        return rom->read32(address);
    }
    
    else if (address >= Motherboard::RAM_START && address + 3 <= Motherboard::RAM_END) {
        return memory->read32(address);
    }
    
    else {
        error("CP05AOOB", "Absolute address: " + std::to_string(address));
        return 0;
    }
}

uint64_t CPU::read64(uint64_t address) {
    if (address >= Motherboard::ROM_START && address + 7 <= Motherboard::ROM_END) {
        return rom->read64(address);
    }
    
    else if (address >= Motherboard::RAM_START && address + 7 <= Motherboard::RAM_END) {
        return memory->read64(address);
    }
    
    else {
        error("CP06AOOB", "Absolute address: " + std::to_string(address));
        return 0;
    }
}

std::vector<uint8_t> CPU::readBytesVector(uint64_t start, size_t length) {
    if (start >= Motherboard::ROM_START && start + length - 1 <= Motherboard::ROM_END) {
        return rom->readBytesVector(start, length);
    }
    
    else if (start >= Motherboard::RAM_START && start + length - 1 <= Motherboard::RAM_END) {
        return memory->readBytesVector(start, length);
    }
    
    else {
        error("CP07AOOB", "Absolute address: " + std::to_string(start));
        return {0};
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

void CPU::write16(uint64_t address, uint16_t value) {
    if (address >= Motherboard::ROM_START && address + 1 <= Motherboard::ROM_END) {
        error("CP01CWTR", "Absolute address: " + std::to_string(address));
    }
    
    else if (address >= Motherboard::RAM_START && address + 1 <= Motherboard::RAM_END) {
        memory->write16(address, value);
    }
    
    else {
        error("CP09AOOB", "Absolute address: " + std::to_string(address));
    }
}

void CPU::write32(uint64_t address, uint32_t value) {
    if (address >= Motherboard::ROM_START && address + 3 <= Motherboard::ROM_END) {
        error("CP01CWTR", "Absolute address: " + std::to_string(address));
    }
    
    else if (address >= Motherboard::RAM_START && address + 3 <= Motherboard::RAM_END) {
        memory->write32(address, value);
    }

    else {
        error("CP10AOOB", "Absolute address: " + std::to_string(address));
    }
}

void CPU::write64(uint64_t address, uint64_t value) {
    if (address >= Motherboard::ROM_START && address + 7 <= Motherboard::ROM_END) {
        error("CP01CWTR", "Absolute address: " + std::to_string(address));
    }
    
    else if (address >= Motherboard::RAM_START && address + 7 <= Motherboard::RAM_END) {
        memory->write64(address, value);
    }
    
    else {
        error("CP11AOOB", "Absolute address: " + std::to_string(address));
    }
}

void CPU::writeBytesVector(uint64_t start, const std::vector<uint8_t>& data) {
    if (start >= Motherboard::ROM_START && start + data.size() - 1 <= Motherboard::ROM_END) {
        error("CP02CWTR", "Absolute address: " + std::to_string(start));
    }
    
    else if (start >= Motherboard::RAM_START && start + data.size() - 1 <= Motherboard::RAM_END) {
        memory->writeBytesVector(start, data);
    }
    
    else {
        error("CP12AOOB", "Absolute address: " + std::to_string(start));
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