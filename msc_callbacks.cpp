#include "common.h"
#include "flash.h"
#include <algorithm>
#include <cstdint>
#include <string>

void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8],
                        uint8_t product_id[16], uint8_t product_rev[4]) {
  std::string vid = "Stefan";
  std::string pid = "Mass Storage";
  std::string rev = "1.0";

  std::copy(vid.begin(), vid.end(), vendor_id);
  std::copy(pid.begin(), pid.end(), product_id);
  std::copy(rev.begin(), rev.end(), product_rev);
}

bool tud_msc_test_unit_ready_cb(uint8_t lun) {
  (void)lun;
  return true;
}

void tud_msc_capacity_cb(uint8_t lun, uint32_t *block_count,
                         uint16_t *block_size) {
  *block_count = DISK_BLOCK_NUM;
  *block_size = DISK_BLOCK_SIZE;
}

int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset,
                          void *buffer, uint32_t bufsize) {
  (void)lun;
  if (lba >= DISK_BLOCK_NUM) {
    return -1;
  }

  Params params = {
      .lba = lba, .offset = offset, .buffer = buffer, .bufsize = bufsize};

  auto result = flash_read(&params);
  if (result == PICO_OK) {
    return static_cast<uint32_t>(bufsize);
  }

  return -1;
}


int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset,
                           uint8_t *buffer, uint32_t bufsize) {
  (void)lun;
  if (lba >= DISK_BLOCK_NUM) {
    return -1;
  }

  Params params = {
      .lba = lba, .offset = offset, .buffer = buffer, .bufsize = bufsize};

  auto result = flash_write(&params);
  if (result == PICO_OK) {
    return static_cast<uint32_t>(bufsize);
  }

  return -1;
}
