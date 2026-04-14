#include "storage.h"
#include <iostream>
#include <cstring>

Storage storage;

Storage::Storage() : disk_size(0), num_blocks(0), fat_offset(0), data_area_offset(0) {
    disk.open("disk.bin", std::ios::in | std::ios::out | std::ios::binary);
    if (!disk) {
        std::cerr << "ERROR: could not open disk.bin\n";
        return;
    }

    disk.seekg(0, std::ios::end);
    disk_size = static_cast<uint64_t>(disk.tellg());

    num_blocks       = (disk_size - METADATA_SIZE) / (BLOCK_SIZE + 8);
    fat_offset       = METADATA_SIZE;
    data_area_offset = fat_offset + num_blocks * 8;

    loadFAT();
}

Storage::~Storage() {
    if (disk.is_open()) disk.close();
}

void Storage::rawwrite(uint64_t offset, const void* src, uint64_t bytes) {
    if (offset + bytes > disk_size) return;
    disk.seekp(offset);
    disk.write(reinterpret_cast<const char*>(src), bytes);
    disk.flush();
}

uint64_t Storage::rawread(uint64_t offset, uint64_t bytes) {
    if (offset + bytes > disk_size) return 0;
    uint64_t value = 0;
    disk.seekg(offset);
    disk.read(reinterpret_cast<char*>(&value), bytes);
    return value;
}

void Storage::loadFAT() {
    fat.resize(num_blocks);
    disk.seekg(fat_offset);
    disk.read(reinterpret_cast<char*>(fat.data()), num_blocks * 8);
}

void Storage::flushFAT() {
    disk.seekp(fat_offset);
    disk.write(reinterpret_cast<const char*>(fat.data()), num_blocks * 8);
    disk.flush();
}

uint64_t Storage::blockToOffset(uint64_t blockIndex) {
    return data_area_offset + blockIndex * BLOCK_SIZE;
}

uint64_t Storage::allocateBlocks(uint64_t numBlocks) {
    std::vector<uint64_t> found;
    for (uint64_t i = 0; i < num_blocks && found.size() < numBlocks; i++) {
        if (fat[i] == FAT_FREE) found.push_back(i);
    }
    if (found.size() < numBlocks) return FAT_END;

    for (uint64_t i = 0; i < found.size() - 1; i++)
        fat[found[i]] = found[i + 1];
    fat[found.back()] = FAT_END;

    flushFAT();
    return found[0];
}

void Storage::freeBlocks(uint64_t firstBlock) {
    uint64_t current = firstBlock;
    while (current != FAT_END && current < num_blocks) {
        uint64_t next = fat[current];
        fat[current] = FAT_FREE;
        current = next;
    }
    flushFAT();
}

bool Storage::findNameEntry(const std::string& name, uint64_t& nameOffset, uint64_t& dataIndex) {
    for (uint64_t i = 0; i < MAX_FILES; i++) {
        uint64_t offset = 8 + i * NAME_ENTRY_SIZE;
        char buf[64] = {};
        disk.seekg(offset);
        disk.read(buf, 64);
        if (buf[0] == '\0') continue;
        if (std::string(buf, strnlen(buf, 64)) == name) {
            nameOffset = offset;
            dataIndex  = rawread(offset + 64, 8);
            return true;
        }
    }
    return false;
}

uint64_t Storage::findFreeNameSlot() {
    for (uint64_t i = 0; i < MAX_FILES; i++) {
        uint64_t offset = 8 + i * NAME_ENTRY_SIZE;
        uint8_t first = static_cast<uint8_t>(rawread(offset, 1));
        if (first == 0x00) return offset;
    }
    return UINT64_MAX;
}

uint64_t Storage::findFreeDataSlot() {
    uint64_t base = 8 + NAME_TABLE_SIZE;
    for (uint64_t i = 0; i < MAX_FILES; i++) {
        uint64_t offset = base + i * DATA_ENTRY_SIZE;
        uint64_t firstBlock = rawread(offset, 8);
        if (firstBlock == DATA_ENTRY_FREE) return i;
    }
    return UINT64_MAX;
}

uint8_t  Storage::readbytes8 (uint64_t offset) { return static_cast<uint8_t> (rawread(offset, 1)); }
uint16_t Storage::readbytes16(uint64_t offset) { return static_cast<uint16_t>(rawread(offset, 2)); }
uint32_t Storage::readbytes32(uint64_t offset) { return static_cast<uint32_t>(rawread(offset, 4)); }
uint64_t Storage::readbytes64(uint64_t offset) { return                        rawread(offset, 8);  }

void Storage::writebytes8 (uint64_t offset, uint8_t  value) { rawwrite(offset, &value, 1); }
void Storage::writebytes16(uint64_t offset, uint16_t value) { rawwrite(offset, &value, 2); }
void Storage::writebytes32(uint64_t offset, uint32_t value) { rawwrite(offset, &value, 4); }
void Storage::writebytes64(uint64_t offset, uint64_t value) { rawwrite(offset, &value, 8); }

void Storage::mvtram(uint64_t disk_address, uint64_t ram_address, uint64_t length) {
    if (disk_address + length > disk_size) return;
    std::vector<uint8_t> buffer(length);
    disk.seekg(disk_address);
    disk.read(reinterpret_cast<char*>(buffer.data()), length);
    memory.writeBytesVector(ram_address, buffer);
}

void Storage::mvtdisk(uint64_t ram_address, uint64_t disk_address, uint64_t length) {
    if (disk_address + length > disk_size) return;
    std::vector<uint8_t> buffer = memory.readBytesVector(ram_address, length);
    disk.seekp(disk_address);
    disk.write(reinterpret_cast<const char*>(buffer.data()), length);
    disk.flush();
}

std::optional<uint64_t> Storage::findfile(const std::string& name) {
    uint64_t nameOffset, dataIndex;
    if (!findNameEntry(name, nameOffset, dataIndex)) return std::nullopt;
    uint64_t dataOffset = 8 + NAME_TABLE_SIZE + dataIndex * DATA_ENTRY_SIZE;
    uint64_t firstBlock = rawread(dataOffset, 8);
    return blockToOffset(firstBlock);
}

bool Storage::mkfile(const std::string& name, uint64_t size) {
    if (name.empty() || name.size() > 63) return false;

    uint64_t nameOffset, dataIndex;
    if (findNameEntry(name, nameOffset, dataIndex)) return false;

    uint64_t freeNameSlot = findFreeNameSlot();
    if (freeNameSlot == UINT64_MAX) return false;

    uint64_t freeDataSlot = findFreeDataSlot();
    if (freeDataSlot == UINT64_MAX) return false;

    uint64_t numBlocks = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    uint64_t firstBlock = allocateBlocks(numBlocks);
    if (firstBlock == FAT_END) return false;

    char nameBuf[64] = {};
    strncpy(nameBuf, name.c_str(), 63);
    rawwrite(freeNameSlot,      nameBuf,     64);
    rawwrite(freeNameSlot + 64, &freeDataSlot, 8);

    uint64_t dataOffset = 8 + NAME_TABLE_SIZE + freeDataSlot * DATA_ENTRY_SIZE;
    rawwrite(dataOffset,     &firstBlock, 8);
    rawwrite(dataOffset + 8, &size,       8);

    uint64_t count = rawread(0, 8) + 1;
    rawwrite(0, &count, 8);

    return true;
}

bool Storage::delfile(const std::string& name) {
    uint64_t nameOffset, dataIndex;
    if (!findNameEntry(name, nameOffset, dataIndex)) return false;

    uint64_t dataOffset = 8 + NAME_TABLE_SIZE + dataIndex * DATA_ENTRY_SIZE;
    uint64_t firstBlock = rawread(dataOffset, 8);

    freeBlocks(firstBlock);

    char empty[64] = {};
    rawwrite(nameOffset, empty, 64);

    uint64_t free_sentinel = DATA_ENTRY_FREE;
    uint64_t zero = 0;
    rawwrite(dataOffset,     &free_sentinel, 8);
    rawwrite(dataOffset + 8, &zero,          8);

    uint64_t count = rawread(0, 8) - 1;
    rawwrite(0, &count, 8);

    return true;
}

uint64_t Storage::filesize(const std::string& name) {
    uint64_t nameOffset, dataIndex;
    if (!findNameEntry(name, nameOffset, dataIndex)) return 0;
    return rawread(8 + NAME_TABLE_SIZE + dataIndex * DATA_ENTRY_SIZE + 8, 8);
}

bool Storage::fileexists(const std::string& name) {
    uint64_t nameOffset, dataIndex;
    return findNameEntry(name, nameOffset, dataIndex);
}

bool Storage::renamefile(const std::string& oldName, const std::string& newName) {
    if (newName.empty() || newName.size() > 63) return false;
    uint64_t nameOffset, dataIndex;
    if (!findNameEntry(oldName, nameOffset, dataIndex)) return false;
    char nameBuf[64] = {};
    strncpy(nameBuf, newName.c_str(), 63);
    rawwrite(nameOffset, nameBuf, 64);
    return true;
}

uint64_t Storage::disksize() const { return disk_size; }