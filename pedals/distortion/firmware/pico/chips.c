#include "chips.h"
#include "board.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"

static const uint8_t POT_ADDR[3] = {ADDR_POT_GAIN, ADDR_POT_TONE, ADDR_POT_LEVEL};
static bool leds_ok;
static bool pot_ok[3];

void pots_init(void)
{
    i2c_init(i2c0, 400 * 1000);
    gpio_set_function(PIN_POT_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_POT_SCL, GPIO_FUNC_I2C);     /* 4.7k pull-ups on the board */
}

/* MCP45HV51 (datasheet DS20005304A, section 7): command byte = address AD3:AD0 (wiper 0 = 0),
 * command C1:C0 (00 = write data), then D9:D8 (unused), followed by the data byte. WLAT is tied
 * low on the board, so the new value takes effect at once. */
bool pot_write(int pot, uint8_t code)
{
    uint8_t b[2] = {0x00, code};
    pot_ok[pot] = i2c_write_timeout_us(i2c0, POT_ADDR[pot], b, 2, false, 1000) == 2;
    return pot_ok[pot];
}

bool pot_answered(int pot) { return pot_ok[pot]; }
bool leds_answered(void) { return leds_ok; }

static bool reg(uint8_t r, uint8_t v)
{
    uint8_t b[2] = {r, v};
    return i2c_write_timeout_us(i2c1, ADDR_LEDS, b, 2, false, 1000) == 2;
}

/* IS31FL3236A (Lumissil datasheet rev. I): 00h shutdown (1 = normal), 01h-24h PWM, 25h update,
 * 26h-49h LED control (bit 0 on, bits 2:1 current: 00 = full, set by R33 to about 5 mA),
 * 4Ah global (0 = normal), 4Bh PWM frequency (1 = 22 kHz, above hearing), 4Fh reset. */
bool leds_init(void)
{
    i2c_init(i2c1, 400 * 1000);
    gpio_set_function(PIN_LED_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_LED_SCL, GPIO_FUNC_I2C);
    gpio_init(PIN_LED_EN);
    gpio_set_dir(PIN_LED_EN, GPIO_OUT);
    gpio_put(PIN_LED_EN, 1);
    sleep_ms(1);
    leds_ok = reg(0x4F, 0x00) && reg(0x00, 0x01) && reg(0x4B, 0x01) && reg(0x4A, 0x00);
    if (!leds_ok)
        return false;
    uint8_t ctl[37] = {0x26};
    for (int i = 1; i <= 36; i++)
        ctl[i] = 0x01;
    i2c_write_timeout_us(i2c1, ADDR_LEDS, ctl, sizeof ctl, false, 3000);
    return reg(0x25, 0x00);
}

void leds_write(const uint8_t pwm[36])
{
    if (!leds_ok)
        return;                                           /* no driver answering: carry on without */
    uint8_t b[37] = {0x01};
    for (int i = 0; i < 36; i++)
        b[1 + i] = pwm[i];
    i2c_write_timeout_us(i2c1, ADDR_LEDS, b, sizeof b, false, 3000);
    reg(0x25, 0x00);
}
