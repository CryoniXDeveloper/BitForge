#include "motherboard.h"
#include "cpu.h"
#include <iostream>

Motherboard::Motherboard() {
    RAM_END = RAM_START + ram.size() - 1;
    rom.loadFromFile("rom.bin");

    cpu = new CPU();
    cpu->motherboard = this;
    cpu->rom = &rom;
    cpu->ram = &ram;
    cpu->instructionPointer = ROM_START;
    cpu->stackPointer = RAM_END;
}

Motherboard::~Motherboard() {
    delete cpu;
}

void Motherboard::run() {
    cpu->start();
}
