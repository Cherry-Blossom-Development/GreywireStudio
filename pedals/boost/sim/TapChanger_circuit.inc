* Tap Changer - shared circuit (included by the TapChanger_*.cir run files)
*
* Audio path of the Tap Changer clean boost: one non-inverting op-amp stage
* with a digital pot in its feedback, so the Boost knob changes the gain and
* the stage never has to clip before a volume control.
*
* The run files define these parameters before including this file:
*   boost - Boost knob as a fraction of the pot's travel, 0 (min) to 1 (max)
*   Rp    - Boost digital pot value
*   Rw    - digital pot wiper resistance (MCP45HV51: 75 ohms typical at 36V,
*           145 typical / 200 max at 10V; about 150 assumed at 9V)
*   Vsup  - supply at the input of D3 (9V adapter, or about 17.5V from a
*           charge pump, which is not modelled)
*   Rail  - how close the op-amp output gets to its supply rails, volts
*   En    - op-amp input voltage noise, V/rtHz
*   Gbw, Slw - op-amp gain-bandwidth (Hz) and slew rate (V/s)
* and the guitar source:  Vguitar SRC 0 ...  with a series Rsrc (pickup).
*
* Digital pots only accept signals inside their supply, so the audio is
* centered on VB (half the supply), as in Flashover.

* ---- Power ----------------------------------------------------------------
V1 VDC 0 {Vsup}
D3 VDC V9 1N5817
C7 V9 0 100u

* ---- Bias voltage (VB, buffered by the other half of the dual op-amp) ------
R7 V9 VBR 1Meg
R8 VBR 0 1Meg
C6 VBR 0 10u
XU1B VBR VB V9 0 VB level2 Avol=1Meg GBW={Gbw} Slew={Slw} Rail={Rail} Rin=1T Ilimit=25m

* ---- Input ----------------------------------------------------------------
* R1 stops a pop at the input. R1 and R3 together load the guitar with about
* 1.1M. R2 and C2 keep radio signals out without touching the audio band.
Rs SRC IN {Rsrc}
R1 IN 0 2.2Meg
C1 IN N1 100n
R2 N1 OPIN 1k
C2 OPIN 0 100p
R3 OPIN VB 2.2Meg

* ---- Gain stage -----------------------------------------------------------
* Gain = 1 + (RV1 + Rw) / R4: about 10.2x (+20 dB) at max and about 1.14x
* (+1 dB) at min, where only the wiper resistance is left. C3 sets the bass
* roll-off with R4 (about 14 Hz) and keeps the DC gain at 1.
XU1A OPIN NFB V9 0 OPOUT level2 Avol=1Meg GBW={Gbw} Slew={Slw} Rail={Rail} Rin=1T Ilimit=25m en={En}
RV1 OPOUT NFB {Rp*boost+Rw}
R4 NFB N3 1.1k
C3 N3 VB 10u

* ---- Output ---------------------------------------------------------------
* R12 keeps a long cable's capacitance from upsetting the op-amp.
R12 OPOUT N5 100
C9 N5 OUT 1u
R11 OUT 0 100k
* Guitar amplifier input
Ramp OUT 0 1Meg

* ---- Models -----------------------------------------------------------------
.lib UniversalOpAmp2.lib
* 1N5817 Schottky (as in LTspice's standard.dio)
.model 1N5817 D(Is=31.7u Rs=.051 N=1.373 Cjo=190p M=.3 Eg=.69 Xti=2 Iave=1 Vpk=20 mfg=OnSemi type=Schottky)
