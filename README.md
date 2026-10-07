# GreywireStudio

GreywireStudio is a line of guitar effects pedals from Cherry Blossom Development. The first series, **Linework**, is a set of pedals that work on their own or together under a phone-connected control pedal. See `General Concept.docx` for the overall product concept.

## Tools

All tools are free:

| Tool | Purpose |
|------|---------|
| [KiCad](https://www.kicad.org/) | Schematic capture and PCB layout |
| [LTspice](https://www.analog.com/ltspice) | Circuit simulation (gain, clipping, tone response) |

## Layout

```
libraries/
  GreywireStudio.kicad_sym   # Shared KiCad symbols (e.g. 3PDT footswitch)
pedals/
  <pedal-name>/
    README.md   # Design notes, parts list, status
    *Product Document.docx  # Product definition
    kicad/      # KiCad project (schematic + PCB)
    sim/        # LTspice simulations (.asc)
```

## Pedals

| Pedal | Name | Type | Status |
|-------|------|------|--------|
| [Distortion](pedals/distortion/) | Flashover | Distortion / overdrive | Design |
| [Controller](pedals/controller/) | Switchyard | Control pedal (phone-connected) | Concept |

Product names are working names pending a trademark search.
