#include <pico/bootrom.h>
#include <hardware/resets.h>
#include <hardware/structs/ssi.h>

void ssi_enable(void) {
    if (!ssi_hw->ssienr) {
        uint32_t resets = 0 | RESETS_RESET_IO_QSPI_BITS | RESETS_RESET_PADS_QSPI_BITS;

        reset_block(resets);
        unreset_block_wait(resets);

        __compiler_memory_barrier();

        ((void(*)(void))rom_func_lookup_inline(ROM_FUNC_CONNECT_INTERNAL_FLASH))();
        ((void(*)(void))rom_func_lookup_inline(ROM_FUNC_FLASH_EXIT_XIP))();
        ((void(*)(void))rom_func_lookup_inline(ROM_FUNC_FLASH_ENTER_CMD_XIP))();

        ssi_hw->ssienr = 0; // SSI_SSIENR_SSI_EN_RESET ? SSI_SSIENR_RESET ?
        ssi_hw->baudr = 16;
        ssi_hw->ssienr = 1; // SSI_SSIENR_SSI_EN_BITS ?

        ((void(*)(void))rom_func_lookup_inline(ROM_FUNC_FLASH_FLUSH_CACHE))();

        __compiler_memory_barrier();
    }
}
