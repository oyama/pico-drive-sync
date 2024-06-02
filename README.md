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

## Limitations

- Windows 11 does not automatically start the USB MSC when the Pico is booted from RAM. This problem can be avoided by restarting Windows 11.
  
## Build and Run

To compile and install this project, a setup with the [pico-sdk](https://github.com/raspberrypi/pico-sdk) is necessary. Please refer to [Getting Started with Raspberry Pi Pico](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf) to prepare your toolchain. This project incorporates [pico-vfs](https://github.com/oyama/pico-vfs) and [littlefs](https://github.com/littlefs-project/littlefs) as _git submodule_, which requires an update post-checkout.

```bash
git clone https://github.com/oyama/pico-drive-sync.git
cd pico-drive-sync
git submodule update --init --recursive

mkdir build; cd build;
PICO_SDK_PATH=/path/to/pico-sdk cmake ..
make run
```
If the compilation is successful, the programme is loaded into Pico's RAM using `openocd` and executed. The `openocd` command is required to place the firmware in RAM and run it.

## Configuration

Specify the block device size of the littlefs on the flash memory with the option `-DFLASH_SIZE` in CMake. If not specified, `1441792` bytes are set.

```bash
PICO_SDK_PATH=/path/to/pico-sdk cmake .. -DFLASH_SIZE=1441792
```
