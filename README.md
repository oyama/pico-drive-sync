# Pico Drive Sync - Seamlessly share littlefs from Raspberry Pi Pico's onboard flash via USB

This project is a tool designed to share data stored on the Raspberry Pi Pico's flash memory as a USB storage device that can be accessed directly from a PC. It allows for easy backup and inspection of files on embedded devices' filesystems without the need for specialized tools typically required for such operations. By mounting it as a USB drive on a PC, file manipulation becomes intuitive, significantly enhancing development efficiency.

## How the programme works

1. Create a RAM disk with FAT and mount it to `/ram`
2. Mount the onboard flash memory littlefs to `/flash`
3. Copy the contents of `/flash` to `/ram`
4. Start USB MSC

The firmware is designed to be deployed in RAM, not flash. The total file size that can be shared via USB MSC is limited to the RAM disk size of 64 KB.

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
If the compilation is successful, the programme is loaded into Pico's RAM using `openocd` and executed.

## Configuration

Specify the block device size of the littlefs on the flash memory with the option `-DFLASH_SIZE` in CMake. If not specified, `1441792` bytes are set.

```bash
PICO_SDK_PATH=/path/to/pico-sdk cmake .. -DFLASH_SIZE=1441792
```
