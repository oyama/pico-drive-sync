#include <stdio.h>
#include <string.h>
#include <hardware/clocks.h>
#include <hardware/flash.h>
#include "blockdevice/heap.h"
#include "blockdevice/flash.h"
#include "filesystem/fat.h"
#include "filesystem/littlefs.h"
#include "filesystem/vfs.h"


blockdevice_t *blockdevice_heap;  // Share with USB MSC


bool fs_init(void) {
    blockdevice_heap = blockdevice_heap_create(64*1024);
    blockdevice_t *flash = blockdevice_flash_create(PICO_FLASH_SIZE_BYTES - PICO_FS_DEFAULT_SIZE, 0);

    filesystem_t *lfs = filesystem_littlefs_create(500, 16);
    filesystem_t *fat = filesystem_fat_create();

    printf("Format and mount /ram FAT RAM disk ... ");
    int err = fs_format(fat, blockdevice_heap);
    if (err == -1) {
        printf("FAT format error: %s\n", strerror(errno));
        return false;
    }
    err = fs_mount("/ram", fat, blockdevice_heap);
    if (err == -1) {
        printf("fs_mount /ram error: %s\n", strerror(errno));
        return false;
    }
    printf("ok\n");

    printf("Mount /flash littlefs on-board flash ... ");
    err = fs_mount("/flash", lfs, flash);
    if (err == -1) {
        printf("fs_mount /flash error: %s\n", strerror(errno));
        return false;
    }
    printf("ok\n");

    return err == 0;
}
