#include "ht16k33.h"
#include <string.h>

static i2c_inst_t *bus;
static uint8_t address;
static uint8_t shown[17];
static bool ok;

static const uint8_t COM_OF_DIGIT[4] = {0, 1, 3, 4};
static const uint8_t DIGITS[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};

static bool cmd(uint8_t c)
{
    return i2c_write_timeout_us(bus, address, &c, 1, false, 2000) == 1;
}

bool ht16k33_init(i2c_inst_t *i2c, uint8_t addr, uint8_t brightness)
{
    bus = i2c;
    address = addr;
    ok = cmd(0x21)                           /* oscillator on */
        && cmd(0x81)                         /* display on, no blinking */
        && cmd(0xE0 | (brightness & 0x0F));
    memset(shown, 0xFF, sizeof shown);       /* force the first write */
    return ok;
}

void ht16k33_show(const char text[4], uint8_t dp)
{
    if (!ok)
        return;                              /* no display fitted: carry on without it */
    uint8_t ram[17] = {0x00};                /* display RAM address 0, then 16 bytes */
    for (int d = 0; d < 4; d++) {
        char c = text[d];
        uint8_t seg = (c >= '0' && c <= '9') ? DIGITS[c - '0'] : (c == '-') ? 0x40 : 0x00;
        if (dp & (1u << d))
            seg |= 0x80;
        ram[1 + 2 * COM_OF_DIGIT[d]] = seg;
    }
    if (memcmp(ram, shown, sizeof ram) == 0)
        return;
    if (i2c_write_timeout_us(bus, address, ram, sizeof ram, false, 5000) == (int)sizeof ram)
        memcpy(shown, ram, sizeof ram);
}
