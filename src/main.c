/* Seamlessly share littlefs from Raspberry Pi Pico's onboard flash via USB
 *
 * Copyright 2024, Hiroyuki OYAMA. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <bsp/board.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <pico/stdlib.h>
#include <pico/time.h>
#include <tusb.h>
#include "filesystem/vfs.h"

#define SRC_PREFIX   "/flash"
#define DIST_PREFIX   "/ram"

bool is_usb_write_access(void);
extern bool remount_ram_disk(void);

static void create_directory(const char *prefix, const char *dist, const char *src_dir, const char *dir) {
    char src_filename[PATH_MAX + 2] = {0};
    char dist_filename[PATH_MAX + 3] = {0};

    snprintf(src_filename, sizeof(src_filename) - 1, "%s/%s", src_dir, dir);
    if (strcmp(src_dir, prefix) == 0) {
        snprintf(dist_filename, sizeof(dist_filename) - 1,
                 "%s/%s", dist, dir);
    } else {
        snprintf(dist_filename, sizeof(dist_filename) - 1,
                 "%s/%s/%s", dist, (src_dir + strlen(prefix)), dir);
    }
    int err = mkdir(dist_filename, 0777);
    if (err == -1 && errno != EEXIST) {
        fprintf(stderr, "mkdir %s:%s", dist_filename, strerror(errno));
        return;
    }
    printf("ok\n");
}

static void file_copy(const char *prefix, const char *dist, const char *src_dir, const char *file) {
    char src_filename[PATH_MAX + 2] = {0};
    char dist_filename[PATH_MAX + 3] = {0};
    (void)prefix;
    snprintf(src_filename, sizeof(src_filename) - 1, "%s/%s", src_dir, file);
    snprintf(dist_filename, sizeof(dist_filename) - 1, "%s/%s", dist, file);
    printf("copy %s to %s ... ", src_filename, dist_filename);

    FILE *in = fopen(src_filename, "rb");
    if (in == NULL) {
        printf("fopen %s: %s", src_filename, strerror(errno));
        return;
    }
    FILE *out = fopen(dist_filename, "wb");
    if (out == NULL) {
        printf("fopen %s: %s", dist_filename, strerror(errno));
        fclose(in);
        return;
    }

    uint8_t buffer[512] = {0};
    while (1) {
        size_t read_size = fread(buffer, 1, sizeof(buffer), in);
        if (read_size == 0) {
            if (feof(in))
                break;
            fprintf(stderr, "can't read %s: %s", src_filename, strerror(errno));
            break;
        }
        size_t write_size = fwrite(buffer, 1, read_size, out);
        if (write_size != read_size) {
            fprintf(stderr, "can't write %s: %s\n", dist_filename, strerror(errno));
            break;
        }
    }
    fclose(out);
    fclose(in);

    printf("ok\n");
}

static void file_sync(const char *origin_prefix, const char *src, const char *dist) {
    DIR *dir = opendir(src);
    if (dir == NULL) {
        fprintf(stderr, "opendir %s: %s", src, strerror(errno));
        return;
    }

    struct dirent *ent = NULL;
    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_type == DT_DIR && (strcmp(ent->d_name, ".") == 0 ||
                                      strcmp(ent->d_name, "..") == 0)) {
            continue;
        } else if (ent->d_type == DT_DIR && ent->d_name[0] == '.') {
            continue;
        } else if (ent->d_type == DT_DIR) {
            create_directory(origin_prefix, dist, src, ent->d_name);
            char src_path[PATH_MAX + 2] = {0};
            char dist_path[PATH_MAX + 2] = {0};
            snprintf(src_path, sizeof(src_path) - 1, "%s/%s", src, ent->d_name);
            snprintf(dist_path, sizeof(dist_path) - 1, "%s/%s", dist, ent->d_name);
            file_sync(origin_prefix, src_path, dist_path);
        } else if (ent->d_type == DT_REG) {
            file_copy(origin_prefix, dist, src, ent->d_name);
        }
    }
    int err = closedir(dir);
    if (err == -1) {
        fprintf(stderr, "closedir: %s", strerror(errno));
    }
}

static void file_delete_if_needed(const char *src, const char *dist, const char *file) {
    char src_filename[PATH_MAX + 3] = {0};
    char dist_filename[PATH_MAX + 3] = {0};
    snprintf(src_filename, sizeof(src_filename) - 1, "%s/%s", src, file);
    snprintf(dist_filename, sizeof(dist_filename) - 1, "%s/%s", dist, file);

    struct stat finfo;
    int err = stat(dist_filename, &finfo);
    if (err == -1) {
        printf("Delete %s ... ", src_filename);
        err = unlink(src_filename);
        if (err == -1) {
            fprintf(stderr, "unlink %s: %s", src_filename, strerror(errno));
            return;
        }
        printf("ok\n");
    }
}

static void file_delete(const char *origin_prefix, const char *src, const char *dist) {
    DIR *dir = opendir(src);
    if (dir == NULL) {
        fprintf(stderr, "opendir %s: %s", src, strerror(errno));
        return;
    }

    struct dirent *ent = NULL;
    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_type == DT_DIR && (strcmp(ent->d_name, ".") == 0 ||
                                      strcmp(ent->d_name, "..") == 0)) {
            continue;
        } else if (ent->d_type == DT_DIR && ent->d_name[0] == '.') {
            continue;
        } else if (ent->d_type == DT_DIR) {
            char src_path[PATH_MAX + 2] = {0};
            char dist_path[PATH_MAX + 2] = {0};
            snprintf(src_path, sizeof(src_path) - 1, "%s/%s", src, ent->d_name);
            snprintf(dist_path, sizeof(dist_path) - 1, "%s/%s", dist, ent->d_name);
            file_delete(origin_prefix, src_path, dist_path);
        } else if (ent->d_type == DT_REG) {
            file_delete_if_needed(src, dist, ent->d_name);
        }
    }
    int err = closedir(dir);
    if (err == -1) {
        fprintf(stderr, "closedir: %s", strerror(errno));
    }
}


static bool is_end_of_writing(void) {
    static bool last_access = false;
    bool usb_write = is_usb_write_access();
    bool result = false;
    if (last_access != usb_write && !usb_write) {
        result = true;
    }
    last_access = usb_write;
    return result;
}

int main(void) {
    board_init();
    tud_init(BOARD_TUD_RHPORT);
    stdio_init_all();
    if (!fs_init()) {
        fprintf(stderr, "File system initialize failure\n");
        return -1;
    }

    file_sync(SRC_PREFIX, "/flash", "/ram");

    printf("USB MSC start\n");
    while (1) {
         if (is_end_of_writing()) {
             if (!remount_ram_disk())
                 continue;
             file_sync(DIST_PREFIX, "/ram", "/flash");
             file_delete(SRC_PREFIX, "/flash", "/ram");
         }
         tud_task();
    }
}
