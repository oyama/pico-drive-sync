# Raspberry Pi on-board flash memory exporter

This is a sample project that reads the contents of the littlefs file system extracted to the Raspberry Pi Pico's onboard flash memory to the host PC via USB Mass Storage Class. The firmware for this project is designed to be deployed in RAM, not flash.

## How the programme works

1. Create a RAM disk with FAT and mount it to `/ram`
2. Mount the onboard flash memory littlefs to `/flash`
3. Copy the contents of `/flash` to `/ram`
4. Start USB MSC

The total file size that can be shared via USB MSC is limited to the RAM disk size of 64 KB.

## Build and Run

To compile and install this project, a setup with the [pico-sdk](https://github.com/raspberrypi/pico-sdk) is necessary. Please refer to [Getting Started with Raspberry Pi Pico](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf) to prepare your toolchain. This project incorporates [pico-vfs](https://github.com/oyama/pico-vfs) and [littlefs](https://github.com/littlefs-project/littlefs) as _git submodule_, which requires an update post-checkout.

```bash
git clone https://github.com/oyama/pico-flash-explorer.git
cd pico-flash-explorer
git submodule update --init --recursive

mkdir build; cd build;
PICO_SDK_PATH=/path/to/pico-sdk cmake ..
make run
```
If the compilation is successful, the programme is loaded into Pico's RAM using openocd and executed.

## Configuration

Specify the block device size of the littlefs on the flash memory with the option `-DFLASH_SIZE` in CMake. If not specified, `1441792` bytes are set.

```bash
PICO_SDK_PATH=/path/to/pico-sdk cmake .. -DFLASH_SIZE=1441792
```
