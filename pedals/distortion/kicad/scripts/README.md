# Flashover generator scripts

These Python scripts generate the Flashover KiCad projects (`../Flashover` and `../Flashover_Controls`).

**Once anyone routes a board by hand in KiCad, the KiCad files become the master copy.** After that,
don't run `make_boards.sh`: it overwrites the `.kicad_pcb` files. The schematic generator
(`gen_flashover.py`) overwrites the schematics in the same way.

| Script | What it does | Run with |
|---|---|---|
| `gen_flashover.py` | Writes both schematics and their project files. Uses `kigen.py` and `kisym.py` | any Python 3 |
| `rules.py` | Design rules and net classes, written into the `.kicad_pro` files | (imported) |
| `netlists.sh` | Exports both netlists to `build/`, for the placement script | bash |
| `gen_pcb.py` | Places every part on both boards, in 1590BB pedal coordinates, and adds the board outline and copper planes | KiCad's Python |
| `fanout.py <project>` | Adds a short track and a via from each surface-mount pad down to its inner plane (GND on both boards, +5V on the control board) | KiCad's Python |
| `route.sh <project> [passes]` | Autoroutes with Freerouting (`route.py` and `dsnfix.py` handle the round trip), then adds ground pours | bash |
| `make_boards.sh [max passes]` | All of the board steps above, for both boards, followed by `drc.sh` | bash |
| `drc.sh` | Runs the design rule check, with schematic parity, on both boards and prints a summary | bash |
| `render.sh` | Renders both boards to `build/*_pcb.png` | bash |

- **Paths:** set in `paths.py` and `env.sh`. The KiCad 10 install paths are in `env.sh`, `gen_pcb.py` and `kisym.py`.
- **`build/`:** not committed. It holds netlists, router files and logs.
- **Freerouting:** download version 2.1.x (it runs on Java 21) from
  <https://github.com/freerouting/freerouting/releases> and save it as `build/freerouting.jar`.
- **Router quirks:**
  - Run headless, Freerouting ignores every pass limit tried (`-mp`, `--router.max_passes`, the settings file). It stops by itself after about 1,000 passes; `route.sh` also stops it after 90 minutes, and a stopped run saves nothing.
  - It can't connect pads at angles other than multiples of 90°, so the ring LEDs and U10 are placed square.
- **KiCad's Python quirk:** `pcbnew.SaveBoard` resets the project's design rules. Every script that
  saves a board calls `rules.apply_file` afterwards to put them back.
