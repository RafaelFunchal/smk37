# SLOOP 3 on the M-VAVE SMK-37

SLOOP 3, the groovebox, on the SMK-37's bigger panel: 37 velocity keys, 16 RGB pads, 8 knobs, 4 faders, both wheels and
a sustain pedal, **plus a sampled grand piano** (the PIANO engine, from [Erik37](https://github.com/genkigenki/erik37)):
synths, drums and a real piano on the same keyboard, each track its own.

> **Status: untested on a unit.** This port builds, passes SLOOP 3's tests and runs on the PC twin's simulated SMK-37
> (`make twin-smk`: every scenario, power cuts during saves included), but it has not run on a real SMK-37 yet. Make the
> full flash backup first (below) and expect to tune things on the device: pad sensitivity, encoder direction, the
> CPU load of the piano.

## Models

| Model | Status |
|---|---|
| SMK-37 Pro (and the Donner Starrykey 37 Play) | the target |
| SMK-37 Elite | same hardware; m-bootloader runs on it (as a Pro) |
| MKE-P37 | same hardware as far as known; m-bootloader not reported on it yet |

One build for all: SLOOP 3 installs only through m-bootloader, which writes it the same way on each.

## Install

1. **m-bootloader on the SMK-37, once**, with its full flash backup: put the keyboard in USB download mode, run
   [M-Flasher](https://github.com/jvitkauskas/m-flasher)'s backup (keep the file), then install `smk37-uboot.boot`
   from the [latest m-bootloader release](https://github.com/jvitkauskas/m-bootloader/releases/latest). Its
   calibration guard keeps a copy of the keyboard's factory calibration.
2. **Build** (below) and open `build/fw-smk37/installer-smk37.html` in Chrome or Edge.
3. Switch the SMK-37 on holding **SEQ PLAY**, choose **MIDI MODE** with **<** / **>**, press **SEQ PLAY**, connect USB.
4. The page finds the bootloader, **reads the keyboard's model from its flash first** (the calibration marker
   `trimdat` at `0xC1224`): it refuses an FM-1, and the FM-1's page refuses an SMK-37, with nothing written.
5. Back up from the page (the whole flash, one file), then Install.

**The way back:** restore your backup with M-Flasher (hold **SEQ REC** while switching on for download mode), or run
M-VAVE's own updater (M-UPGRADE, firmware V016), which puts the stock firmware and bootloader back.

## Controls

The SMK-37 is SLOOP 3's FM-1 panel with more: everything in the [guide](docs/GUIDE.md) holds, with the SMK-37's printed
names for the buttons and K5..K8 for KNOB 1..4.

| SMK-37 | SLOOP 3 |
|---|---|
| Keys F3..G5 (the middle 27) | the FM-1's 27 keys, **with velocity** from your unit's factory curves |
| Keys C3..E3 and G#5..C6 (the outer 10) | more notes, through the track's scale, chords and arpeggiator; on the drum track C3 KICK, D3 SNARE, E3 HAT, G#5 BELL, A5 OPEN, B5 CRASH, C6 RIDE |
| 16 pads | the FM-1's 16 white keys (top row 1-8, bottom 9-16): notes, drums, steps (SEQ), patterns (BT), the mix (GLOBE), HOME's keys; with velocity; lit in the track's colour |
| KNOB BANK tapped | DRUM PADS on / off: the pads play the drum kit whatever track the keys play (its light on) |
| KNOB BANK held + K3 | LAYER: a second synth track every key plays too, the same notes and velocity (OFF, or a track) |
| KNOB BANK held + K4 | SPLIT's track: what the keys below the split point play (OFF, a synth track, or the drums) |
| KNOB BANK held + a key | the split point at that key (keys below it go to the SPLIT's track); the same key again: no split |
| FADER BANK | the faders' bank: A level, B filter, C reverb, D delay of synth tracks 1-4; E the drums' level, filter, reverb, delay (its light on off bank A; OCTAVE- held: back) |
| Faders 1-4 | the bank's four; a fader takes over once it reaches the value (no jump) |
| < / > | the previous / next track; with SEQ held or on STEPS: the step page (each acts when let go) |
| < and > together | SMK-37 SETTINGS, again: PANEL TEST, again: back where you were |
| K1 | MASTER (the volume) |
| K2 / K3 / K4 | SELECT / ALGORITHM / PRESETS |
| K5..K8 | KNOB 1..4 |
| PATCH, BT, SCALE, CHORD, NOTE REPEAT, PARA, GLOBE | HOME, SAVE, SEL, ENV, LFO, EDIT, GLO |
| FX, ARP, SEQ, SEQ PLAY, SEQ REC, OCTAVE- / + | FX, ARP, SEQ, PLAY, REC, OCT- / + |
| Pitch wheel | bend on the selected synth track (GLOBE, tapped to its MIDI page: BEND, 1..12 semitones) |
| Mod wheel | vibrato on the selected synth track |
| PEDAL jack | sustain (a footswitch of either polarity: its rest is read at power-on); CC 64 out |
| TRS jack | MIDI OUT (what goes out on USB); no MIDI IN on this keyboard |
| USB | MIDI and audio as on the FM-1, named "SMK-37 SLOOP 3" (the web editor finds it) |

Power-on: **OCTAVE-** held, RESCUE (an empty project; yours is kept); **SEQ PLAY** held, m-bootloader's menu.

## SMK-37 SETTINGS and PANEL TEST

Press **<** and **>** together. Saved with the project.

| Knob | Setting | |
|---|---|---|
| K5 | TOUCH | the keys' and the pads' velocity: DYNAMIC (your unit's factory curves), SOFT, HARD, FIX 64, FIX 100, FIX 127 |
| K6 | PADS | how firm a pad hit must be: LOW, MID (default), HIGH, MAX. Pads that fire twice or by themselves: higher |
| K7 | LIGHT | the pads' brightness: 100 %, 50 %, 25 % |
| K8 | KNOBS | NORMAL, or REVERSE if every knob turns the wrong way on your unit |

**PANEL TEST** (< and > together again) shows every input as the keyboard reads it, and does nothing with the buttons:
the 16 tiles are the pads (their force 0..1023 as a number and a bar, lit while held), the dials the faders (0..127),
the top line the last key or pad with its velocity, the pitch wheel (W), mod wheel (M) and pedal (P) as raw readings.
A button or a knob says its printed name (a knob: which way and how far). The keys still play. < and > together: back.

## Larger text

Hold **PATCH** (HOME), turn **K2** to *SCREEN 1/4*, then turn **K7** (its third knob): **TEXT: LARGE** draws the
screen's small type a step larger (capitals 16 px instead of 13): the knobs' names, the page names, the tiles, the track list, the messages. Kept
with the project. Where a text has no room for it (the stacked lists of values, a line the large type would push its
neighbour off), it stays as it was: nothing is cut or overlaps that was not before. TEXT: NORMAL is the screen as
SLOOP 3 draws it on the FM-1.

## First install: what to check

After installing, in this order (each step is a minute). If something is off, note what PANEL TEST shows.

1. **It starts:** the SLOOP logo, then the tracks. If not: switch on holding OCTAVE- (RESCUE), or hold SEQ PLAY for
   m-bootloader's menu, and tell what the screen says.
2. **PANEL TEST** (< and > together, twice):
   - every button names itself (18), every knob K2..K8 names itself turning right as RIGHT (K1 is the volume);
   - all 37 keys: the top line names each, soft and hard strokes give low and high V;
   - the pads: each tile rises when pressed, the top row is pads 1-8; resting, the numbers stay low and steady;
   - the faders 0 at the bottom, 127 at the top; the wheels and the pedal move their numbers.
3. **Sound:** a track on CONCERT, chords with the sustain pedal; then PLAY the demo.
4. **The load:** hold PATCH: the header shows the audio load (CPU, PEAK, LATE). Play a full pattern with piano chords
   on top: PEAK under ~85 % and LATE not rising is good.
5. **Saving:** BT twice (save), switch off and on: your project comes back.
6. **Lights:** pads in the track's colour; KNOB BANK's, FADER BANK's and SEQ PLAY's lights.

Settings to try if something is off: knobs backwards, KNOBS REVERSE; pads too eager or too stiff, PADS; velocity too
loud or soft, TOUCH. Things worth reporting: a knob or button with the wrong name, a key or pad that never shows, pads
firing at rest, PEAK near 100 %, the screen or the pads' colours wrong.

## Layers and splits

SLOOP 3's five tracks always play together from the sequencer. On the keys, KNOB BANK sets up two more ways to play
them live (the screen says what is set each time):

- **Layer:** a piano on track 1 and a pad on track 2, both from one key. Hold KNOB BANK, turn **K3** to the pad's track.
- **Split:** a bass (or the drums) under your left hand. Hold KNOB BANK, press the key where the split goes (the keys below
  it), turn **K4** to that part's track. The split part plays with its own track's scale, chords and arpeggiator.
- Together: split bass on the left, piano and pad layered on the right; the drums on the pads (KNOB BANK tapped), and
  the sequencer playing the rest.

The sustain pedal holds both parts of a layer. REC records the selected track (the split's and the layer's notes are
heard, not recorded). The layer and the split are saved with the project (and come back when it opens).

## The piano

Six factory sounds among the KEYS: CONCERT, STUDIO GRND, VINTAGE UP, JAZZ GRAND, SOFT PIANO, BRIGHT PNO. Or EDIT >
engine PIANO on any synth track:

| | TONE page | | DAMP page |
|---|---|---|---|
| K5 | TONE: CONCERT, STUDIO, VINTAGE, JAZZ | K5 | DAMPR: how long a released note rings |
| K6 | BRILL (CC 74) | K6 | NOISE: the dampers' release noise |
| K7 | TOUCH: SOFT, NORMAL, HARD, FIXED | K7 | WIDTH: the keys across the stereo |
| K8 | DECAY: how long a held note rings | K8 | UNA C: the soft pedal's depth |

The Salamander Grand Piano V3 (Alexander Holm, CC BY 3.0) as Erik37 plays it: its velocity layers crossfaded, each
key's own decay, the brightness that follows the touch, the dampers. To fit the SMK-37's flash next to SLOOP 3, it
keeps 2 of Erik37's 3 velocity layers and plays its lowest keys (A0..C#1) from the next sample down
([engine/PIANO.md](engine/PIANO.md)). Room: SLOOP 3's reverb (the sound's REV send).

## What differs from SLOOP 3 on the FM-1

- **24 voices** (the FM-1: 32), and eleven engines built for size: the room in RAM for the SMK-37's panel and the piano.
- **One sample slot (USR1)**; USR2-4 are refused by the editor. The flash holds the piano instead.
- **SLOOP 2.8's data is not imported** (it is the FM-1's; SLOOP Pro's projects are another format).
- **The line along the bottom of the screen is the battery.** Connected to a computer over USB (powered and charging), a cyan light runs along it **at the tempo**: a step each eighth note, one sweep a bar. While the sequencer plays (or counts in), each sweep starts on a bar's first beat; stopped, it follows the tempo you set. (The FM-1 runs it at a fixed speed.)
- No TRS MIDI IN (the SMK-37 has none), no Bluetooth (its radio needs M-VAVE's closed SDK).
- RAM and flash are nearly full: 1 KB of RAM and ~15 KB of flash room left for the SMK-37's code.

## The flash

| Range | |
|---|---|
| `0x04000-0x7BFFF` | SLOOP 3 and its sample bank (at `0x4C000`, 16 KB later than on the FM-1), enciphered (an install writes it) |
| `0x7C000-0xB9FFF` | the piano bank, in clear (an install writes it) |
| `0xC0000-0xC1FFF` | Bluetooth pairing and **the factory calibration**: never written (m-bootloader and SLOOP 3 both refuse) |
| `0xC2000-0xD5FFF` | your sample slot USR1 |
| `0xD6000-0xF3FFF` | your FM6 bank, SYN kits, project, sounds: out of an install's reach, kept by updates |
| `0xF4000-0xFFFFF` | M-VAVE's resources, the unit's keys: never written |

The firmware may erase or program only `0xC2000-0xF3FFF` (`hal/smk37/smk37.h`, checked in `hal/fm1/flash.c`).

## Build and test

Linux (or a Linux container) with the [pi32v2 LLVM toolchain](https://github.com/jvitkauskas/pi32v2-llvm), Python 3 with
NumPy and Pillow, Node.js:

```sh
BOARD=smk37 fw/build.sh /path/to/toolchain/bin   # build/fw-smk37: fw.area, piano.img, installer-smk37.html
fw/build.sh /path/to/toolchain/bin               # the FM-1, byte for byte as before (build/fw)
make test                                        # SLOOP 3's tests
make twin-smk                                    # the SMK-37 on the PC twin: its scenarios and the FM-1's
```

On macOS (or anywhere with Docker), `tools/dev/dev.sh` runs these in a ready Linux container: see
[tools/dev/README.md](tools/dev/README.md). `tests/fm1-reference.sha256` holds the FM-1 build's hashes it must keep.

`BOARD=smk37` also checks the installer page against a simulated m-bootloader on an SMK-37 and on an FM-1
(`tests/install_sim.js ... smk37 ... [--wrong-unit]`).

## Credits

SLOOP 3 by isod89 (GPL-3.0). The SMK-37's hardware facts: M. Geisler's
[fm1-to-smk37](https://github.com/genkigenki/fm1-to-smk37) and SLOOP Pro (isod89/sloop-pro), whose control logic
(velocity, pads, faders, wheels, pedal) `hal/smk37/smk_ctl.h` follows; confirmed against M-VAVE's open
[smk37_open](https://github.com/m-vave/smk37_open) board layer. The piano: Erik37 by M. Geisler (MPL-2.0), the
Salamander Grand Piano V3 by Alexander Holm (CC BY 3.0). m-bootloader by Julius Vitkauskas (MIT).
