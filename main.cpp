#include "common.h"
#include "flash.h"
#include "tusb_config.h"
#include <algorithm>
#include <common/tusb_types.h>
#include <cstdint>
#include <hardware/regs/addressmap.h>
#include <pico/flash.h>
#include <pico/stdlib.h>
#include <pico/time.h>
#include <string>
#include <tusb.h>

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
