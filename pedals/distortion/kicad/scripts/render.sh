# Render both boards to build/<project>_pcb.png for a quick look
cd "$(dirname "$0")" && . ./env.sh
for p in Flashover Flashover_Controls; do "$K" pcb export pdf --mode-single -l F.Cu,B.Cu,F.Fab,B.Fab,F.Courtyard,B.Courtyard,Edge.Cuts,User.Drawings,F.Silkscreen -o build/$p.pcb.pdf $KD/$p/$p.kicad_pcb >/dev/null 2>&1; done
python -c "
import pymupdf
for p in ('Flashover','Flashover_Controls'):
    d=pymupdf.open('build/'+p+'.pcb.pdf'); pg=d[0]
    r=pymupdf.Rect(95*72/25.4, 40*72/25.4, 200*72/25.4, 180*72/25.4)
    pg.get_pixmap(dpi=200, clip=r).save('build/'+p+'_pcb.png')
"
