/* The Linework loader: the first 32 KB of flash, started at every power-up (lw_fw.h).
 *
 * It installs a waiting update, checks the firmware, and starts it. If there's nothing it can
 * start, or the pedal's recovery buttons are held at power-up, it stays in recovery mode: a
 * minimal Linework pedal that keeps the MIDI loop going, answers Switchyard (as its model,
 * firmware 0.0.0) and takes a firmware update. The Link LED flashes quickly meanwhile.
 *
 * Built once per pedal, with these definitions (from its CMakeLists.txt):
 *   LOADER_MODEL0, LOADER_MODEL1   the pedal's model number
 *   LOADER_KEY1, LOADER_KEY2       the two recovery buttons (pressed = low)
 *   LOADER_LINK_LED                the Link LED's pin
 *   LOADER_MIDI_TX, LOADER_MIDI_RX
 * The relays and pots are left alone: at power-up they rest in bypass. */
#include "pico/stdlib.h"
#include "lw_update.h"
#include "lw_midi.h"
#include "pedal_flash.h"

static bool held(uint pin)
{
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_up(pin);
    busy_wait_us(5000);                         /* let the switch filters settle */
    return !gpio_get(pin);
}

static lw_pedal pd;
static uint32_t now_ms(void *c) { (void)c; return to_ms_since_boot(get_absolute_time()); }
static void send(void *c, const uint8_t *m, int n) { (void)c; lw_midi_send(m, n); }

int main(void)
{
    bool recovery = held(LOADER_KEY1) && held(LOADER_KEY2);
    if (lw_fw_boot(&lw_flash_whole, recovery, NULL) == FW_BOOT_APP)
        lw_start_app();

    /* recovery mode */
    gpio_init(LOADER_LINK_LED);
    gpio_set_dir(LOADER_LINK_LED, GPIO_OUT);
    lw_midi_init(LOADER_MIDI_TX, LOADER_MIDI_RX);
    pd_hal h = {
        .now_ms = now_ms, .send = send, .update = &lw_update_hal, .flash = &pedal_flash,
        .model = {LOADER_MODEL0, LOADER_MODEL1}, .fw = {0, 0, 0}, .ncontrols = 0,
    };
    lw_serial(h.serial);
    pd_init(&pd, &h);
    while (true) {
        int b;
        while ((b = lw_midi_get()) >= 0)
            pd_byte(&pd, (uint8_t)b);
        pd_poll(&pd);
        lw_midi_flush();
        gpio_put(LOADER_LINK_LED, (now_ms(NULL) / 100) % 2);
    }
}
