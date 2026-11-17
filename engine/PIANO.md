# PIANO

A sampled grand piano engine for SLOOP 3, ported from **Erik37**'s piano engine (`app/dsp/erk.c`, by M. Geisler).
`engine/piano.c` (MPL-2.0) is Erik37's voice made into a SLOOP `gb_engine`. It is float, renders 64-frame blocks
and uses SLOOP's voice allocation. The bank is Erik37's ERK1 format (`engine/erk_bank.h`, `engine/erk_codec.h`, copied
verbatim), cut down by `tools/mkpiano.py` and read in place from the flash window, as SAMPLE's bank is.

## What a voice does

- The key's zone and the velocity's layers come from the bank's key and velocity tables. Two layers are crossfaded
  through the attack. From about 0.22 s the voice hands over, in about 90 ms, to the dominant layer alone.
- Block ADPCM (32 samples in 17 bytes) is decoded in place and read through a 4-point cubic Hermite interpolator. The
  pitch follows the voice's pitch every block (bend, glide, LFO), relative to its key.
- A one-pole brightness filter is set by velocity, TONE, BRILL and UNA CORDA. After about 0.25 s it darkens towards
  the key's floor.
- The envelope is the key's two-slope decay (m1, then m2 after the knee). Release uses the two-stage damper
  (mr1 for about 60 ms, then mr2). The top keys (ERK_NODAMP, from F#6) have no damper and ring on.
- At release, the bank's damper noise is mixed into the same voice. It is louder for loud, still-ringing notes (the
  floor is the header's `rel_gain`).
- Pan is constant power by key across WIDTH. It is applied to the voice's `pan_l` / `pan_r` at its start.
- The engine has its own envelopes (`ownenv = 1`): render returns 0 when the note and its noise are over.

## E0..E7

| | Name | Range (default) | What it does |
|---|---|---|---|
| E0 | TONE | CONCERT STUDIO VINTAGE JAZZ (CONCERT) | Erik37's voicings: filter brightness, low and high shelf, and a small detune of the unisons by key (VINTAGE, JAZZ) |
| E1 | BRILL | 0..100 (50) | Brilliance. The filter is scaled by 2^(0.7 b) and the high shelf by ±6 dB (b = −1..+1) |
| E2 | TOUCH | SOFT NORMAL HARD FIXED (NORMAL) | Erik37's touch curves (exponents 0.55, 0.95, 1.7; FIXED plays every note at velocity 100) |
| E3 | DECAY | 0..127 (64) | Held decay length × 0.25..4 (both slopes and the knee) |
| E4 | DAMPER | 0..127 (64) | Release length × 0.25..4 (both damper stages) |
| E5 | NOISE | 0..100 (40) | Release noise level (40 is Erik37's default; 100 is its 1.1 level) |
| E6 | WIDTH | 0..100 (80) | Spread of the keys across the stereo field |
| E7 | UNA CORDA | 0..100 (0) | Soft pedal held at this depth: a softer hammer, a quieter and darker note |

TONE, BRILL, TOUCH, DECAY, WIDTH and UNA CORDA are read when a note starts. NOISE and DAMPER are read at its release.

## Trimmed from Erik37, and why

- **The bank must fit SLOOP's flash.** Erik37's bank is 392 KB. The trimmed image is 249,872 bytes: at
  0x78000 it ends at 0xB5010, under the 0xC0000 limit with 45,040 bytes free.
  - **Zone 0 (A0..C#1) is dropped.** Its five keys play zone 1's sample, transposed down by up to 5 semitones, with
    their rate recomputed exactly from the lowest key of zone 1.
  - **Two of the three velocity layers are kept:** soft (0) and loud (2). The velocity table is rewritten so the
    position between layers stays continuous: old p = l0 + w·(l1 − l0), 0..2, becomes l0 = 0, l1 = 1, w = p/2.
- **Erik37's bus is not ported:** sympathetic resonance, the FDN reverb, the pad layer and the metronome. SLOOP has its own
  effects.
- **The voice allocator, re-strike fade and voice shedding are not ported.** SLOOP's voice layer does these.
- **The half pedal and sostenuto are not ported.** SLOOP's MIDI layer holds note-offs while the sustain pedal is down.
- **Erik37's cap of 6 notes crossfading two layers at once is not ported.** Here every note whose velocity falls
  between the layers decodes two streams for its first ~0.3 s.
- **The tone shelves run per voice.** Erik37 runs them on its bus; the sound is the same.

## Credits

- **Erik37**, Copyright (C) 2026 M. Geisler (genkigenki/makkuro), MPL-2.0. `engine/piano.c`, `engine/piano.h`,
  `engine/erk_bank.h`, `engine/erk_codec.h` and `tools/mkpiano.py` are derived from it and stay MPL-2.0.
- **Piano samples:** "Salamander Grand Piano V3" by Alexander Holm, CC BY 3.0
  (https://creativecommons.org/licenses/by/3.0/). They were resampled, trimmed, looped and compressed for Erik37, and
  cut down further for SLOOP 3: zone 0 and the middle velocity layer are removed. The flash image is a derivative of the
  recording and stays under CC BY 3.0. Keep this attribution wherever the firmware or the image is passed on.
