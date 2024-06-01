#include <string.h>
#include <ctype.h>
#include <bsp/board.h>
#include <tusb.h>
#include "blockdevice/heap.h"

static bool ejected = false;
static bool usb_connected = false;

extern blockdevice_t *blockdevice_heap;


bool usb_connection_status(void) {
    return usb_connected;
}

void tud_mount_cb(void) {
    usb_connected = true;
}

void tud_suspend_cb(bool remote_wakeup_en) {
    (void)remote_wakeup_en;
    usb_connected = false;
}

void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4]) {
    (void) lun;

    const char vid[] = "TinyUSB";
    const char pid[] = "Mass Storage";
    const char rev[] = "1.0";

    memcpy(vendor_id  , vid, strlen(vid));
    memcpy(product_id , pid, strlen(pid));
    memcpy(product_rev, rev, strlen(rev));
}

bool tud_msc_test_unit_ready_cb(uint8_t lun) {
    (void) lun;

    if (ejected) {
        tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x3a, 0x00);
        return false;
    }

    return true;
}

void tud_msc_capacity_cb(uint8_t lun, uint32_t *block_count, uint16_t *block_size) {
    (void) lun;
    *block_count = blockdevice_heap->size(blockdevice_heap) /  blockdevice_heap->erase_size;
    *block_size  = blockdevice_heap->erase_size;
}

bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject) {
    (void) lun;
    (void) power_condition;
    if ( load_eject ) {
        if (start) {
            // load disk storage
        } else {
            // unload disk storage
            ejected = true;
        }
    }
    return true;
}

int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize) {
    (void)lun;
    (void)offset;

    int err = blockdevice_heap->read(blockdevice_heap, buffer, lba * blockdevice_heap->erase_size, bufsize);
    if (err != 0) {
        printf("read error=%d\n", err);
    }
    return (int32_t) bufsize;
}

bool tud_msc_is_writable_cb (uint8_t lun) {
    (void) lun;
    return true;
}

int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize) {
    (void)lun;
    (void)offset;

    uint32_t block_size = blockdevice_heap->erase_size;
    int err = blockdevice_heap->erase(blockdevice_heap, lba * block_size, bufsize);
    if (err != BD_ERROR_OK) {
        printf("erase error=%d\n", err);
    }
    err = blockdevice_heap->program(blockdevice_heap, buffer, lba * block_size, bufsize);
    if (err != BD_ERROR_OK) {
        printf("program error=%d\n", err);
    }

    return (int32_t)bufsize;
}

int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void *buffer, uint16_t bufsize) {
    (void)lun;

    void const* response = NULL;
    int32_t resplen = 0;
    bool in_xfer = true;

    switch (scsi_cmd[0]) {
    default:
        tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);
        resplen = -1;
        break;
    }

    if (resplen > bufsize)
        resplen = bufsize;

    if (response && (resplen > 0)) {
        if(in_xfer) {
            memcpy(buffer, response, (size_t)resplen);
        } else {
            ; // SCSI output
        }
    }

    return (int32_t)resplen;
}
