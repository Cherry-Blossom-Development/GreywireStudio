#include "pedal_flash.h"
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"

extern char __flash_binary_end;

void pedal_flash_check(void)
{
    if ((uintptr_t)&__flash_binary_end - XIP_BASE > PEDAL_FLASH_BASE)
        panic("firmware overlaps the pedal's settings in flash");
}

static void rd(void *ctx, uint32_t off, void *buf, uint32_t n)
{
    (void)ctx;
    memcpy(buf, (const void *)(XIP_BASE + PEDAL_FLASH_BASE + off), n);
}

/* Programs page by page. The rest of each page is sent as FF, which leaves the flash there
 * unchanged, so several records can share a page. Each page takes about 0.5 ms with
 * interrupts off; the UART's 32-byte buffer holds about 10 ms of MIDI. */
static void wr(void *ctx, uint32_t off, const void *buf, uint32_t n)
{
    (void)ctx;
    static uint8_t page[FLASH_PAGE_SIZE];
    const uint8_t *src = buf;
    while (n) {
        uint32_t abs = PEDAL_FLASH_BASE + off;
        uint32_t start = abs & ~(FLASH_PAGE_SIZE - 1u);
        uint32_t in = abs - start;
        uint32_t k = FLASH_PAGE_SIZE - in;
        if (k > n)
            k = n;
        memset(page, 0xFF, sizeof page);
        memcpy(&page[in], src, k);
        uint32_t irq = save_and_disable_interrupts();
        flash_range_program(start, page, FLASH_PAGE_SIZE);
        restore_interrupts(irq);
        off += k;
        src += k;
        n -= k;
    }
}

/* About 45 ms with interrupts off: pedal.c only asks for it in a gap on MIDI in */
static void er(void *ctx, uint32_t off, uint32_t n)
{
    (void)ctx;
    uint32_t irq = save_and_disable_interrupts();
    flash_range_erase(PEDAL_FLASH_BASE + off, n);
    restore_interrupts(irq);
}

const lw_flash pedal_flash = {NULL, rd, wr, er};
