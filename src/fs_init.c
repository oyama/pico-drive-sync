/*
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <string.h>
#include <hardware/clocks.h>
#include <hardware/flash.h>
#include "blockdevice/heap.h"
#include "blockdevice/flash.h"
#include "filesystem/fat.h"
#include "filesystem/littlefs.h"
#include "filesystem/vfs.h"

#define RAM_DISK_SIZE    (64 * 1024)

blockdevice_t *blockdevice_heap;  // Share to device access in usb_msc.c
static filesystem_t *fat;

//  USB devices require remounting to incorporate USB host updates
bool remount_ram_disk(void) {
    int err = fs_unmount("/ram");
    if (err == -1) {
        printf("fs_mount /ram error: %s\n", strerror(errno));
        return false;
    }
    err = fs_mount("/ram", fat, blockdevice_heap);
    if (err == -1) {
        printf("fs_mount /ram error: %s\n", strerror(errno));
        return false;
    }
    return true;
}

bool fs_init(void) {
    blockdevice_heap = blockdevice_heap_create(RAM_DISK_SIZE);
    blockdevice_t *flash = blockdevice_flash_create(PICO_FLASH_SIZE_BYTES - PICO_FS_DEFAULT_SIZE, 0);

    fat = filesystem_fat_create();
    filesystem_t *lfs = filesystem_littlefs_create(500, 16);

    printf("/ram format FAT ... ");
    int err = fs_format(fat, blockdevice_heap);
    if (err == -1) {
        fprintf(stderr, "%s", strerror(errno));
        return false;
    }
    printf("ok\n");

    printf("/ram mount FAT ... ");
    err = fs_mount("/ram", fat, blockdevice_heap);
    if (err == -1) {
        fprintf(stderr, "%s", strerror(errno));
        return false;
    }
    printf("ok\n");

    printf("/flash mount ... ");
    err = fs_mount("/flash", lfs, flash);
    if (err == -1) {
        fprintf(stderr, "%s", strerror(errno));
        return false;
    }
    printf("ok\n");

    return true;
}
