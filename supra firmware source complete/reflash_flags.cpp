#include "reflash_flags.h"

#include <cstring>

#include "hardware/flash.h"
#include "hardware/regs/addressmap.h"
#include "hardware/sync.h"

namespace {

constexpr uint32_t kMagicSUPR = 0x52505553u; // "SUPR" in little-endian

struct FlashFlagsPage {
    uint32_t magic;
    uint32_t date_set; // 0xFFFFFFFF = unset, 0x00000000 = set
    uint8_t reserved[FLASH_PAGE_SIZE - 8];
};

struct FlashFlagsSector {
    FlashFlagsPage page0;
    uint8_t reserved[FLASH_SECTOR_SIZE - FLASH_PAGE_SIZE];
};

__attribute__((section(".flashdata.supra_reflash_flags"), used, aligned(FLASH_SECTOR_SIZE)))
const volatile FlashFlagsSector g_default_flags = {
    {kMagicSUPR, 0xFFFFFFFFu, {0}},
    {0},
};

volatile const FlashFlagsPage* flags_ptr() {
    return &g_default_flags.page0;
}

uint32_t flash_offset_bytes() {
    const uintptr_t xip_addr = reinterpret_cast<uintptr_t>(&g_default_flags.page0);
    return static_cast<uint32_t>(xip_addr - XIP_BASE);
}

uint32_t load_u32(volatile const void* p) {
    uint32_t v;
    const volatile uint8_t* b = reinterpret_cast<const volatile uint8_t*>(p);
    v = (uint32_t)b[0] | ((uint32_t)b[1] << 8u) | ((uint32_t)b[2] << 16u) | ((uint32_t)b[3] << 24u);
    return v;
}

void read_bytes(void* out, volatile const void* in, size_t n) {
    uint8_t* dst = reinterpret_cast<uint8_t*>(out);
    const volatile uint8_t* src = reinterpret_cast<const volatile uint8_t*>(in);
    for (size_t i = 0; i < n; ++i) {
        dst[i] = src[i];
    }
}

} // namespace

bool supra_reflash_date_is_set() {
    volatile const FlashFlagsPage* page = flags_ptr();
    if (load_u32(&page->magic) != kMagicSUPR) {
        return false;
    }
    return load_u32(&page->date_set) == 0x00000000u;
}

void supra_reflash_date_mark_set() {
    volatile const FlashFlagsPage* current = flags_ptr();
    if (load_u32(&current->magic) == kMagicSUPR && load_u32(&current->date_set) == 0x00000000u) {
        return;
    }

    uint8_t page_buf[FLASH_PAGE_SIZE];
    read_bytes(page_buf, current, sizeof(page_buf));

    auto* new_page = reinterpret_cast<FlashFlagsPage*>(page_buf);
    new_page->magic = kMagicSUPR;
    new_page->date_set = 0x00000000u;

    const uint32_t offset = flash_offset_bytes();
    const uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(offset, FLASH_SECTOR_SIZE);
    flash_range_program(offset, page_buf, FLASH_PAGE_SIZE);
    restore_interrupts(ints);
}
