/* Seamlessly share littlefs from Raspberry Pi Pico's onboard flash via USB
 *
 * Copyright 2024, Hiroyuki OYAMA. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <pico/stdlib.h>
#include <tusb.h>
#include "filesystem/vfs.h"


int main(void) {
    tud_init(BOARD_TUD_RHPORT);
    stdio_init_all();
    fs_init();

    tud_disconnect();
    sleep_ms(250);
}
