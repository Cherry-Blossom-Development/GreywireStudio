/* Switchyard on the RP2040: connects the core (core/switchyard.c) to the hardware.
 * Stage 1 breadboard: Pico, Adafruit 878 display, two footswitches with LEDs, three MIDI jacks.
 * Phone charging (USB-C power controller) comes with stage 2. */
#include "pico/stdlib.h"
#include "pico/unique_id.h"
#include "hardware/uart.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "hardware/adc.h"
#include "tusb.h"

#include "board.h"
#include "ring.h"
#include "flash_store.h"
#include "ht16k33.h"
#include "switchyard.h"
#include "lw_update.h"

RING(loop_in, 1024);       /* filled by the UART0 interrupt */
RING(loop_out, 4096);      /* to the MIDI loop */
RING(other_out, 512);      /* to other brands' pedals */
RING(app_out, 4096);       /* to the phone */

static switchyard sy;

/* ---------------------------------------------------------------- hal for the core */

static uint32_t hal_now(void *c) { (void)c; return to_ms_since_boot(get_absolute_time()); }
static void hal_loop_send(void *c, const uint8_t *m, int n) { (void)c; ring_put_msg(&loop_out, m, n); }
static void hal_other_send(void *c, const uint8_t *m, int n) { (void)c; ring_put_msg(&other_out, m, n); }
static void hal_app_send(void *c, const uint8_t *m, int n) { (void)c; ring_put_msg(&app_out, m, n); }
static bool hal_app_connected(void *c) { (void)c; return tud_mounted(); }
static void hal_display(void *c, const char t[4], uint8_t dp) { (void)c; ht16k33_show(t, dp); }

static bool hal_power_9v(void *c)
{
    (void)c;
#if BOARD_HAS_SENSE_9V
    adc_select_input(PIN_SENSE_9V - 26);
    return adc_read() > 2000;                /* about 3V when 9V is in, 0V when it isn't */
#else
    return true;                             /* breadboard: no sense divider */
#endif
}

/* ---------------------------------------------------------------- MIDI UARTs */

static void on_uart0(void)
{
    while (uart_is_readable(uart0))
        ring_put(&loop_in, (uint8_t)uart_getc(uart0));
}

static void midi_init(void)
{
    uart_init(uart0, MIDI_BAUD);
    gpio_set_function(PIN_LOOP_TX, GPIO_FUNC_UART);
    gpio_set_function(PIN_LOOP_RX, GPIO_FUNC_UART);
    uart_set_fifo_enabled(uart0, true);
    irq_set_exclusive_handler(UART0_IRQ, on_uart0);
    irq_set_enabled(UART0_IRQ, true);
    uart_set_irq_enables(uart0, true, false);

    uart_init(uart1, MIDI_BAUD);             /* other brands: transmit only */
    gpio_set_function(PIN_OTHER_TX, GPIO_FUNC_UART);
}

static void flush_uart(uart_inst_t *u, ring *r)
{
    while (!ring_empty(r) && uart_is_writable(u))
        uart_putc_raw(u, (char)ring_get(r));
}

/* ---------------------------------------------------------------- USB MIDI */

static void usb_poll(void)
{
    tud_task();
    if (!tud_mounted()) {
        app_out.tail = app_out.head;         /* nobody listening: drop */
        return;
    }
    uint8_t buf[64];
    while (tud_midi_available()) {
        uint32_t n = tud_midi_stream_read(buf, sizeof buf);
        if (!n)
            break;
        for (uint32_t i = 0; i < n; i++)
            sy_app_byte(&sy, buf[i]);
    }
    while (!ring_empty(&app_out)) {          /* contiguous chunk; stream_write keeps its own state */
        uint32_t t = app_out.tail & app_out.mask;
        uint32_t k = ring_used(&app_out);
        if (k > app_out.mask + 1 - t)
            k = app_out.mask + 1 - t;
        if (k > sizeof buf)
            k = sizeof buf;
        uint32_t w = tud_midi_stream_write(0, &app_out.buf[t], k);
        app_out.tail += w;
        if (w < k)
            break;                           /* USB buffer full: try again next time */
    }
}

/* ---------------------------------------------------------------- footswitches and LEDs */

typedef struct { uint pin, led; bool state; uint8_t count; int dir; } footswitch;
static footswitch fs[2] = {{PIN_SW_DOWN, PIN_LED_DOWN, false, 0, -1}, {PIN_SW_UP, PIN_LED_UP, false, 0, +1}};

static void switches_init(void)
{
    for (int i = 0; i < 2; i++) {
        gpio_init(fs[i].pin);
        gpio_set_dir(fs[i].pin, GPIO_IN);
        gpio_pull_up(fs[i].pin);             /* pressed = low */
        gpio_init(fs[i].led);
        gpio_set_dir(fs[i].led, GPIO_OUT);
        gpio_put(fs[i].led, 0);
    }
}

/* called once a millisecond: a change must hold for 10 ms to count */
static void switches_poll(void)
{
    for (int i = 0; i < 2; i++) {
        bool pressed = !gpio_get(fs[i].pin);
        if (pressed == fs[i].state) {
            fs[i].count = 0;
            continue;
        }
        if (++fs[i].count < 10)
            continue;
        fs[i].state = pressed;
        fs[i].count = 0;
        gpio_put(fs[i].led, pressed);        /* the LED lights while the switch is held */
        if (pressed)
            sy_step(&sy, fs[i].dir);         /* pressing is the only action */
    }
}

/* ---------------------------------------------------------------- main */

int main(void)
{
    store_check_fits();
    switches_init();
    midi_init();

    i2c_init(i2c1, 400 * 1000);
    gpio_set_function(PIN_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_SCL, GPIO_FUNC_I2C);
    ht16k33_init(i2c1, 0x70, 8);

#if BOARD_HAS_SENSE_9V
    adc_init();
    adc_gpio_init(PIN_SENSE_9V);
#endif

    tusb_init();

    sy_hal hal = {
        .now_ms = hal_now, .loop_send = hal_loop_send, .other_send = hal_other_send,
        .app_send = hal_app_send, .app_connected = hal_app_connected, .power_9v = hal_power_9v,
        .store_read = store_read, .store_write = store_write, .store_erase = store_erase,
        .display = hal_display, .update = &lw_update_hal,   /* its own updates over USB */
    };
    pico_unique_board_id_t id;
    pico_get_unique_board_id(&id);
    uint64_t u = 0;
    for (int i = 0; i < PICO_UNIQUE_BOARD_ID_SIZE_BYTES; i++)
        u = (u << 8) | id.id[i];
    lw_serial_from_u64(u, hal.serial);
    sy_init(&sy, &hal);

    uint32_t last_ms = hal_now(NULL);
    while (true) {
        usb_poll();
        while (!ring_empty(&loop_in))
            sy_loop_byte(&sy, (uint8_t)ring_get(&loop_in));
        uint32_t t = hal_now(NULL);
        if (t != last_ms) {
            last_ms = t;
            switches_poll();
        }
        sy_poll(&sy);
        flush_uart(uart0, &loop_out);
        flush_uart(uart1, &other_out);
    }
}
