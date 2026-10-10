"""Where things live, worked out from this file's location so the scripts run from any checkout."""
import os

HERE = os.path.dirname(os.path.abspath(__file__)).replace("\\", "/") + "/"
KI = os.path.normpath(HERE + "..").replace("\\", "/") + "/"                    # pedals/distortion/kicad/
BUILD = HERE + "build/"                                                         # netlists, DSN/SES, logs (not committed)
LIB = os.path.normpath(HERE + "../../../../libraries").replace("\\", "/") + "/"
os.makedirs(BUILD, exist_ok=True)
