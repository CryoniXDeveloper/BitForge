#pragma once
#include <cstdint>
#include <fstream>
#include <string>
#include <optional>
#include <vector>
#include "ram.h"

static constexpr uint64_t MAX_FILES        = 100000;
static constexpr uint64_t NAME_ENTRY_SIZE  = 72;
static constexpr uint64_t NAME_TABLE_SIZE  = MAX_FILES * NAME_ENTRY_SIZE;
static constexpr uint64_t DATA_ENTRY_SIZE  = 16;
static constexpr uint64_t DATA_TABLE_SIZE  = MAX_FILES * DATA_ENTRY_SIZE;
static constexpr uint64_t BLOCK_SIZE       = 128;
static constexpr uint64_t METADATA_SIZE    = 8 + NAME_TABLE_SIZE + DATA_TABLE_SIZE;
static constexpr uint64_t FAT_FREE         = 0;
static constexpr uint64_t FAT_END          = UINT64_MAX;
static constexpr uint64_t DATA_ENTRY_FREE  = UINT64_MAX;

class Storage {
public:
    Storage();
    ~Storage();

    uint8_t  readbytes8 (uint64_t offset);
    uint16_t readbytes16(uint64_t offset);
    uint32_t readbytes32(uint64_t offset);
    uint64_t readbytes64(uint64_t offset);

    void writebytes8 (uint64_t offset, uint8_t  value);
    void writebytes16(uint64_t offset, uint16_t value);
    void writebytes32(uint64_t offset, uint32_t value);
    void writebytes64(uint64_t offset, uint64_t value);

    void mvtram (uint64_t disk_address, uint64_t ram_address, uint64_t length);
    void mvtdisk(uint64_t ram_address,  uint64_t disk_address, uint64_t length);

    std::optional<uint64_t> findfile  (const std::string& name);
    bool                    mkfile    (const std::string& name, uint64_t size);
    bool                    delfile   (const std::string& name);
    uint64_t                filesize  (const std::string& name);
    bool                    fileexists(const std::string& name);
    bool                    renamefile(const std::string& oldName, const std::string& newName);

    uint64_t disksize() const;

private:
    std::fstream         disk;
    uint64_t             disk_size;
    uint64_t             num_blocks;
    uint64_t             fat_offset;
    uint64_t             data_area_offset;
    std::vector<uint64_t> fat;

    void     rawwrite(uint64_t offset, const void* src, uint64_t bytes);
    uint64_t rawread (uint64_t offset, uint64_t bytes);

    void     flushFAT();
    void     loadFAT();

    bool     findNameEntry(const std::string& name, uint64_t& nameOffset, uint64_t& dataIndex);
    uint64_t findFreeNameSlot();
    uint64_t findFreeDataSlot();
    uint64_t allocateBlocks(uint64_t numBlocks);
    void     freeBlocks(uint64_t firstBlock);

    uint64_t blockToOffset(uint64_t blockIndex);
};

extern Storage storage;