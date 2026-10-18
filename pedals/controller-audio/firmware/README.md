# Switchyard Audio firmware (Daisy Seed)

Firmware for the Switchyard Audio breadboard prototypes. It's written in C++ with Electrosmith's free libraries, which are included as git submodules in `lib/`:

| Library | Version | What it gives us |
| --- | --- | --- |
| `lib/libDaisy` | v9.0.0 | Talks to the Daisy's hardware: audio, knobs, switches, pins |
| `lib/DaisySP` | master as of Sep 28, 2026 | Ready-made sound-processing parts (delay lines, filters) |

DaisySP has a submodule of its own, `DaisySP-LGPL`, which we deliberately don't download or use. Its code is under the LGPL license, which puts conditions on products that ship it. Everything else is under the MIT license, which only asks us to keep the copyright notice.

## Programs

| Folder | What it does |
| --- | --- |
| `stage1_delay/` | Test delay for breadboard stage 1. Time (50 ms to 1 s), Repeats and Mix knobs; the footswitch turns the effect and the bypass relay on and off. Wiring: `../SWITCHYARD_AUDIO_BREADBOARD_STAGE1.md` |

## One-time setup

These tools were installed with winget on Oct 18, 2026 and are on the PATH (open a new terminal if a command isn't found):

```
winget install --id Arm.GnuArmEmbeddedToolchain -e    # ARM compiler, 14.2
winget install --id ezwinports.make -e                # make
```

Download the libraries (only needed once, or after a fresh clone):

```
git submodule update --init pedals/controller-audio/firmware/lib/libDaisy pedals/controller-audio/firmware/lib/DaisySP
git -C pedals/controller-audio/firmware/lib/libDaisy submodule update --init
```

Build the libraries (takes a minute or two):

```
cd pedals/controller-audio/firmware/lib/libDaisy && make
cd ../DaisySP && make
```

## Building a program

```
cd pedals/controller-audio/firmware/stage1_delay
make
```

The result is `build/stage1_delay.bin`.

## Loading it onto the Daisy

1. Plug the Daisy into the computer with the micro-USB data cable.
2. Put it into loading mode (called DFU, see the Glossary): hold the **BOOT** button, press and release **RESET**, then release BOOT.
3. Open the Daisy Web Programmer at https://flash.daisy.audio in Chrome or Edge (other browsers can't talk to USB devices).
4. Click Connect, choose the Daisy ("DFU in FS Mode"), then choose `build/stage1_delay.bin` and click Program.

The Daisy restarts and runs the program when it's done. If the browser can't see the Daisy on Windows, Electrosmith's getting-started guide has a one-time USB driver step.

## Glossary

**DFU (Device Firmware Update):** a standard USB mode for loading a program onto a chip. The Daisy's processor has it built in, so a Daisy can always be reloaded, even if a program crashes.

**Submodule:** a git repository kept inside another one at a fixed version. `git submodule update --init` downloads it. It means the code here always builds against exactly the library version it was written for.
