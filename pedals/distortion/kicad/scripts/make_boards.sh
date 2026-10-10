# Rebuild both boards from the schematics: place, fan out to the planes, autoroute, check.
# WARNING: this overwrites the .kicad_pcb files, including any routing done by hand.
cd "$(dirname "$0")" && . ./env.sh
bash netlists.sh && "$KP" gen_pcb.py && \
for p in Flashover_Controls Flashover; do "$KP" fanout.py $p && bash route.sh $p ${1:-100}; done
bash drc.sh
