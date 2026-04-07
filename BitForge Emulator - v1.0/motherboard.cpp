#include "motherboard.h"
#include "cpu.h"
#include <iostream>

Motherboard::Motherboard() {
    rom.loadFromFile("rom.bin");

    cpu = new CPU();
    cpu->motherboard = this;
    cpu->rom = &rom;
    cpu->memory = &memory;
    cpu->instructionPointer = ROM_START;
    cpu->stackPointer = STACK_START;
    cpu->basePointer = STACK_START;
}

Motherboard::~Motherboard() {
    delete cpu;
}

void Motherboard::run() {
    cpu->start();
}
