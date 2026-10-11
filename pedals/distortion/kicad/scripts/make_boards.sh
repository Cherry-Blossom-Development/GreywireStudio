# Rebuild the main board from the schematic: place, fan out to the ground plane, autoroute, check.
# WARNING: this overwrites Flashover.kicad_pcb, including any routing done by hand.
# The control board is hand-routed and is left alone (see README.md).
cd "$(dirname "$0")" && . ./env.sh
bash netlists.sh && "$KP" gen_pcb.py && "$KP" fanout.py Flashover && bash route.sh Flashover ${1:-100}
bash drc.sh
