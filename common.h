#pragma once
#include <cstdint>
#include <hardware/flash.h>

constexpr uint32_t SECTORS = 32;
constexpr uint32_t RESERVED_FLASH_OFFSET = PICO_FLASH_SIZE_BYTES - SECTORS * FLASH_SECTOR_SIZE; // last 128KiB
constexpr uint32_t MAX_SECTORS_PER_WRITE = 4;

constexpr uint32_t DISK_BLOCK_SIZE = 512;
constexpr uint32_t DISK_BLOCK_NUM = (FLASH_SECTOR_SIZE * SECTORS) / DISK_BLOCK_SIZE;

struct Params {
  uint32_t lba;
  uint32_t offset;
  void *buffer;
  uint32_t bufsize;
};
