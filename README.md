# GreywireStudio

GreywireStudio is a line of guitar effects pedals from Cherry Blossom Development, starting with a distortion pedal.

## Tools

All tools are free:

| Tool | Purpose |
|------|---------|
| [KiCad](https://www.kicad.org/) | Schematic capture and PCB layout |
| [LTspice](https://www.analog.com/ltspice) | Circuit simulation (gain, clipping, tone response) |

## Layout

Each pedal has its own folder under `pedals/`:

```
pedals/
  <pedal-name>/
    README.md   # Design notes, parts list, status
    kicad/      # KiCad project (schematic + PCB)
    sim/        # LTspice simulations (.asc)
```

## Pedals

| Pedal | Status |
|-------|--------|
| [Distortion](pedals/distortion/) | Design |
