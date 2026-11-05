/* The stand-in pedal: a second Pico on the breadboard with its own MIDI in (H11L1 on GP1)
 * and MIDI out (GP0). It answers Switchyard like a Linework pedal (core/pedal.c), so the
 * whole loop can be tested before Flashover has its MIDI parts.
 *
 * It keeps its channel, presets and current values in flash (the last 64 KB), as real pedals do.
 *
 * Debug: open the Pico's USB serial port (115200, any terminal). It prints the position and
 * channel when they change. Typing 1-3 turns "knob" 1-3 up by 8 and 4 switches it on or off, to test hand-change reports. */
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/unique_id.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

#include "board.h"
#include "ring.h"
#include "pedal_flash.h"
#include "pedal.h"

#define PIN_ONBOARD_LED 25

RING(rx, 1024);
RING(tx, 2048);
static lw_pedal pedal;

static uint32_t hal_now(void *c) { (void)c; return to_ms_since_boot(get_absolute_time()); }
static void hal_send(void *c, const uint8_t *m, int n) { (void)c; ring_put_msg(&tx, m, n); }

static void on_uart0(void)
{
    while (uart_is_readable(uart0))
        ring_put(&rx, (uint8_t)uart_getc(uart0));
}

int main(void)
{
    pedal_flash_check();
    stdio_init_all();
    gpio_init(PIN_ONBOARD_LED);
    gpio_set_dir(PIN_ONBOARD_LED, GPIO_OUT);

    uart_init(uart0, MIDI_BAUD);
    gpio_set_function(PIN_LOOP_TX, GPIO_FUNC_UART);
    gpio_set_function(PIN_LOOP_RX, GPIO_FUNC_UART);
    irq_set_exclusive_handler(UART0_IRQ, on_uart0);
    irq_set_enabled(UART0_IRQ, true);
    uart_set_irq_enables(uart0, true, false);

    pd_hal hal = {
        .now_ms = hal_now, .send = hal_send,
        .model = {0x7F, 0x7F},               /* test model: not a real pedal */
        .fw = {0, 1, 0},
        .ncontrols = 4,
        .cc = {20, 21, 22, 80},              /* knobs 1-3 and on/off, as in the spec's Message map */
        .kind = {PD_KNOB, PD_KNOB, PD_KNOB, PD_ONOFF},
        .value = {64, 64, 64, 0},
        .flash = &pedal_flash,
    };
    pico_unique_board_id_t id;
    pico_get_unique_board_id(&id);
    uint64_t u = 0;
    for (int i = 0; i < PICO_UNIQUE_BOARD_ID_SIZE_BYTES; i++)
        u = (u << 8) | id.id[i];
    lw_serial_from_u64(u, hal.serial);
    pd_init(&pedal, &hal);

    uint8_t shown_pos = 0xFF, shown_ch = 0xFE;
    int shown_preset = -2;
    while (true) {
        while (!ring_empty(&rx))
            pd_byte(&pedal, (uint8_t)ring_get(&rx));
        pd_poll(&pedal);
        while (!ring_empty(&tx) && uart_is_writable(uart0))
            uart_putc_raw(uart0, (char)ring_get(&tx));

        /* onboard LED: on once a channel is assigned */
        gpio_put(PIN_ONBOARD_LED, pedal.channel <= 0x0F);
        if (pedal.position != shown_pos || pedal.channel != shown_ch || pedal.last_preset != shown_preset) {
            shown_pos = pedal.position;
            shown_ch = pedal.channel;
            shown_preset = pedal.last_preset;
            if (shown_ch <= 0x0F)                /* MIDI channels are shown 1-16 */
                printf("position %u, channel %u, last preset %d\n", shown_pos, shown_ch + 1u, shown_preset + 1);
            else
                printf("position %u, no channel yet\n", shown_pos);
        }
        int c = getchar_timeout_us(0);
        if (c >= '1' && c <= '4') {
            int k = c - '1';
            uint8_t v = k == 3 ? (uint8_t)(pedal.value[3] ? 0 : 127)        /* 4: on/off toggles */
                               : (uint8_t)((pedal.value[k] + 8) & 0x7F);
            pd_set_control(&pedal, k, v);
            printf("knob %d = %u\n", k + 1, pedal.value[k]);
        }
    }
}
