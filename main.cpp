#include "common/tusb_types.h"
#include "hardware/flash.h" // FLASH_SECTOR_SIZE
#include "tusb_config.h"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <hardware/regs/addressmap.h>
#include <pico/flash.h>
#include <pico/stdlib.h>
#include <pico/time.h>
#include <string>
#include <tusb.h>
constexpr uint32_t SECTORS = 32;
constexpr uint32_t RESERVED_FLASH_OFFSET =
    PICO_FLASH_SIZE_BYTES - SECTORS * FLASH_SECTOR_SIZE; // last 128KiB
constexpr uint32_t MAX_SECTORS_PER_WRITE = 4;

/* A combination of interfaces must have a unique product id, since PC will save
 * device driver after the first plug. Same VID/PID with different interface e.g
 * MSC (first), then CDC (later) will possibly cause system error on PC.
 *
 * Auto ProductID layout's Bitmap:
 *   [MSB]         HID | MSC | CDC          [LSB]
 */
#define _PID_MAP(itf, n) ((CFG_TUD_##itf) << (n))
#define USB_PID                                                                \
  (0x4000 | _PID_MAP(CDC, 0) | _PID_MAP(MSC, 1) | _PID_MAP(HID, 2) |           \
   _PID_MAP(MIDI, 3) | _PID_MAP(VENDOR, 4))

#define USB_VID 0xBABE
#define USB_BCD 0x0200

tusb_desc_device_t dev = {
    .bLength =
        sizeof(tusb_desc_device_t),      ///< Size of this descriptor in bytes.
    .bDescriptorType = TUSB_DESC_DEVICE, ///< DEVICE Descriptor Type.
    .bcdUSB = USB_BCD, ///< BUSB Specification Release Number in Binary-Coded
                       ///< Decimal (i.e., 2.10 is 210H).

    .bDeviceClass = 0,    ///< Class code (assigned by the USB-IF).
    .bDeviceSubClass = 0, ///< Subclass code (assigned by the USB-IF).
    .bDeviceProtocol = 0, ///< Protocol code (assigned by the USB-IF).
    .bMaxPacketSize0 =
        CFG_TUD_ENDPOINT0_SIZE, ///< Maximum packet size for endpoint zero (only
                                ///< 8, 16, 32, or 64 are valid). For HS devices
                                ///< is fixed to 64.

    .idVendor = USB_VID,  ///< Vendor ID (assigned by the USB-IF).
    .idProduct = USB_PID, ///< Product ID (assigned by the manufacturer).
    .bcdDevice = 0x1111,  ///< Device release number in binary-coded decimal.
    .iManufacturer = 0, ///< Index of string descriptor describing manufacturer.
    .iProduct = 0,      ///< Index of string descriptor describing product.
    .iSerialNumber = 0, ///< Index of string descriptor describing the device's
                        ///< serial number.

    .bNumConfigurations = 1 ///< Number of possible configurations.
};

int main() {
  tusb_rhport_init_t dev_init = {.role = TUSB_ROLE_DEVICE,
                                 .speed = TUSB_SPEED_AUTO};
  tusb_init(BOARD_TUD_RHPORT, &dev_init);
  stdio_init_all();

  while (true) {
    tud_task();
  }

  return 0;
}

//--------------------------------------------------------------------+
// Device descriptor callbacks
//--------------------------------------------------------------------+

uint8_t const *tud_descriptor_device_cb(void) { return (uint8_t const *)&dev; }

enum { ITF_NUM_MSC, ITF_NUM_TOTAL };

#define EPNUM_MSC_OUT 0x01
#define EPNUM_MSC_IN 0x81

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_MSC_DESC_LEN)

uint8_t const desc_configuration[] = {
    // Config number, interface count, string index, total length, attribute,
    // power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),

    // Interface number, string index, EP Out & EP In address, EP size
    TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 0, EPNUM_MSC_OUT, EPNUM_MSC_IN, 64),
};

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
  (void)index;
  return desc_configuration;
}

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  return nullptr;
}

//--------------------------------------------------------------------+
// MSC callbacks
//--------------------------------------------------------------------+

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

constexpr uint32_t DISK_BLOCK_SIZE = 512;
constexpr uint32_t DISK_BLOCK_NUM =
    (FLASH_SECTOR_SIZE * SECTORS) / DISK_BLOCK_SIZE;

uint8_t msc_disk[DISK_BLOCK_NUM][DISK_BLOCK_SIZE] = {0};

void tud_msc_capacity_cb(uint8_t lun, uint32_t *block_count,
                         uint16_t *block_size) {
  *block_count = DISK_BLOCK_NUM;
  *block_size = DISK_BLOCK_SIZE;
}

struct Params {
  uint32_t lba;
  uint32_t offset;
  void *buffer;
  uint32_t bufsize;
};
/* - FLASH_PAGE_SIZE = 1u << 8 = 256 bytes (smallest unit you can program)
- FLASH_SECTOR_SIZE = 1u << 12 = 4096 bytes (smallest unit you can erase — this
is the 4KB you've been using)
- FLASH_BLOCK_SIZE = 1u << 16 = 65536 bytes (64KB, used internally for the block
erase command)
*/

void read(void *data) {
  auto params = static_cast<Params *>(data);
  uint8_t *flash_start = (uint8_t *)(XIP_BASE + RESERVED_FLASH_OFFSET);

  uint8_t const *addr =
      flash_start + params->lba * DISK_BLOCK_SIZE + params->offset;
  auto *dst = static_cast<uint8_t *>(params->buffer);
  std::copy(addr, addr + params->bufsize, dst);
}

int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset,
                          void *buffer, uint32_t bufsize) {
  (void)lun;
  if (lba >= DISK_BLOCK_NUM) {
    return -1;
  }

  Params params = {
      .lba = lba, .offset = offset, .buffer = buffer, .bufsize = bufsize};

  auto result = flash_safe_execute(&read, static_cast<void *>(&params), 1000);
  if (result == PICO_OK) {
    return static_cast<uint32_t>(bufsize);
  }

  return -1;
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

  uint8_t *addr =
      buf + (params->lba * DISK_BLOCK_SIZE + params->offset) % FLASH_SECTOR_SIZE;
  std::copy_n((uint8_t *)params->buffer, params->bufsize, addr);
  // program sectors
  flash_range_program(RESERVED_FLASH_OFFSET + (startSector * FLASH_SECTOR_SIZE),
                      buf, FLASH_SECTOR_SIZE * (1 + endSector - startSector));
}

int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset,
                           uint8_t *buffer, uint32_t bufsize) {
  (void)lun;
  if (lba >= DISK_BLOCK_NUM) {
    return -1;
  }

  Params params = {
      .lba = lba, .offset = offset, .buffer = buffer, .bufsize = bufsize};

  auto result = flash_safe_execute(&write, static_cast<void *>(&params), 1000);
  if (result == PICO_OK) {
    return static_cast<uint32_t>(bufsize);
  }

  return -1;
}
