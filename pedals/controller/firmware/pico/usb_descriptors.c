/* USB descriptors: Switchyard appears to the phone as a class-compliant USB MIDI device
 * with one port, so iPhone and Android need no driver. */
#include "tusb.h"
#include "pico/unique_id.h"
#include <string.h>

/* Development IDs (TinyUSB's example vendor ID). Before selling, get a free product ID from
 * Raspberry Pi for the RP2040 (see the Switchyard Product Document, Certification). */
#define USB_VID 0xCAFE
#define USB_PID 0x4C57                       /* "LW" */

static const tusb_desc_device_t device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = 0x00,                    /* class given per interface */
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = USB_VID,
    .idProduct = USB_PID,
    .bcdDevice = 0x0010,                     /* firmware 0.1.0 */
    .iManufacturer = 1,
    .iProduct = 2,
    .iSerialNumber = 3,
    .bNumConfigurations = 1,
};

const uint8_t *tud_descriptor_device_cb(void) { return (const uint8_t *)&device; }

enum { ITF_MIDI = 0, ITF_MIDI_STREAMING, ITF_TOTAL };
#define EP_MIDI_OUT 0x01
#define EP_MIDI_IN 0x81
#define CONFIG_LEN (TUD_CONFIG_DESC_LEN + TUD_MIDI_DESC_LEN)

static const uint8_t config[] = {
    /* bMaxPower 100 mA: the controller part draws about 60 mA from the phone */
    TUD_CONFIG_DESCRIPTOR(1, ITF_TOTAL, 0, CONFIG_LEN, 0x00, 100),
    TUD_MIDI_DESCRIPTOR(ITF_MIDI, 4, EP_MIDI_OUT, EP_MIDI_IN, 64),
};

const uint8_t *tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index;
    return config;
}

#if SY_RECOVERY
#define SY_USB_PRODUCT "Switchyard (recovery)"   /* the loader in recovery mode (sy_loader.c) */
#else
#define SY_USB_PRODUCT "Switchyard"
#endif

static const char *const strings[] = {
    NULL,                                    /* 0: language, handled below */
    "Greywire Studio",
    SY_USB_PRODUCT,
    NULL,                                    /* 3: serial, from the chip's flash ID */
    "Switchyard MIDI",
};

static uint16_t desc_str[33];

const uint16_t *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void)langid;
    char serial[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];
    const char *s;
    int n;
    if (index == 0) {
        desc_str[1] = 0x0409;                /* English */
        n = 1;
    } else {
        if (index == 3) {
            pico_get_unique_board_id_string(serial, sizeof serial);
            s = serial;
        } else if (index < sizeof strings / sizeof strings[0] && strings[index]) {
            s = strings[index];
        } else {
            return NULL;
        }
        n = (int)strlen(s);
        if (n > 32)
            n = 32;
        for (int i = 0; i < n; i++)
            desc_str[1 + i] = (uint8_t)s[i];
    }
    desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * n + 2));
    return desc_str;
}
