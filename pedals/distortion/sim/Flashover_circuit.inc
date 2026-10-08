* Flashover - shared circuit (included by the Flashover_*.cir run files)
*
* Audio path of the Flashover pedal: the GS1 / Distortion+ gain stage scaled
* for 100k digital pots, switchable Soft/Hard clipping, a DS-1-style tone
* control, an output buffer and a digital-pot Level control.
*
* The run files define these parameters before including this file:
*   dist  - Gain knob, 0 (min) to 1 (max)
*   tone  - Tone knob, 0 (dark) to 1 (bright)
*   level - Level knob, 0 (silent) to 1 (full)
*   hard  - clipping mode, 0 = Soft (germanium), 1 = Hard (silicon)
*   Rt    - Tone digital pot value (10k or 20k under evaluation)
*   Rw    - digital pot wiper resistance (about 75 ohms for MCP41HV parts)
* and the guitar source:  Vguitar IN 0 ...
*
* Digital pots only accept signals between 0V and 9V, so everything after
* the gain stage is centered on VB (4.5V) instead of ground.

* ---- Power ----------------------------------------------------------------
* 9V adapter -> D3 reverse-polarity diode -> +9V rail
V1 VDC 0 9
D3 VDC V9 1N5817
C7 V9 0 100u

* ---- Bias voltage (VB = 4.5V, buffered) -----------------------------------
* R7/R8 divide 9V in half; U3 buffers it so the tone network, clipping
* diodes and Level pot can all return to VB without disturbing it.
R7 V9 VBR 1Meg
R8 VBR 0 1Meg
C6 VBR 0 10u
XU3 VBR VB V9 0 VB level2 Avol=200k GBW=3Meg Slew=13Meg Rail=1.5 Rin=1T Ilimit=25m

* ---- Input (unchanged from GS1) -------------------------------------------
R1 IN 0 1Meg
C1 IN N1 10n
R2 N1 OPIN 10k
C2 OPIN 0 1n
R3 OPIN VB 1Meg

* ---- Gain stage (GS1 values scaled for a 100k digital pot) ----------------
* GS1: R5 1M, R4 4.7k, RV1 1M, C3 47n.  Flashover: R5 100k, RV1 100k digital
* pot, C3 470n (x10 keeps the same bass roll-off). R4 is 390 rather than 470
* because the pot's ~75 ohm wiper resistance adds to it.
* Gain = 1 + R5 / (R4 + RV1): about 216x at max, about 2x at min.
XU1 OPIN NINV V9 0 OPOUT level2 Avol=200k GBW=1Meg Slew=0.5Meg Rail=1.5 Rin=2Meg Ilimit=25m
R5 OPOUT NINV 100k
R4 NINV N3 390
RV1 N3 N4 {100k*(1-dist)+Rw}
C3 N4 0 470n

* ---- Clipping (Soft / Hard) -----------------------------------------------
* Both diode pairs are on the board; a relay connects one pair. The relay is
* modelled as 0.1 ohm (closed) or 1G ohm (open). R10 sets the DC level of
* CLIP to VB.
C4 OPOUT N5 1u
R6 N5 CLIP 10k
R10 CLIP VB 1Meg
C5 CLIP VB 1n
* Soft: germanium 1N34A pair
RKS CLIP CS {if(hard,1G,0.1)}
D1 CS VB 1N34A
D2 VB CS 1N34A
* Hard: silicon 1N4148 pair
RKH CLIP CH {if(hard,0.1,1G)}
D4 CH VB 1N4148
D5 VB CH 1N4148

* ---- Tone (DS-1 style) ----------------------------------------------------
* Low-pass (6.8k + 100n, about 234 Hz) and high-pass (22n + 6.8k, about
* 1.06 kHz) filters, blended by the Tone pot. tone = 0 is the dark end.
RLP CLIP LP 6.8k
CLP LP VB 100n
CHP CLIP HP 22n
RHP HP VB 6.8k
RTA LP TW {Rt*tone+1}
RTB TW HP {Rt*(1-tone)+1}

* ---- Output amplifier and Level -------------------------------------------
* U2 isolates the tone network from the Level pot and adds gain to make up
* for the tone network's loss (about 10 dB), so the pedal can play louder
* than the bypassed guitar. Gain = 1 + RF/RG = 4 (+12 dB).
* RLA/RLB are the 10k Level digital pot; its wiper LW feeds the output.
XU2 TW NFB V9 0 BUFO level2 Avol=200k GBW=3Meg Slew=13Meg Rail=1.5 Rin=1T Ilimit=25m
RF BUFO NFB 30k
RG NFB VB 10k
RLA BUFO LW {10k*(1-level)+1}
RLB LW VB {10k*level+1}
C9 LW OUT 1u
R11 OUT 0 100k
* Guitar amplifier input
Ramp OUT 0 1Meg

* ---- Models -----------------------------------------------------------------
.lib UniversalOpAmp2.lib
* 1N34A germanium diode (common community model, same as GS1.cir)
.model 1N34A D(Is=2.6u Rs=7 N=1.6 Cjo=0.5p M=0.5 Vj=0.1 BV=60 Ibv=15u)
* 1N4148 silicon diode (as in LTspice's standard.dio)
.model 1N4148 D(Is=2.52n Rs=.568 N=1.752 Cjo=4p M=.4 tt=20n Iave=200m Vpk=75 mfg=OnSemi type=silicon)
* 1N5817 Schottky (as in LTspice's standard.dio)
.model 1N5817 D(Is=31.7u Rs=.051 N=1.373 Cjo=190p M=.3 Eg=.69 Xti=2 Iave=1 Vpk=20 mfg=OnSemi type=Schottky)
