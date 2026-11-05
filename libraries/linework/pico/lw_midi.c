#include "lw_midi.h"
#include "ring.h"
#include "pico/stdlib.h"
#include "pico/unique_id.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

#define MIDI_BAUD 31250

RING(rx, 1024);            /* filled by the UART0 interrupt */
RING(tx, 2048);

static void on_uart0(void)
{
    while (uart_is_readable(uart0))
        ring_put(&rx, (uint8_t)uart_getc(uart0));
}

void lw_midi_init(unsigned tx_pin, unsigned rx_pin)
{
    uart_init(uart0, MIDI_BAUD);
    gpio_set_function(tx_pin, GPIO_FUNC_UART);
    gpio_set_function(rx_pin, GPIO_FUNC_UART);
    uart_set_fifo_enabled(uart0, true);
    irq_set_exclusive_handler(UART0_IRQ, on_uart0);
    irq_set_enabled(UART0_IRQ, true);
    uart_set_irq_enables(uart0, true, false);
}

int lw_midi_get(void) { return ring_get(&rx); }
void lw_midi_send(const uint8_t *m, int n) { ring_put_msg(&tx, m, n); }

void lw_midi_flush(void)
{
    while (!ring_empty(&tx) && uart_is_writable(uart0))
        uart_putc_raw(uart0, (char)ring_get(&tx));
}

void lw_serial(uint8_t out[LW_SERIAL_LEN])
{
    pico_unique_board_id_t id;
    pico_get_unique_board_id(&id);
    uint64_t u = 0;
    for (int i = 0; i < PICO_UNIQUE_BOARD_ID_SIZE_BYTES; i++)
        u = (u << 8) | id.id[i];
    lw_serial_from_u64(u, out);
}
