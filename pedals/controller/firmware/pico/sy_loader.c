/* Switchyard's loader: the first 32 KB of flash, started at every power-up (lw_fw.h, with
 * LW_LAYOUT_SWITCHYARD). Pedals have the shared loader in libraries/linework/pico/loader.c;
 * Switchyard needs its own because it is updated over USB, not over the MIDI loop.
 *
 * It installs a waiting update, checks the firmware and starts it. If there's nothing it can
 * start, or both footswitches are held at power-up, it stays in recovery mode:
 *   - it appears over USB as "Switchyard (recovery)", a USB MIDI device like Switchyard;
 *   - it answers Switchyard info and the identity request as Switchyard, firmware 0.0.0, so the
 *     app can tell it needs an update;
 *   - it takes Switchyard's own firmware update (sy_update.c), then restarts into it;
 *   - any other Linework command gets "unknown command";
 *   - both footswitch LEDs flash quickly.
 * The loop and the display are left alone. */
#include <string.h>
#include "pico/stdlib.h"
#include "pico/unique_id.h"
#include "tusb.h"

#include "board.h"
#include "lw_update.h"
#include "sy_update.h"

static bool held(uint pin)
{
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_up(pin);
    busy_wait_us(5000);                         /* let the switch filters settle */
    return !gpio_get(pin);
}

static uint8_t serial[LW_SERIAL_LEN];
static lw_parser parse;
static sy_update upd;

static uint32_t now_ms(void *c) { (void)c; return to_ms_since_boot(get_absolute_time()); }

/* to the app: the USB MIDI buffer is small, so keep serving USB until it has all gone */
static void reply(void *c, const uint8_t *m, int n)
{
    (void)c;
    uint32_t sent = 0, t0 = now_ms(NULL);
    while (sent < (uint32_t)n && tud_mounted() && now_ms(NULL) - t0 < 100) {
        sent += tud_midi_stream_write(0, m + sent, (uint32_t)n - sent);
        tud_task();
    }
}

static void ack(uint8_t qq, uint8_t cmd, uint8_t st)
{
    lw_msg m;
    lw_begin(&m, LW_ACK, qq);
    lw_put(&m, LW_NO_CHANNEL);
    lw_put(&m, cmd);
    lw_put(&m, st);
    lw_end(&m);
    reply(NULL, m.b, m.n);
}

static void on_msg(void *ctx, const uint8_t *m, int n)
{
    (void)ctx;
    if (syu_app_msg(&upd, m, n))
        return;
    if (n == 6 && m[0] == 0xF0 && m[1] == 0x7E && m[3] == 0x06 && m[4] == 0x01) {   /* identity request */
        uint8_t r[16] = {0xF0, 0x7E, 0x7F, 0x06, 0x02};
        int k = 5;
        memcpy(&r[k], &LW_HEADER[1], LW_HDR_LEN - 1);
        k += LW_HDR_LEN - 1;
        r[k++] = 0x02;                          /* Switchyard: model 02 00 */
        r[k++] = 0x00;
        r[k++] = 0;                             /* firmware 0.0.0: recovery mode */
        r[k++] = 0;
        r[k++] = 0;
        r[k++] = LW_SPEC_VERSION;
        r[k++] = 0xF7;
        reply(NULL, r, k);
        return;
    }
    if (!lw_is_linework(m, n))
        return;
    if (LW_CMD(m) == SY_INFO_REQ) {
        lw_msg r;
        lw_begin(&r, SY_INFO, LW_QQ(m));
        lw_put(&r, 0);
        lw_put(&r, 0);
        lw_put(&r, 0);
        lw_put(&r, LW_SPEC_VERSION);
        lw_put_n(&r, serial, LW_SERIAL_LEN);
        lw_put(&r, 0x01);                       /* 9V: not measured here */
        lw_end(&r);
        reply(NULL, r.b, r.n);
        return;
    }
    if (LW_CMD(m) != LW_ACK)
        ack(LW_QQ(m), LW_CMD(m), ST_UNKNOWN);
}

int main(void)
{
    bool recovery = held(PIN_SW_DOWN) && held(PIN_SW_UP);
    if (lw_fw_boot(&lw_flash_whole, recovery, NULL) == FW_BOOT_APP)
        lw_start_app();

    /* recovery mode */
    gpio_init(PIN_LED_DOWN);
    gpio_set_dir(PIN_LED_DOWN, GPIO_OUT);
    gpio_init(PIN_LED_UP);
    gpio_set_dir(PIN_LED_UP, GPIO_OUT);
    pico_unique_board_id_t id;
    pico_get_unique_board_id(&id);
    uint64_t u = 0;
    for (int i = 0; i < PICO_UNIQUE_BOARD_ID_SIZE_BYTES; i++)
        u = (u << 8) | id.id[i];
    lw_serial_from_u64(u, serial);              /* the same serial as Switchyard's firmware */

    syu_init(&upd);
    upd.hal = &lw_update_hal;
    upd.now_ms = now_ms;
    upd.reply = reply;
    upd.serial = serial;
    lw_parser_init(&parse, on_msg, NULL);
    tusb_init();

    while (true) {
        tud_task();
        uint8_t buf[64];
        while (tud_mounted() && tud_midi_available()) {
            uint32_t n = tud_midi_stream_read(buf, sizeof buf);
            if (!n)
                break;
            for (uint32_t i = 0; i < n; i++)
                lw_parse(&parse, buf[i]);
        }
        syu_poll(&upd);
        bool on = (now_ms(NULL) / 100) % 2;
        gpio_put(PIN_LED_DOWN, on);
        gpio_put(PIN_LED_UP, on);
    }
}
