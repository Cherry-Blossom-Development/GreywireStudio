# Export both schematics' netlists into build/, for gen_pcb.py
cd "$(dirname "$0")" && . ./env.sh
for p in Flashover Flashover_Controls; do "$K" sch export netlist --format kicadxml -o build/$p.xml "$KD/$p/$p.kicad_sch"; done
