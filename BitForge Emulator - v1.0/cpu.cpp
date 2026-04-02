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
        cycles++;
    }
 
    CPURunTime = timer.end();

    std::cout << std::fixed << std::setprecision(10)
          << "CPU Finished in " << cycles
          << " cycles in " << CPURunTime << " seconds.\n";

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
    operandsVector.clear();

    op1Size = 0;
    op2Size = 0;
    op3Size = 0;

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

            if (opcode == 0x14) {
                if (op2Size == 0) {
                    op2Size = read16(instructionPointer);
                }
                
                else {
                    op2VectorSize = read16(instructionPointer);
                }

                instructionPointer += 2;
            }

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

        case 0x15:
        case 0x16:
        case 0x17:
        case 0x18:
        case 0x19:
        case 0x1A:
        case 0x1B:
        case 0x1C:
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

        case 0xFD:
            break;

        case 0xFE:
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
            setFlagBit(FLAG_OVERFLOW, (int8_t)value != (int8_t)value1 + (int8_t)value2);

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
            setFlagBit(FLAG_OVERFLOW, (int16_t)value != (int16_t)value1 + (int16_t)value2);

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
            setFlagBit(FLAG_OVERFLOW, (int32_t)value != (int32_t)value1 + (int32_t)value2);

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
            setFlagBit(FLAG_OVERFLOW, (int64_t)value != (int64_t)value1 + (int64_t)value2);

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