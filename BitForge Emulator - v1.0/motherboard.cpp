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

uint8_t Motherboard::readPort8(uint16_t port) {
    return ioPorts[port];
}

uint16_t Motherboard::readPort16(uint16_t port) {
    if (port > IO_PORT_COUNT - 2) {
        return 0;
    }

    return static_cast<uint16_t>(ioPorts[port]) |
        (static_cast<uint16_t>(ioPorts[port + 1]) << 8);
}

uint32_t Motherboard::readPort32(uint16_t port) {
    if (port > IO_PORT_COUNT - 4) {
        return 0;
    }

    return static_cast<uint32_t>(ioPorts[port]) |
        (static_cast<uint32_t>(ioPorts[port + 1]) << 8) |
        (static_cast<uint32_t>(ioPorts[port + 2]) << 16) |
        (static_cast<uint32_t>(ioPorts[port + 3]) << 24);
}

uint64_t Motherboard::readPort64(uint16_t port) {
    if (port > IO_PORT_COUNT - 8) {
        return 0;
    }

    return static_cast<uint64_t>(ioPorts[port]) |
          (static_cast<uint64_t>(ioPorts[port + 1]) << 8) |
          (static_cast<uint64_t>(ioPorts[port + 2]) << 16) |
          (static_cast<uint64_t>(ioPorts[port + 3]) << 24) |
          (static_cast<uint64_t>(ioPorts[port + 4]) << 32) |
          (static_cast<uint64_t>(ioPorts[port + 5]) << 40) |
          (static_cast<uint64_t>(ioPorts[port + 6]) << 48) |
          (static_cast<uint64_t>(ioPorts[port + 7]) << 56);
}

void Motherboard::writePort8(uint16_t port, uint8_t value) {
    ioPorts[port] = value;
}

void Motherboard::writePort16(uint16_t port, uint16_t value) {
    if (port > IO_PORT_COUNT - 2) { 
        return;
    }

    for (int i = 0; i < 2; i++) ioPorts[port + i] = (value >> (8 * i)) & 0xFF;
}

void Motherboard::writePort32(uint16_t port, uint32_t value) {
    if (port > IO_PORT_COUNT - 4) {
        return;
    }

    for (int i = 0; i < 4; i++) ioPorts[port + i] = (value >> (8 * i)) & 0xFF;
}

void Motherboard::writePort64(uint16_t port, uint64_t value) {
    if (port > IO_PORT_COUNT - 8) {
        return;
    }
    
    for (int i = 0; i < 8; i++) ioPorts[port + i] = (value >> (8 * i)) & 0xFF;
}

void Motherboard::run() {
    cpu->start();
}
