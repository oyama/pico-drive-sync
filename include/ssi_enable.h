#pragma once

/* Enable Synchronous Serial Interface (SSI) controller
 * XIP is disabled when running RAM with no_flash, so SSI must be enabled to read on-board flash memory.
 *
 * https://forums.raspberrypi.com/viewtopic.php?p=2090058#p2090058
 */
void ssi_enable(void);
