# Switchyard - KiCad project

Schematic only, started Nov 4, 2026. There is no board layout yet. That waits for the enclosure layout and for the stage 2 breadboard to prove phone charging.

| Folder | What's in it |
|---|---|
| `Switchyard/` | The whole pedal on one board: power, USB-C and phone charging, controller, MIDI, footswitches, LEDs and display |
| `scripts/gen_switchyard.py` | Writes the project |

The project has one board. Whether the display and LEDs move to a second board behind the lid, as on Flashover and Tap Changer, gets decided with the enclosure layout.

## Sheets

| Sheet | What's on it |
|---|---|
| Power | 9V jack and reverse-polarity diode; U3, a 9V-to-5V switching converter (see Glossary); the 9V sense line; D2 and D4, which pick 5V from either the 9V supply or the phone |
| USB | USB-C jack J5, ESD protection U9, U7 (FUSB302B USB-C power controller), and U8, the switch that gives the phone its keep-alive charge |
| Controller | RP2040, flash, crystal, 3.3V regulator, SWD header, and a BOOTSEL button |
| MIDI | Loop in (J2), loop out (J1), and the out for other brands (J3) |
| Panel | Down and Up footswitches with their LEDs, the HT16K33 display driver (U6), and the 4-digit display (DS1) |
| Mounting | Four M3 holes, placed once the enclosure is chosen |

## How it's made

`scripts/gen_switchyard.py` writes the project. Run it with any Python 3. It reuses Flashover's schematic writer in `pedals/distortion/kicad/scripts/`, and the controller and MIDI circuits are copied from Tap Changer. It overwrites the schematics each time it runs, so once they're edited by hand in KiCad, stop running it.

- **Matches the breadboard:** the GPIO numbers and part references (U1, U2, R1-R10, D1-D3, J1-J4, SW1, SW2, LED1, LED2, DS1) are the same as in `SWITCHYARD_BREADBOARD_STAGE1.md`. That means the breadboard firmware carries over.
- **Display:** U6 is the same HT16K33 chip as on the breadboard's Adafruit backpack, and its digits are wired to the same outputs (COM0, COM1, COM3, COM4). So the backpack's display code works unchanged. KiCad has no symbol for it, so one was added to `libraries/GreywireStudio.kicad_sym` (20-pin package, pins from Holtek's datasheet v1.20).
- **New since the breadboard:** the BOOTSEL button. It's used to load firmware onto a blank chip over USB.

### New GPIO pins

These go on stage 2 of the breadboard, the stage that adds phone charging:

| GPIO | Net | Use |
|---|---|---|
| 8 | PD_INT | Interrupt from U7, the USB-C power controller |
| 9 | VBUS_EN | Turns on the keep-alive charge (U8) |
| 10 | VBUS_FLT | U8 reports an overcurrent |
| 26 | SENSE_9V | Tells the firmware whether the 9V supply is plugged in (ADC0) |

## Phone charging (draft, unproven)

1. With no 9V, U7 presents itself as a power sink, even while unpowered. So the phone powers Switchyard through D4.
2. With 9V present, the firmware asks the phone for a power role swap (see Glossary). The phone keeps running the USB data.
3. Once the swap is agreed, the firmware turns on U8, which sends 5V at up to 0.5 A from U3 to the phone.

The firmware needs a USB Power Delivery (PD) stack to do this. Phones handle role swaps in different ways, so this is the part to test first on the stage 2 breadboard, on several iPhone and Android models. The product document already says the same.

## Open points

- **U7's VCONN pins are tied to ground.** The datasheet doesn't say what to do with them when they're unused, and the firmware never turns VCONN on. Check against onsemi's evaluation board before layout.
- **There are no capacitors on the CC lines.** The USB-C spec allows 200-600 pF there, but it's optional. Revisit if the breadboard shows PD message errors.
- **The display runs from 3.3V,** as on the breadboard. That's slightly dimmer than 5V, but it keeps the I2C levels matched with the RP2040.
- **The footprint choices are first picks.** These are the USB-C jack (GCT USB4105), the inductor (Bourns SRN4018) and the display (Kingbright 0.56-inch). Revisit them at layout.

## Checks

On Nov 4, 2026, the project passed KiCad's electrical rules check with no errors or warnings. Every net in the exported netlist was checked by hand against the design: the MIDI jacks match the breadboard, both USB data lines run through U9 and R14/R15 to the RP2040, and the display's segments and digits match the Adafruit backpack's wiring.

## Glossary

- **Switching converter (buck):** a regulator that switches the input on and off quickly and smooths the result with an inductor and capacitors. It wastes far less power than a linear regulator, which turns the extra voltage into heat. That matters here because of the 0.5 A phone charge.
- **Power role swap:** a USB Power Delivery message that swaps which side supplies power while the data roles stay the same. Here the phone stays the USB host, but Switchyard becomes the power source.
- **VBUS:** the USB 5V power wire.
- **CC lines:** USB-C's two configuration pins. Resistors on them tell each side what's connected and which way the plug is turned, and PD messages travel over them.
- **VCONN:** power for chips inside some USB-C cables. Switchyard doesn't need it.
- **BOOTSEL:** the RP2040's built-in loader. Held at power-up, it makes the chip appear as a USB drive for loading firmware.
