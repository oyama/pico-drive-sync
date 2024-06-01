/* On-board flash USB MSC exploler
 *
 * Copyright 2024, Hiroyuki OYAMA. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <errno.h>
#include <string.h>
#include <bsp/board.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <tusb.h>
#include <dirent.h>
#include "filesystem/vfs.h"

#define PREFIX   "/flash"

static void file_copy(const char *dist, const char *src_dir, const char *file) {
    char src_filename[PATH_MAX + 2] = {0};
    char dist_filename[PATH_MAX + 3] = {0};

    snprintf(src_filename, sizeof(src_filename) - 1, "%s/%s", src_dir, file);
    if (strcmp(src_dir, PREFIX) == 0) {
        snprintf(dist_filename, sizeof(dist_filename) - 1,
                 "%s/%s", dist, file);
    } else {
        snprintf(dist_filename, sizeof(dist_filename) - 1,
                 "%s/%s/%s", dist, (src_dir + strlen(PREFIX)), file);
    }
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

static void file_sync(const char *src, const char *dist) {
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
        } else if (ent->d_type == DT_DIR) {
            ; // mkdir();
            ; // file_sync()
        } else if (ent->d_type == DT_REG) {
            file_copy(dist, src, ent->d_name);
        }
    }
    int err = closedir(dir);
    if (err == -1) {
        fprintf(stderr, "closedir: %s", strerror(errno));
    }
}

int main(void) {
    board_init();
    tud_init(BOARD_TUD_RHPORT);
    stdio_init_all();
    if (!fs_init()) {
        fprintf(stderr, "File system initialize failure\n");
        return -1;
    }

    file_sync("/flash", "/ram");
    printf("USB MSC start\n");
    while (1) {
         tud_task();
    }
}
