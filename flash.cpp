#include "flash.h"
#include <algorithm>
#include <pico/flash.h>

void read(void *data);
void write(void *data);

int flash_read(Params *data) {
  return flash_safe_execute(read, static_cast<void *>(&data), 1000);
}

int flash_write(Params *data) {
  return flash_safe_execute(&write, static_cast<void *>(&data), 1000);
}

void read(void *data) {
  auto params = static_cast<Params *>(data);
  uint8_t *flash_start = (uint8_t *)(XIP_BASE + RESERVED_FLASH_OFFSET);

  uint8_t const *addr =
      flash_start + params->lba * DISK_BLOCK_SIZE + params->offset;
  auto *dst = static_cast<uint8_t *>(params->buffer);
  std::copy(addr, addr + params->bufsize, dst);
}
void write(void *data) {
  auto params = static_cast<Params *>(data);
  uint8_t *flash_start = (uint8_t *)(XIP_BASE + RESERVED_FLASH_OFFSET);

  auto startSector =
      ((params->lba * DISK_BLOCK_SIZE) + params->offset) / FLASH_SECTOR_SIZE;

  auto endSector =
      ((params->lba * DISK_BLOCK_SIZE) + params->offset + params->bufsize) /
      FLASH_SECTOR_SIZE;

  if (1 + endSector - startSector > MAX_SECTORS_PER_WRITE) {
    abort();
  }

  uint8_t buf[FLASH_SECTOR_SIZE * (1 + endSector - startSector)];
  std::copy_n(flash_start + (startSector * FLASH_SECTOR_SIZE),
              FLASH_SECTOR_SIZE * (1 + endSector - startSector), buf);

  flash_range_erase(RESERVED_FLASH_OFFSET + (startSector * FLASH_SECTOR_SIZE),
                    (1 + endSector - startSector) * FLASH_SECTOR_SIZE);

  uint8_t *addr = buf + (params->lba * DISK_BLOCK_SIZE + params->offset) %
                            FLASH_SECTOR_SIZE;
  std::copy_n((uint8_t *)params->buffer, params->bufsize, addr);
  // program sectors
  flash_range_program(RESERVED_FLASH_OFFSET + (startSector * FLASH_SECTOR_SIZE),
                      buf, FLASH_SECTOR_SIZE * (1 + endSector - startSector));
}
