#pragma once
#include "common/tusb_types.h"
#include "tusb_config.h"

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
