/* Seamlessly share littlefs from Raspberry Pi Pico's onboard flash via USB
 *
 * Copyright 2024, Hiroyuki OYAMA. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <dirent.h>
#include <errno.h>
#include <hardware/structs/timer.h>
#include <stdio.h>
#include <string.h>
#include <pico/stdlib.h>
#include <tusb.h>
#include "filesystem/vfs.h"
#include "ssi_enable.h"

#define SRC_PREFIX          "/flash"
#define DIST_PREFIX         "/ram"
#define WINDOWS_HIDDEN_DIR  "System Volume Information"
#define USB_HOST_RECOGNISE_TIME   (250) // Time required for the USB host to recognise the change. Approx. 250 ms min

static uint8_t copy_buffer[512] = {0};  // Buffer used for file copying. This location because we want to reduce memory
extern bool is_usb_write_access(void);  // from usb_msc.c
extern bool remount_ram_disk(void);     // from fs_init.c


static void create_directory(const char *path) {
    printf("mkdir %s  # ", path);
    int err = mkdir(path, 0777);
    if (err == -1 && errno != EEXIST) {
        fprintf(stderr, "%s", strerror(errno));
        return;
    }
    printf("ok\n");
}

static void file_copy(const char *dist, const char *src) {
    printf("cp %s %s  # ", src, dist);

    FILE *in = fopen(src, "rb");
    if (in == NULL) {
        printf("fopen: %s", strerror(errno));
        return;
    }
    FILE *out = fopen(dist, "wb");
    if (out == NULL) {
        printf("fopen: %s", strerror(errno));
        fclose(in);
        return;
    }

    while (1) {
        size_t read_size = fread(copy_buffer, 1, sizeof(copy_buffer), in);
        if (read_size == 0) {
            if (feof(in))
                break;
            fprintf(stderr, "fread: %s", strerror(errno));
            break;
        }
        size_t write_size = fwrite(copy_buffer, 1, read_size, out);
        if (write_size != read_size) {
            fprintf(stderr, "fwrite: %s", strerror(errno));
            break;
        }
    }
    fclose(out);
    fclose(in);

    printf("ok\n");
}

static void directory_file_copy(const char *src, const char *dist) {
    DIR *dir = opendir(src);
    if (dir == NULL) {
        fprintf(stderr, "opendir %s: %s", src, strerror(errno));
        return;
    }
    char src_path[PATH_MAX + 2] = {0};
    char dist_path[PATH_MAX + 2] = {0};

    struct dirent *ent = NULL;
    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_type == DT_DIR && (strcmp(ent->d_name, ".") == 0 ||
                                      strcmp(ent->d_name, "..") == 0)) {
            continue;
        } else if (ent->d_type == DT_DIR && ent->d_name[0] == '.') {
            continue;
        } else if (ent->d_type == DT_DIR && strcmp(ent->d_name, WINDOWS_HIDDEN_DIR) == 0) {
            continue;
        } else if (ent->d_type == DT_DIR || ent->d_type == DT_REG) {
            snprintf(src_path, sizeof(src_path) - 1, "%s/%s", src, ent->d_name);
            snprintf(dist_path, sizeof(dist_path) - 1, "%s/%s", dist, ent->d_name);
            if (ent->d_type == DT_DIR) {
                create_directory(dist_path);
                directory_file_copy(src_path, dist_path);
            } else {
                file_copy(dist_path, src_path);
            }
        }
    }
    int err = closedir(dir);
    if (err == -1) {
        fprintf(stderr, "closedir: %s", strerror(errno));
    }
}

static void unlink_if_needed(const char *src, const char *dist) {
    struct stat finfo;
    int err = stat(dist, &finfo);
    if (err == -1) {
        printf("unlink %s  # ", src);
        err = unlink(src);
        if (err == -1) {
            fprintf(stderr, "%s", strerror(errno));
            return;
        }
        printf("ok\n");
    }
}

static void directory_file_delete(const char *src, const char *dist) {
    DIR *dir = opendir(src);
    if (dir == NULL) {
        fprintf(stderr, "opendir %s: %s", src, strerror(errno));
        return;
    }

    struct dirent *ent = NULL;
    char src_path[PATH_MAX + 3] = {0};
    char dist_path[PATH_MAX + 3] = {0};
    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_type == DT_DIR && (strcmp(ent->d_name, ".") == 0 ||
                                      strcmp(ent->d_name, "..") == 0)) {
            continue;
        } else if (ent->d_type == DT_DIR && ent->d_name[0] == '.') {
            continue;
        } else if (ent->d_type == DT_DIR || ent->d_type == DT_REG) {
            snprintf(src_path, sizeof(src_path) - 1, "%s/%s", src, ent->d_name);
            snprintf(dist_path, sizeof(dist_path) - 1, "%s/%s", dist, ent->d_name);
            if (ent->d_type == DT_DIR)
                directory_file_delete(src_path, dist_path);
            unlink_if_needed(src_path, dist_path);
        }
    }
    int err = closedir(dir);
    if (err == -1) {
        fprintf(stderr, "closedir: %s", strerror(errno));
    }
}

static bool is_end_of_usb_msc_write(void) {
    static bool last_access = false;
    bool usb_write = is_usb_write_access();
    bool result = false;
    if (last_access != usb_write && !usb_write) {
        result = true;
    }
    last_access = usb_write;
    return result;
}

static void reconnect_usb_for_host(void) {
    timer_hw->dbgpause = 0;  // NOTE: https://github.com/raspberrypi/pico-sdk/issues/1152

    tud_disconnect();
    sleep_ms(USB_HOST_RECOGNISE_TIME);
    tud_connect();
    sleep_ms(USB_HOST_RECOGNISE_TIME);
}

int main(void) {
    tud_init(BOARD_TUD_RHPORT);
    stdio_init_all();
    reconnect_usb_for_host();

    ssi_enable();
    if (!fs_init()) {
        fprintf(stderr, "File system initialize failure\n");
        return -1;
    }

    directory_file_copy("/flash", "/ram");
    printf("USB MSC start\n");
    while (1) {
         if (is_end_of_usb_msc_write()) {
             if (!remount_ram_disk())  // Reflect updates from the host
                 continue;
             directory_file_copy("/ram", "/flash");
             directory_file_delete("/flash", "/ram");
         }
         tud_task();
    }
}
