#include "flash_store.h"
#include "board.h"
#include "switchyard.h"
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"

extern char __flash_binary_end;

void store_check_fits(void)
{
    if ((uintptr_t)&__flash_binary_end - XIP_BASE > STORE_BASE)
        panic("firmware is larger than 1 MB and overlaps the setlist store");
    if (STORE_BASE + SY_STORE_SIZE > PICO_FLASH_SIZE_BYTES)
        panic("setlist store doesn't fit in flash");
}

void store_read(void *ctx, uint32_t off, void *buf, uint32_t n)
{
    (void)ctx;
    memcpy(buf, (const void *)(XIP_BASE + STORE_BASE + off), n);
}

/* Programs the bytes page by page. The rest of each page is sent as FF, which leaves the
 * flash there unchanged, so several writes can share one page. */
void store_write(void *ctx, uint32_t off, const void *buf, uint32_t n)
{
    (void)ctx;
    static uint8_t page[FLASH_PAGE_SIZE];
    const uint8_t *src = buf;
    while (n) {
        uint32_t abs = STORE_BASE + off;
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

void store_erase(void *ctx, uint32_t off, uint32_t n)
{
    (void)ctx;
    uint32_t irq = save_and_disable_interrupts();
    flash_range_erase(STORE_BASE + off, n);   /* n is a multiple of 4 KB */
    restore_interrupts(irq);
}
