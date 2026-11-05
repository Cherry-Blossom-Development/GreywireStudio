#include "lw_chips.h"
#include "pico/stdlib.h"

static i2c_inst_t *led_bus;
static uint8_t led_addr;
static bool leds_ok;

void lw_i2c_init(i2c_inst_t *i2c, unsigned sda, unsigned scl)
{
    i2c_init(i2c, 400 * 1000);
    gpio_set_function(sda, GPIO_FUNC_I2C);
    gpio_set_function(scl, GPIO_FUNC_I2C);
}

bool lw_pot_write(i2c_inst_t *i2c, uint8_t addr, uint8_t code)
{
    uint8_t b[2] = {0x00, code};
    return i2c_write_timeout_us(i2c, addr, b, 2, false, 1000) == 2;
}

static bool reg(uint8_t r, uint8_t v)
{
    uint8_t b[2] = {r, v};
    return i2c_write_timeout_us(led_bus, led_addr, b, 2, false, 1000) == 2;
}

bool lw_leds_init(i2c_inst_t *i2c, uint8_t addr, unsigned en_pin)
{
    led_bus = i2c;
    led_addr = addr;
    gpio_init(en_pin);
    gpio_set_dir(en_pin, GPIO_OUT);
    gpio_put(en_pin, 1);
    sleep_ms(1);
    leds_ok = reg(0x4F, 0x00) && reg(0x00, 0x01) && reg(0x4B, 0x01) && reg(0x4A, 0x00);
    if (!leds_ok)
        return false;
    uint8_t ctl[37] = {0x26};
    for (int i = 1; i <= 36; i++)
        ctl[i] = 0x01;
    i2c_write_timeout_us(led_bus, led_addr, ctl, sizeof ctl, false, 3000);
    leds_ok = reg(0x25, 0x00);
    return leds_ok;
}

void lw_leds_write(const uint8_t pwm[36])
{
    if (!leds_ok)
        return;
    uint8_t b[37] = {0x01};
    for (int i = 0; i < 36; i++)
        b[1 + i] = pwm[i];
    i2c_write_timeout_us(led_bus, led_addr, b, sizeof b, false, 3000);
    reg(0x25, 0x00);
}

bool lw_leds_answered(void) { return leds_ok; }
