# shared settings for the shell helpers; source it from the scripts folder
KP="/c/Users/dalla/AppData/Local/Programs/KiCad/10.0/bin/python.exe"    # KiCad's bundled Python (has pcbnew)
K="/c/Users/dalla/AppData/Local/Programs/KiCad/10.0/bin/kicad-cli.exe"
KD="$(cd .. && pwd)"                                                     # pedals/distortion/kicad
mkdir -p build
