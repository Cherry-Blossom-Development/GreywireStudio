# DRC both boards (with schematic parity) and summarise
cd "$(dirname "$0")" && . ./env.sh
for p in Flashover Flashover_Controls; do "$K" pcb drc --refill-zones --schematic-parity --severity-all --format json -o build/$p.drc.json $KD/$p/$p.kicad_pcb >/dev/null 2>&1; python - $p <<'PY'
import json, sys, collections
d = json.load(open('build/' + sys.argv[1] + '.drc.json'))
print(sys.argv[1], 'unconnected:', len(d.get('unconnected_items', [])), 'parity:', len(d.get('schematic_parity', [])))
c = collections.Counter(v['type'] for v in d['violations'])
print(' ', dict(c))
for v in d['violations']:
    if v['type'] not in ('silk_overlap', 'silk_over_copper', 'silk_edge_clearance', 'text_height', 'text_thickness'):
        print('   ', v['type'], '|', ' / '.join(i['description'] for i in v['items'])[:150])
for v in d.get('schematic_parity', [])[:10]:
    print('   parity', v['type'], v['description'][:120], v['items'][0]['description'][:40])
PY
done
