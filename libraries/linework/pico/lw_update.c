#include "lw_update.h"
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "hardware/watchdog.h"
#include "hardware/structs/scb.h"
#include "hardware/structs/nvic.h"
#include "hardware/structs/systick.h"
#include "hardware/structs/timer.h"

static void rd(void *ctx, uint32_t off, void *buf, uint32_t n)
{
    (void)ctx;
    memcpy(buf, (const void *)(XIP_BASE + off), n);
}

/* page by page, FF-padded, so writes needn't be page-aligned (flash only clears bits) */
static void wr(void *ctx, uint32_t off, const void *buf, uint32_t n)
{
    (void)ctx;
    static uint8_t page[FLASH_PAGE_SIZE];
    const uint8_t *src = buf;
    while (n) {
        uint32_t start = off & ~(FLASH_PAGE_SIZE - 1u), in = off - start, k = FLASH_PAGE_SIZE - in;
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

static void er(void *ctx, uint32_t off, uint32_t n)
{
    (void)ctx;
    if (off < FW_LOADER_SIZE)
        return;                                 /* never the loader */
    uint32_t irq = save_and_disable_interrupts();
    flash_range_erase(off, n);
    restore_interrupts(irq);
}

const lw_flash lw_flash_whole = {NULL, rd, wr, er};

static lw_fw_rx rx;
static int u_begin(void *c, uint32_t size, uint32_t crc) { (void)c; return lw_fw_begin(&rx, &lw_flash_whole, size, crc); }
static bool u_erase(void *c) { (void)c; return lw_fw_erase_step(&rx); }
static int u_block(void *c, uint32_t n, const uint8_t *d) { (void)c; return lw_fw_block(&rx, n, d); }
static int u_finish(void *c) { (void)c; return lw_fw_finish(&rx); }
static void u_restart(void *c)
{
    (void)c;
    watchdog_reboot(0, 0, 10);                  /* a normal start: the loader runs first */
    while (true)
        tight_loop_contents();
}
const pd_fw_hal lw_update_hal = {NULL, u_begin, u_erase, u_block, u_finish, u_restart};

void lw_start_app(void)
{
    const uint32_t *vt = (const uint32_t *)(XIP_BASE + FW_APP_OFF + FW_VECTORS);
    /* leave nothing running that could interrupt the firmware before it sets itself up */
    systick_hw->csr = 0;
    timer_hw->armed = 0xF;
    timer_hw->inte = 0;
    timer_hw->intr = 0xF;
    nvic_hw->icer = 0xFFFFFFFFu;
    nvic_hw->icpr = 0xFFFFFFFFu;
    scb_hw->vtor = (uint32_t)vt;
    __asm volatile("msr msp, %0\n"
                   "bx %1\n" ::"r"(vt[0]), "r"(vt[1]));
    __builtin_unreachable();
}
