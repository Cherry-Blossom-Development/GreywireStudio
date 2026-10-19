# Intellectual property: initial research

Oct 18, 2026

This is a first look at patents, trademarks and licenses that could affect the Greywire products: Flashover, Switchyard, Switchyard Audio, the Linework system and the phone app. The aim is to avoid infringing anyone else's rights, not to file our own patents.

**This is not legal advice or a clearance.** It's a starting list for a patent and trademark attorney. A proper "freedom to operate" review (see the Glossary) reads every claim of every relevant patent against the final design, and it's the only thing that gives real protection. Patents are judged on their exact claim wording, which these notes only summarize.

## Summary

| Area | Finding | Action |
| --- | --- | --- |
| Core ideas (MIDI loop, presets per song part, footswitch stepping, analog pedals with digital pots, relay bypass, dry analog path with digital echoes) | No active patent found that clearly covers them as we've designed them | Ask the attorney to confirm, using the watch list below |
| Selling or adding effects through the app | One active patent (Cort, until 2032) covers timed "try before you buy" previews and an authentication cable | **Don't use time-limited trials or an authentication cable** |
| Recording to the cloud through the pedal | One active Fender patent (until 2032) is close to the General Concept's "record and upload" idea | **Get legal review before building recording** |
| Preset changes with delay or reverb tails | One active inMusic patent (until 2038) covers running old and new presets on two audio threads at once | **Use a different method if we add tails** |
| Names | No live US trademark for Greywire, Flashover, Switchyard or Linework in music gear, nor for the next four pedals (Tap Changer, Three Phase, Rolling Blackout, Double Circuit) | File our own trademarks before launch; don't name the app "Switchyard"; don't use "Brownout" (a live pedal mark) |
| Licenses for code we use | All permissive (MIT, BSD) | Keep DaisySP-LGPL out; include the license notices with the product |

## Patents

### How the search was done

- Google Patents, US patents and applications with a priority date from Jan 1, 2006 on. US patents last 20 years from filing, so anything filed earlier has expired.
- Keyword searches for each product idea, plus searches by pedal and music-gear companies (Source Audio, Strymon, Line 6, Chase Bliss, Positive Grid, Eventide, Electro-Harmonix, Empress, Meris, Roland/Boss, Fender, Harman/DigiTech, Fishman, Intelliterran/Singular Sound, Fractal, Kemper, Neural DSP, Universal Audio, Yamaha, inMusic).
- Status and expiry dates come from Google Patents' legal-status data, which can lag behind the USPTO.

Limits: keyword search misses patents that use unexpected wording, foreign patents weren't checked, and applications filed in the last 18 months aren't published yet.

### Active patents to watch

| Patent | Owner | Expires | What its main claims require | Our position |
| --- | --- | --- | --- | --- |
| [US8989408B2](https://patents.google.com/patent/US8989408B2/en) Downloading effects to an effects unit | Cor-Tek (Cort), originally Harman/DigiTech | 2032-09-05 | Claim 1: a user device loads effects into an effects unit through a cable with **a built-in authentication chip**. Claims 8 and 13: a library of effects that can be **previewed on the unit for a limited time**, then disabled until bought | Loading effects as firmware updates over a plain USB cable, with no timed trials, appears to avoid these claims. Keep it that way |
| [US9373313B2](https://patents.google.com/patent/US9373313B2/en) Storing and accessing a musical performance on a remote server | Fender | 2032-10-04 | Claim 1: a controller connected to an instrument's communication link that receives control data **and** sends the instrument's real-time audio through that link **as a cloud storage recording** | Not in any current design. The General Concept's "pedal uploads recordings to the network" idea comes close. **Review before building recording** |
| [US10403252B2](https://patents.google.com/patent/US10403252B2/en) Connecting and controlling musical instruments over a network | Fender | 2035-06-18 | Claim 19: a device with a communication link, a controller that receives control data **and passes audio** over it, **an access point**, and an interface that detects the device and shows its status | The Linework loop carries no audio and has no access point. Recheck if a later version streams audio or uses Wi-Fi |
| [US20210043176A1](https://patents.google.com/patent/US20210043176A1/en) Gapless preset switching (granted) | inMusic (HeadRush) | 2038-01-19 | A multi-effects unit that, on a footswitch preset change, processes the sound with the old preset and the new preset **on two audio-engine threads at the same time** | Switchyard Audio's stage 1 firmware has one engine and no tails. If we add delay or reverb "spillover" across preset changes, do it within one engine (keep the old echoes in the delay memory) and have the attorney check |
| [US20200365127A1](https://patents.google.com/patent/US20200365127A1/en) Touch screen interface for a multi-effects unit (granted) | inMusic | 2038-01-19 | A multi-effects unit with switches, **its own electronic display** and stored presets | Switchyard has no screen; the phone is the display. Low concern |
| [US10127899B2](https://patents.google.com/patent/US10127899B2/en) Controlling multiple effects modules and pedals on a common platform | John A. Perez | 2036-04-05 | A system with **a touch screen, a backplane, and analog effects modules plugged into that backplane**, at least one with a digitally controlled pot or switch, plus external pedals with wired plugs | Linework pedals are separate boxes on a MIDI cable, not modules in a backplane, and Switchyard has no built-in touch screen. Low concern, but it's the closest match to "digital pots in analog pedals under central control", so include it in the review |
| [US8957297B2](https://patents.google.com/patent/US8957297B2/en) Programmable pedalboard | Cor-Tek (Cort) | 2033-06-12 | A docking station for **a portable computer that produces the effects**, with footswitches for effects and for tone banks | Switchyard Audio makes its effects itself, so this doesn't fit. Keep the phone as a controller only |
| [US11290797B2](https://patents.google.com/patent/US11290797B2/en) Intelligent cable adapter | Prophet Productions | 2039-07-31 | A plug-in **cable adapter** with its own converter and processor | Not our product shape. No concern |

### Expired or abandoned (no longer a risk)

These cover nearby ideas, but they can no longer be enforced. Published patents that have lapsed also count as prior art, which helps if anyone later claims these ideas.

| Patent | Subject | Status |
| --- | --- | --- |
| [US20160019877A1](https://patents.google.com/patent/US20160019877A1/en) (Source Audio) | Networking effects processors with two-way communication and stored settings | Abandoned |
| [US9728172B1](https://patents.google.com/patent/US9728172B1/en) (Perez) | Earlier version of the Perez platform | Expired, fees not paid |
| [US9595248B1](https://patents.google.com/patent/US9595248B1/en) | Remotely operated bypass loops | Expired, fees not paid |
| [US8609973B2](https://patents.google.com/patent/US8609973B2/en) (CleanStage) | Audio effects controller for musicians | Expired, fees not paid |
| [US9226064B2](https://patents.google.com/patent/US9226064B2/en) (FXConnectX) | Wireless switching of effects pedals with status updates | Expired, fees not paid |
| [US9035165B2](https://patents.google.com/patent/US9035165B2/en) | Control pedal for an electronic device | Expired, fees not paid |
| [US20150332660A1](https://patents.google.com/patent/US20150332660A1/en) (Fender) | Instrument controlling accessories | Abandoned |
| [US20200202828A1](https://patents.google.com/patent/US20200202828A1/en) | Effects loop sequencer | Abandoned |
| [US20140205113A1](https://patents.google.com/patent/US20140205113A1/en) (Stomp Labs) | Expression-controlled audio platform | Abandoned |

### Other things noticed

- **Polyphonic tuners.** Empower Tribe (Behringer) sued Boss over a polyphonic tuner mode in 2025; Boss removed the feature from four products in March 2026 while the case went on. If we ever add a tuner, keep it an ordinary single-string tuner unless cleared.
- **Classic distortion circuits.** Flashover's op-amp and diode clipping design follows circuits that have been around since the 1970s; any patents on them expired long ago. Circuits themselves aren't protected by copyright, but someone else's circuit board layout, firmware, product names and look (trade dress) are. Our boards and code are our own.

## Standards and licenses

| Item | What applies |
| --- | --- |
| MIDI and MIDI over 3.5 mm TRS (Type A) | Published by the MIDI Association and free to use. The MIDI logo has its own usage rules; check them before printing it on a product |
| USB MIDI (class compliant) | Free. A USB vendor and product ID is needed: free product IDs are available from Raspberry Pi for RP2040 designs. The USB-C logo needs a USB-IF license; leave it off unless certified |
| Apple MFi | Not needed, because the pedals use standard USB classes |
| libDaisy, DaisySP (Switchyard Audio firmware) | MIT license: free for commercial use, keep the copyright notice in the product documentation |
| DaisySP-LGPL | **Not used.** LGPL would put extra conditions on the firmware we ship |
| Pico SDK and TinyUSB (RP2040 firmware) | BSD-3 and MIT: free for commercial use with the notice |
| Daisy Seed hardware design | Its schematics are MIT licensed, so the production board can reuse the design with the notice |

Plan to ship a short "open source notices" page in the manual and in the app.

## Trademarks

Searched in the USPTO trademark database on Oct 18, 2026, including spacing and spelling variants (Grey Wire, Graywire, Flash Over, Switch Yard, Line Work, Lineworks).

| Name | What exists | Concern for us |
| --- | --- | --- |
| **Greywire** | GREYWIRE for clothing (cancelled 2021) and handbags (abandoned), GREYWIRE RECORDS for audio recording (cancelled), GREYWIRETECH for electronics parts (abandoned). Nothing live | Low. Free to register |
| **Flashover** | Only dead marks: beer, soft drinks, supplements, paintball goods; "Flash Over USB" (Mitsubishi software, cancelled) | Low |
| **Switchyard** | Live: Freeman & Forrest, influencer-marketing **mobile app** and software (class 9, allowed 2025); University of Tulsa magazine (class 9 and 16); a children's play centre | Low for a pedal. **Don't use "Switchyard" as the app's name** in the app stores, where it would sit next to the marketing app |
| **Linework** | Live: LINEWORK AS (Norway), tattoo-studio booking software (classes 9, 42, 44); utility-tool companies | Low for a pedal series name |

A web search also found no guitar product sold under any of these names. Unregistered use still counts in the US, so the attorney's search should cover that too.

### Names for the next four pedals (Oct 19, 2026)

Candidates for a clean boost, phaser, tremolo and chorus were checked the same way: the USPTO database for live marks (the name as written), plus web searches for pedals already sold under each name. Chosen names:

| Pedal | Name | What exists |
| --- | --- | --- |
| Clean boost | **Tap Changer** | No US marks at all; no pedal found |
| Phaser | **Three Phase** | Two live class 9 marks for unrelated goods: lineman training equipment (THREE PHASE INNOVATIONS) and helmet face shields (3PHASE); no pedal found |
| Tremolo | **Rolling Blackout** | No live marks; no pedal found |
| Chorus | **Double Circuit** | No US marks at all; no pedal found. Mr. Black sells a "DoubleChorus", which is similar wording but a different name |

Backups that were also clear: Hot Stick (boost), Rotating Field and Phase Lag (phaser), Load Shed and Interrupter (tremolo), Crosstalk (chorus; Native Instruments sells a "Crosstalk Guitars" sample library).

Rejected:

| Name | Reason |
| --- | --- |
| **Brownout** | **Live mark for electronic effects pedals** (Jonathan Greenlee, classes 9 and 15). Don't use it for any pedal |
| Step Up | Live class 9 mark covering loudspeakers (UNO S.L.); generic |
| High Tension | Blackstar's HT-Boost is named after "high tension" voltage |
| Feeder | Many live marks; also a well-known band |
| Polyphase | Live electronics mark (Linear Technology, voltage regulator circuits) |
| Flicker | Lastgasp Art Laboratories sells the FLIKRRR flickering tremolo |
| Chopper | Diamond's tremolo calls its choppy mode the CHOPPER |
| Corona | TC Electronic Corona Chorus |
| Parallel Lines | A Blondie album |

Recommended:

1. File US trademark applications on an "intent to use" basis before launch, for **Greywire** in class 9 (electronic effects pedals, downloadable software) and class 15 (musical instrument accessories), and for each product name once final.
2. Name the phone app after the brand (for example "Greywire") rather than Switchyard.
3. Pick the brand name first (see the naming notes), since the domain and app names follow from it.

## Next steps

1. Take this document to a patent attorney for a freedom-to-operate review of the final Switchyard and Switchyard Audio designs. Flat-fee reviews of a handful of patents usually cost a few thousand dollars.
2. Have the trademark applications filed at the same time.
3. Before building **recording or cloud upload**, **paid effect downloads**, **delay or reverb tails across presets** or a **tuner**, recheck against the patents above.
4. Optional: publish our own design documents (for example the Linework Control Specification) once the product launches. A dated public description makes it hard for anyone to patent the same ideas later, at no cost.

## Glossary

**Claims:** the numbered sentences at the end of a patent that define exactly what it covers. A product infringes only if it has every element of at least one claim. Descriptions and drawings elsewhere in the patent don't count on their own.

**Freedom to operate (FTO):** a review by a patent attorney of whether a specific product can be made and sold without infringing active patents. It ends in a written opinion, which also protects against claims of deliberate infringement.

**Expired, fees not paid:** US patent owners must pay maintenance fees 3½, 7½ and 11½ years after the grant. If they don't, the patent ends early and anyone can use what it describes.

**Abandoned application:** a patent application the applicant gave up on before it was granted. It never became enforceable.

**Prior art:** anything made public before a patent was filed. It can make a later patent invalid, so published but expired or abandoned patents protect others who use the same ideas.

**Intent to use:** a US trademark application filed before the product is on sale. It reserves the name from the filing date, as long as the product launches within the allowed time.

**Trademark classes:** trademarks are registered for particular kinds of goods and services, numbered 1 to 45. Class 9 covers electronics and software, class 15 musical instruments and their accessories. The same name can belong to different owners in unrelated classes.
