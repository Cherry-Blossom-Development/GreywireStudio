/* Flashover on the RP2040: connects the core (core/flashover.c) to the hardware.
 *
 * Power-up: holding the footswitch while plugging in starts setup mode (pick a channel with
 * Gain, press the footswitch to save). Recovery mode (footswitch and Soft/Hard held) needs the
 * update loader, which isn't written yet. */
#include "pico/stdlib.h"
#include "pico/unique_id.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"

#include "board.h"
#include "chips.h"
#include "ring.h"
#include "pedal_flash.h"
#include "flashover.h"

RING(rx, 1024);            /* filled by the UART0 interrupt */
RING(tx, 2048);
static flashover fo;

/* ---------------------------------------------------------------- hal for the core */

static uint32_t hal_now(void *c) { (void)c; return to_ms_since_boot(get_absolute_time()); }
static void hal_send(void *c, const uint8_t *m, int n) { (void)c; ring_put_msg(&tx, m, n); }
static void hal_pot(void *c, int pot, uint8_t code) { (void)c; pot_write(pot, code); }
static void hal_relays(void *c, bool hard, bool on)
{
    (void)c;
    gpio_put(PIN_HARD_SEL, hard);
    gpio_put(PIN_FX_ON, on);
}
static void hal_leds(void *c, const uint8_t p[FO_LEDS]) { (void)c; leds_write(p); }
static void hal_link(void *c, bool on) { (void)c; gpio_put(PIN_LINK_LED, on); }

/* ---------------------------------------------------------------- MIDI */

static void on_uart0(void)
{
    while (uart_is_readable(uart0))
        ring_put(&rx, (uint8_t)uart_getc(uart0));
}

static void midi_init(void)
{
    uart_init(uart0, MIDI_BAUD);
    gpio_set_function(PIN_MIDI_TX, GPIO_FUNC_UART);
    gpio_set_function(PIN_MIDI_RX, GPIO_FUNC_UART);
    uart_set_fifo_enabled(uart0, true);
    irq_set_exclusive_handler(UART0_IRQ, on_uart0);
    irq_set_enabled(UART0_IRQ, true);
    uart_set_irq_enables(uart0, true, false);
}

/* ---------------------------------------------------------------- encoders */

/* Quadrature: each valid change of the two pins is a quarter step, + or -, and invalid jumps
 * (a missed change) count as nothing. A click is counted when the pins return to both-high
 * (pull-ups, contacts open), having moved at least half a cycle. This gives one count per
 * click whichever state the encoder rests in. */
static const int8_t QUARTER[16] = {0, +1, -1, 0, -1, 0, 0, +1, +1, 0, 0, -1, 0, -1, +1, 0};
static const uint ENC_A[3] = {PIN_GAIN_A, PIN_TONE_A, PIN_LEVEL_A};
static const uint ENC_B[3] = {PIN_GAIN_B, PIN_TONE_B, PIN_LEVEL_B};
static const bool ENC_REV[3] = ENC_REVERSE;
static volatile int clicks[3];
static uint8_t enc_state[3];
static int8_t enc_acc[3];

static void on_gpio(uint gpio, uint32_t events)
{
    (void)gpio;
    (void)events;
    for (int k = 0; k < 3; k++) {
        uint8_t s = (uint8_t)(gpio_get(ENC_A[k]) << 1 | gpio_get(ENC_B[k]));
        if (s == enc_state[k])
            continue;
        enc_acc[k] += QUARTER[enc_state[k] << 2 | s];
        enc_state[k] = s;
        if (s == 3) {
            if (enc_acc[k] >= 2)
                clicks[k]++;
            else if (enc_acc[k] <= -2)
                clicks[k]--;
            enc_acc[k] = 0;
        }
    }
}

static void encoders_init(void)
{
    for (int k = 0; k < 3; k++) {
        uint p[2] = {ENC_A[k], ENC_B[k]};
        for (int j = 0; j < 2; j++) {
            gpio_init(p[j]);
            gpio_set_dir(p[j], GPIO_IN);
            gpio_pull_up(p[j]);
        }
        enc_state[k] = (uint8_t)(gpio_get(ENC_A[k]) << 1 | gpio_get(ENC_B[k]));
    }
    gpio_set_irq_enabled_with_callback(ENC_A[0], GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, on_gpio);
    for (int k = 0; k < 3; k++) {
        gpio_set_irq_enabled(ENC_A[k], GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
        gpio_set_irq_enabled(ENC_B[k], GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    }
}

static void encoders_poll(void)
{
    for (int k = 0; k < 3; k++) {
        uint32_t irq = save_and_disable_interrupts();
        int n = clicks[k];
        clicks[k] = 0;
        restore_interrupts(irq);
        if (n)
            fo_turn(&fo, FO_GAIN + k, ENC_REV[k] ? -n : n);
    }
}

/* ---------------------------------------------------------------- buttons */

typedef struct { uint pin; int id; bool down; uint8_t count; } button;
static button btn[5] = {
    {PIN_FOOTSW, FO_BTN_FOOT, false, 0}, {PIN_MODE_BTN, FO_BTN_MODE, false, 0},
    {PIN_GAIN_SW, FO_BTN_GAIN, false, 0}, {PIN_TONE_SW, FO_BTN_TONE, false, 0}, {PIN_LEVEL_SW, FO_BTN_LEVEL, false, 0},
};

static void buttons_init(void)
{
    for (int i = 0; i < 5; i++) {
        gpio_init(btn[i].pin);
        gpio_set_dir(btn[i].pin, GPIO_IN);
        gpio_pull_up(btn[i].pin);
    }
    sleep_ms(5);                             /* let the footswitch filter (10k, 100n) settle */
    for (int i = 0; i < 5; i++)
        btn[i].down = !gpio_get(btn[i].pin); /* held at power-up: its release isn't a press */
}

/* once a millisecond: a change must hold for 10 ms to count */
static void buttons_poll(void)
{
    for (int i = 0; i < 5; i++) {
        bool down = !gpio_get(btn[i].pin);
        if (down == btn[i].down) {
            btn[i].count = 0;
            continue;
        }
        if (++btn[i].count < 10)
            continue;
        btn[i].down = down;
        btn[i].count = 0;
        if (down)
            fo_press(&fo, btn[i].id);
    }
}

/* ---------------------------------------------------------------- main */

static void out_pin(uint pin)
{
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, 0);
}

int main(void)
{
    pedal_flash_check();
    out_pin(PIN_HARD_SEL);                   /* relays off: Soft, bypassed */
    out_pin(PIN_FX_ON);
    out_pin(PIN_LINK_LED);
    buttons_init();
    bool setup = btn[0].down;                /* footswitch held while plugging in */
    midi_init();
    pots_init();
    leds_init();

    fo_hal hal = {
        .now_ms = hal_now, .midi_send = hal_send, .set_pot = hal_pot, .set_relays = hal_relays,
        .set_leds = hal_leds, .set_link_led = hal_link, .flash = &pedal_flash,
    };
    pico_unique_board_id_t id;
    pico_get_unique_board_id(&id);
    uint64_t u = 0;
    for (int i = 0; i < PICO_UNIQUE_BOARD_ID_SIZE_BYTES; i++)
        u = (u << 8) | id.id[i];
    lw_serial_from_u64(u, hal.serial);
    fo_init(&fo, &hal, setup);
    encoders_init();

    uint32_t last_ms = hal_now(NULL);
    while (true) {
        while (!ring_empty(&rx))
            fo_midi_byte(&fo, (uint8_t)ring_get(&rx));
        uint32_t t = hal_now(NULL);
        if (t != last_ms) {
            last_ms = t;
            buttons_poll();
        }
        encoders_poll();
        fo_poll(&fo);
        while (!ring_empty(&tx) && uart_is_writable(uart0))
            uart_putc_raw(uart0, (char)ring_get(&tx));
    }
}
