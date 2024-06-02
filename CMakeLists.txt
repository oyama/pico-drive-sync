cmake_minimum_required(VERSION 3.13...3.27)

if(NOT FLASH_SIZE)
  set(FLASH_SIZE 1441792)
endif()
message("Set the littlefs storage size to be sync to ${FLASH_SIZE} bytes")

include(vendor/pico_sdk_import.cmake)
add_subdirectory(vendor/pico-vfs)

set(FAMILY rp2040)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
project(sync C CXX ASM)
pico_sdk_init()

add_executable(sync
  src/main.c
  src/usb_descriptors.c
  src/usb_msc.c
)
target_compile_options(sync PRIVATE -Os -DPICO_VFS_NO_RTC=1  -Werror -Wall -Wextra -Wnull-dereference)
target_include_directories(sync PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(sync PRIVATE
  pico_stdlib
  blockdevice_flash
  blockdevice_heap
  filesystem_littlefs
  filesystem_fat
  filesystem_vfs
  tinyusb_device
)
pico_enable_stdio_usb(sync 1)
pico_enable_filesystem(sync SIZE ${FLASH_SIZE} FS_INIT src/fs_init.c)
pico_add_extra_outputs(sync)
target_link_options(sync PRIVATE -Wl,--print-memory-usage)
pico_set_binary_type(sync no_flash)

#
# USB disconnection firmware
#
add_executable(deinit
  src/deinit.c
  src/usb_descriptors.c
  src/usb_msc.c
)
target_compile_options(deinit PRIVATE -Os -DPICO_VFS_NO_RTC=1  -Werror -Wall -Wextra -Wnull-dereference)
target_include_directories(deinit PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(deinit PRIVATE
  pico_stdlib
  pico_stdlib
  blockdevice_flash
  blockdevice_heap
  filesystem_littlefs
  filesystem_fat
  filesystem_vfs
  tinyusb_device
)
pico_enable_stdio_usb(deinit 1)
pico_set_binary_type(deinit no_flash)
pico_enable_filesystem(deinit SIZE ${FLASH_SIZE} FS_INIT src/fs_init.c)


find_program(OPENOCD openocd)
if(OPENOCD)
  # deinit and sync
  add_custom_target(run COMMAND
    ${OPENOCD} -f interface/cmsis-dap.cfg -f target/rp2040.cfg
               -c "adapter speed 5000" -c "init" -c "reset halt" -c "load_image deinit.elf" -c "resume 0x20000000"
               -c "reset halt" -c "load_image sync.elf" -c "resume 0x20000000" -c "exit"
    DEPENDS deinit sync
  )
endif()
