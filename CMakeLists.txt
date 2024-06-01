cmake_minimum_required(VERSION 3.13...3.27)

if(NOT FLASH_SIZE)
  set(FLASH_SIZE 1441792)
endif()
message("Set explore storage size to ${FLASH_SIZE} bytes")


include(vendor/pico_sdk_import.cmake)
add_subdirectory(vendor/pico-vfs)

set(FAMILY rp2040)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
project(explorer C CXX ASM)
pico_sdk_init()

add_executable(explorer
  src/main.c
  src/usb_descriptors.c
  src/usb_msc.c
)
target_compile_options(explorer PRIVATE -Os -DPICO_VFS_NO_RTC=1  -Werror -Wall -Wextra -Wnull-dereference)
target_include_directories(explorer PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(explorer PRIVATE
  pico_stdlib
  blockdevice_flash
  blockdevice_heap
  filesystem_littlefs
  filesystem_fat
  filesystem_vfs
  tinyusb_board
  tinyusb_device
)
pico_enable_stdio_usb(explorer 1)
#pico_add_extra_outputs(explorer)
pico_enable_filesystem(explorer SIZE ${FLASH_SIZE} FS_INIT src/fs_init.c)

target_link_options(explorer PRIVATE -Wl,--print-memory-usage)
pico_set_binary_type(explorer no_flash)


find_program(OPENOCD openocd)
if(OPENOCD)
  add_custom_target(run
    COMMAND ${OPENOCD} -f interface/cmsis-dap.cfg -f target/rp2040.cfg -c "adapter speed 5000" -c "init" -c "reset halt" -c "load_image explorer.elf" -c "resume 0x20000000" -c "exit"
    DEPENDS explorer
  )
endif()
