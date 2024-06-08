# Pico Drive Sync - Seamlessly share littlefs from Raspberry Pi Pico's onboard flash via USB

This project enables the sharing of data stored on the Raspberry Pi Pico's flash memory, formatted with the littlefs file system, as a USB storage device that can be accessed from a PC. It supports both read and write operations, allowing for easy backup and inspection directly from the PC without the need for specialized tools normally required for such tasks.

The firmware is designed to be deployed in RAM. The total file size that can be shared via USB MSC is limited to the RAM disk size of 64 KB.

## How it works

Extract the firmware to the Pico's RAM and run it

1. Create a RAM disk with FAT and mount it to `/ram`
2. Mount the onboard flash memory littlefs to `/flash`
3. Copy the contents of `/flash` to `/ram`
4. Start USB MSC
5. Wait for host PC to write.
6. When the writing is finished, copy the content from `/ram` to `/flash`
7. Repeat from step 5

When reset, the Pico operates with the original firmware

## Using Pre-built Firmware

For those who prefer not to build the firmware from source, pre-built binaries are available under the [Releases](https://github.com/oyama/pico-drive-sync/releases) section of this repository. To use the pre-built firmware, follow these steps:

1. Download the latest `sync.uf2` file from the Releases page.
2. Connect your Raspberry Pi Pico to your PC in BOOTCEL mode (hold the BOOTSEL button while connecting via USB).
3. Drag and drop the downloaded `sync.uf2` file onto the RPI-RP2 drive that appears on your computer.
4. The Pico will automatically reboot and start running the new firmware after the file transfer is complete.

This allows you to quickly test and use the application without needing to set up a development environment.

## Build and Run

To compile and install this project, a setup with the [pico-sdk](https://github.com/raspberrypi/pico-sdk) is necessary. Please refer to [Getting Started with Raspberry Pi Pico](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf) to prepare your toolchain. This project incorporates [pico-vfs](https://github.com/oyama/pico-vfs) and [littlefs](https://github.com/littlefs-project/littlefs) as _git submodule_, which requires an update post-checkout.

```bash
git clone https://github.com/oyama/pico-drive-sync.git
cd pico-drive-sync
git submodule update --init --recursive

mkdir build; cd build;
PICO_SDK_PATH=/path/to/pico-sdk cmake ..
make
```
Successful compilation will generate `sync.uf2`. Simply drag and drop this onto the Raspberry Pi Pico and the firmware will be executate from RAM.
Alternatively, it can be performed using [picotool](https://github.com/raspberrypi/picotool):

```bash
picotool load --execute sync.uf2 --force
```

When the file operation on the host PC is finished, restart Pico and it will run with the original firmware from the flash.

## Configuration

Specify the block device size of the littlefs on the flash memory with the option `-DFLASH_SIZE` in CMake. If not specified, `1441792` bytes are set. This is a setting consistent with the file system of the MicroPython environment.

```bash
PICO_SDK_PATH=/path/to/pico-sdk cmake .. -DFLASH_SIZE=1441792
```
