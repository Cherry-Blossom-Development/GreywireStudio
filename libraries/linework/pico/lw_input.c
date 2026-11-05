#include "lw_input.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"

/* Quadrature: each valid change of the two pins is a quarter step, + or -, and invalid jumps
 * (a missed change) count as nothing. A click is counted when the pins return to both-high
 * (pull-ups, contacts open), having moved at least half a cycle. This gives one count per
 * click whichever state the encoder rests in. */
static const int8_t QUARTER[16] = {0, +1, -1, 0, -1, 0, 0, +1, +1, 0, 0, -1, 0, -1, +1, 0};
static int nenc;
static uint enc_a[LW_MAX_ENC], enc_b[LW_MAX_ENC];
static volatile int clicks[LW_MAX_ENC];
static uint8_t state[LW_MAX_ENC];
static int8_t acc[LW_MAX_ENC];

static uint8_t read_ab(int k) { return (uint8_t)(gpio_get(enc_a[k]) << 1 | gpio_get(enc_b[k])); }

static void on_gpio(uint gpio, uint32_t events)
{
    (void)gpio;
    (void)events;
    for (int k = 0; k < nenc; k++) {
        uint8_t s = read_ab(k);
        if (s == state[k])
            continue;
        acc[k] += QUARTER[state[k] << 2 | s];
        state[k] = s;
        if (s == 3) {
            if (acc[k] >= 2)
                clicks[k]++;
            else if (acc[k] <= -2)
                clicks[k]--;
            acc[k] = 0;
        }
    }
}

void lw_enc_init(int n, const uint *a, const uint *b)
{
    nenc = n > LW_MAX_ENC ? LW_MAX_ENC : n;
    for (int k = 0; k < nenc; k++) {
        enc_a[k] = a[k];
        enc_b[k] = b[k];
        uint p[2] = {a[k], b[k]};
        for (int j = 0; j < 2; j++) {
            gpio_init(p[j]);
            gpio_set_dir(p[j], GPIO_IN);
            gpio_pull_up(p[j]);
        }
        state[k] = read_ab(k);
    }
    const uint32_t both = GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL;
    gpio_set_irq_enabled_with_callback(enc_a[0], both, true, on_gpio);
    for (int k = 0; k < nenc; k++) {
        gpio_set_irq_enabled(enc_a[k], both, true);
        gpio_set_irq_enabled(enc_b[k], both, true);
    }
}

int lw_enc_take(int k)
{
    uint32_t irq = save_and_disable_interrupts();
    int n = clicks[k];
    clicks[k] = 0;
    restore_interrupts(irq);
    return n;
}

void lw_buttons_init(lw_button *b, int n)
{
    for (int i = 0; i < n; i++) {
        gpio_init(b[i].pin);
        gpio_set_dir(b[i].pin, GPIO_IN);
        gpio_pull_up(b[i].pin);
    }
    sleep_ms(5);
    for (int i = 0; i < n; i++) {
        b[i].down = !gpio_get(b[i].pin);
        b[i].count = 0;
    }
}

void lw_buttons_poll(lw_button *b, int n, void (*press)(int id))
{
    for (int i = 0; i < n; i++) {
        bool down = !gpio_get(b[i].pin);
        if (down == b[i].down) {
            b[i].count = 0;
            continue;
        }
        if (++b[i].count < 10)
            continue;
        b[i].down = down;
        b[i].count = 0;
        if (down)
            press(b[i].id);
    }
}
