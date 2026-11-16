# SLOOP 3: the guide

> **SLOOP 3 is a public beta.** It was rewritten from scratch: a new sound engine, a new sequencer and a new
> interface. It is being tested on real FM-1s now. Expect bugs, keep backups, and please
> [report what you find](https://github.com/isod89/sloop3-fm1/issues). Prefer the tried and tested version?
> SLOOP 2.8 stays at [github.com/isod89/sloop-fm1](https://github.com/isod89/sloop-fm1).

SLOOP 3 plays like SLOOP 2.8.5: the same buttons, the same layers, the same sounds. This guide says how to install
it, what it takes over from SLOOP 2.8, and what is new or has moved. The screen is in English.

## Installing

Open **[isod89.github.io/sloop3-fm1](https://isod89.github.io/sloop3-fm1/)** in Chrome or Edge (or the
`installer-sloop3-*.html` of a [release](https://github.com/isod89/sloop3-fm1/releases), opened from your disk). It
is a guided page, in English or French, that takes you through five steps, one at a time:

1. **Before you start.** What a beta means, and what you need: the FM-1, a USB data cable, Chrome or Edge.
2. **The safety net: m-bootloader, once.** SLOOP 3 is installed only through
   [m-bootloader](https://github.com/jvitkauskas/m-bootloader), the open-source bootloader for the FM-1 by Julius
   Vitkauskas, for your safety: it checks the firmware before starting it, and if an install is interrupted or a
   firmware is damaged, the FM-1 opens a recovery menu instead of hanging. To see whether it is there: switch the
   FM-1 on holding **PLAY**; a menu with BOOT APP, DOWNLOAD MODE and MIDI MODE means it is. If not, the recommended
   way is [M-Flasher](https://github.com/jvitkauskas/m-flasher) (Windows, macOS, Linux):
   - connect the FM-1 and put it in USB download mode (M-Flasher's DOWNLOAD MODE button; with SLOOP 2.8 running,
     OCT- and OCT+ held 5 seconds; with a SLOOP 3 beta, OCT- held at power-on, then REC);
   - **back up** the whole flash;
   - install `fm1-uboot.boot` from the
     [latest m-bootloader release](https://github.com/jvitkauskas/m-bootloader/releases/latest).

   Only the boot area is written. An FM-1 still on M-VAVE's firmware can also take `fm1-uboot.fwsc` in M-VAVE's
   M-UPGRADE app.
3. **Connect.** Switch the FM-1 off, then on holding **PLAY**: m-bootloader's menu. Choose **MIDI MODE**, connect
   USB and click **Connect**. The page finds the FM-1 by itself as soon as it appears, and shows its m-bootloader
   version and chip key.
4. **Back up.** The whole flash, 1 MB, as a file in your downloads (`fm1-backup-….bin`). Nothing is written. Already
   have a full backup from M-Flasher? Tick the box and go on.
5. **Install SLOOP 3.** About a minute: the FM-1 checks the whole package, then writes it. It restarts on SLOOP 3.
   Turn MASTER up gently.

Nothing is erased until the whole package has been read and checked. If an install stops while writing, the FM-1
opens m-bootloader's menu by itself at the next start: install again.

## The first start: what comes over from SLOOP 2.8

SLOOP 3 never writes SLOOP 2.8's area: it reads it. At the first start:

- your **sections A–D** become **your project**, patterns 1–4: notes, ties, nudges, conditions, ratchets, **locks**
  (of the sound and of the mix), each section's sound (the pattern keeps its own), the arpeggiators, the kit and the
  TUNE / DECAY of each drum sound, the mix, the delay, the reverb, the chorus, DUST / DUCK / FILT, TEXTURE, and
  **your song** (tap SAVE twice to see it);
- your **32 user presets** become USER SOUNDS (EDIT 5/6), your **favourites** the FAVS category of SOUNDS (EDIT 6/6);
- your **samples USR1–4**, your **SYN1–4 kits**, your **FM6 bank**: read where they are;
- your settings: menu, lights, calibration, MIDI LEARN, MIDI settings.

**Updating from SLOOP 3.0–3.3.4: one project now.** From 3.3.5 the FM-1 keeps one project, guaranteed to fit at
its fullest (5 tracks × 8 patterns of 128 steps, each with its own sound, its notes and locks, the song). At the first
start of 3.3.5, the newest of your 8 places becomes that project (ONE PROJECT NOW: YOUR PROJECT n); the others are
not read again. **Before installing, back up everything with the editor (Projects, BACK UP EVERYTHING)**: your other
projects stay in that file, and the editor puts any of them back on the FM-1 (RESTORE, or ⇧ file).

**Going back to SLOOP 2.8.5:** restore the backup you made before installing SLOOP 3, with M-Flasher. Everything of
SLOOP 2.8 is still there.

## What is new or has moved

**Messages come in two kinds:**

- **What just changed** (a mode, a value, a sound's name, a hint: KEY E MIN, PATTERN 3 ON THE BAR, TAKE 3.4s…) is
  written in the page's title line, underlined in yellow, for 1.5 s. Nothing is hidden: on TRACKS the tempo stays; on
  a layer, the title stays.
- **What matters** (a save you asked for, a confirmation such as AGAIN TO ERASE, a project or a bank received from
  the editor) shows in the window at the top, green frame; **an error**, red frame (SAVE FAILED,
  CPU OVERLOAD: VOICES CUT, CORE 2: FAILED…).

The autosave opens no window: PROJECT SAVED (AUTO) is written in the title line.

### SOUNDS: choosing a sound

Every sound sorted by category, 16 a page, **each sound on a white key** (the 16 tiles are the 16 white keys, in the
same order). The track's sound is in white; a yellow dot marks a favourite.

The categories: **FAVS** (your 16 favourites), BASS, KEYS, ORGAN, PAD, LEAD, PLUCK, STAB, FX, **YOURS** (your 32
places), **DX7** (the voices of your FM6 bank, by name; only shown when you have a bank). On the drum track: KIT.

Two ways there:

- **Turn PRESETS**: the sound changes as before, and SOUNDS shows where you are. The keys play as usual. The page
  closes by itself 4 seconds after the last turn. **EDIT** tapped while it shows: it stays, and the keys choose.
- **Tap EDIT up to SOUNDS (6/6)**: there, **a white key takes its tile's sound and plays it**. The white keys that
  hold a sound glow faintly, the one with the track's sound is lit. The black keys play the track's sound without
  changing anything.

| On SOUNDS | |
|---|---|
| KNOB 1 (CAT) | the category |
| KNOB 2 (PAGE) or SELECT | the page; SELECT goes on into the next category |
| PRESETS | the next or previous sound |
| KNOB 4 (FAV) | right: the track's sound becomes a favourite (the first free place); left: it no longer is |
| KNOB 3 (MOVE) | on FAVS: moves the favourite one place |

**A favourite at once:** EDIT up to SOUNDS, KNOB 1 on FAVS (the first category), then its white key. Favourites are
written to the flash a second after the last change (FAVOURITES KEPT).

Elsewhere (the steps page, NOTES, a layer held, while recording), PRESETS changes the sound without opening SOUNDS
and says its name in a message, as before.

### DX7 voices (the FM6 bank)

In the web editor, **FM6 bank** tab: **Load DX7 voices (.syx)**. If the bank is empty, the file's voices go into
B1–B27 by themselves (a DX7 file holds 32, the bank 27). Then **Save the bank to the FM-1**. On the FM-1 they are in
SOUNDS, category **DX7**, by name, and become favourites like the other sounds.

**On the FM-1's speaker: HOME held + SELECT, AUDIO page, SPEAKER ON.** The small speaker can't play the lows (the body
of an 808 kick is around 50 Hz): without this setting the drums sound very weak on it. SPEAKER cuts what it can't play
and brings the drums' lows back as harmonics it can play, about 9 dB more drums on an 808 kit. On headphones or
speakers, leave it OFF. It is kept with the settings; it is SLOOP 2.8's SPEAKER LOWCUT, taken over at the first start.

| Where | What |
|---|---|
| ALGORITHM / PRESETS / SELECT | the track / its sound: **PRESETS opens SOUNDS** (above), the next or previous sound, category after category / the tempo, or the next page of a group |
| 5 tracks | the synths on 1–4 (the 4th, pink, is one more synth), the drums on 5; GLO held: 1–5 mute, 6–10 solo; in MIDI, the synths on channels 1–4, the drums on 10 |
| SEQ tapped | the steps (drums: the DRUMS grid); **tapped twice** (synth): **NOTES**, the full-screen piano roll |
| NOTES | the keys write the step; KNOB STEP NOTE LEN FORCE; **PRESETS** the zoom (8 16 32 64 steps); **OCT+ held** the note's sheet: SLIDE NUDGE FILL MOVE COPY DELETE (KNOB 1 the line, KNOB 2 the value, OCT+ tapped does it, OCT- closes) |
| SEQ held + a step | NOTE VEL RATCH NUDGE; PRESETS a lock, ALGORITHM which one (the sound, LEVEL, PAN, CHO, DLY, REV, FILT, INSERT, SLICER) |
| REC | **tapped**: REC on or off (when let go); **held 2 s**: the track erased, and nothing else (from 0.5 s: HOLD REC: ERASE THE TRACK; let go before 2 s: ERASE CANCELLED); EDIT + OCT- brings it back |
| SAVE tapped | SONG (the patterns on the keys); **SELECT**: SONG 2/2, the song line by line (LINE, PAT, BARS, LINES); **on SONG or SONG 2/2, SAVE tapped saves** (the song with its project: PROJECT SAVED, the page stays); HOME leaves; **twice quickly**, anywhere: save the project |
| SAVE held | (3.3.6: the 8 patterns' keys lit, 1–4 and 5–8 each from faint to full, the pattern playing blinking; 13–16 faint) 1–8 **tapped**: that pattern on the bar, as it is (empty: a new pattern, no notes, with the sound that plays: PATTERN 3: NEW, EMPTY); **held 0.6 s**: the pattern that plays is first copied there, notes and sound (PATTERN 3: COPY OF 1; EDIT + OCT- undoes it), then the project is written: PROJECT SAVED; 13 LOOP / SONG, 14 SONG REC, 15 CHAIN, 16 this track / all |
| HOME held | **the project** (3.3.5: one, guaranteed to fit at its fullest; MEMORY shows how full it is): white key 1 **OPEN** (the saved one again), key 3 **NEW**, key 5 **SAVE** (3.3.6: a key left empty between them; lit while HOME is held; the one asked again blinks), each asked twice when something would be lost (AGAIN: CHANGES LOST, AGAIN FOR A NEW ONE); + SELECT: the menu (SCREEN: COLOR and ACCEL; LIGHTS; AUDIO; SEQ · SYSTEM, calibration) |
| EDIT tapped | OSC 1/6, FILTER, VOICE, PLUS, USER SOUNDS (your 32), **SOUNDS** (every sound by category, your 16 favourites) |
| FX tapped | SENDS, INSERT, INSERT MIX, SLICER, **DELAY** (TIME FDBK COLR MIX), **REV · CHO** (SIZE DAMP CRT CDP), MASTER |
| FX held + a key | a punch-in effect while the key is held (27, white and black keys); **FX held + HOME** locks the layer: play them one into the next; FX tapped lets go |
| GLO tapped | GLOBAL, TEXTURE (2 pages), MIDI (2 pages), **REC 6/6** (TRACK: when you change track while recording, **STOP** stops REC (the default), **KEEP** goes on recording on the new track; START: NOTE or CLICK); on these pages **SELECT turns the pages** (3.3.6; the tempo is KNOB 1 on GLOBAL); GLO + F#3: MIDI LEARN |
| 8 patterns a track | each keeps its own sound; a track can switch on its own (SAVE held + 16). Nothing to save apart: the 8 patterns are in the project. Each write of the project is shown: PROJECT SAVED (SAVE twice quickly, a copy by SAVE held + 1–8 held), PROJECT SAVED (AUTO) (the autosave, when stopped) |
| Undo | EDIT held + OCT- (8 levels), OCT+ redoes |

## The start and the visualiser

At power-on the **sloop logo** draws itself in (the dial, the hull, the mast, the two sails: the mainsail in five
bands in the tracks' colours, then the word sloop), about 2.5 s. A button, a key or a knob stops it at once (and does
what it usually does). No window with the name and the version follows: the version is in HOME. A window shows at the
start only when something asks for it (CORE 2: SILENT, SAFE START, SLOOP 2.8 IMPORTED).

**On the TRACKS screen, HOME tapped: the visualiser**, full screen. **SELECT** changes the style (its name shows for a
second; the last one is kept with the settings). **HOME** again, or any page button, closes it. The keys, PLAY, REC
and the layers work as usual (a layer held shows its page, then the visualiser comes back); the tempo is set with
GLO, KNOB 1 (GLOBAL). It sees the mix as if MASTER were at full. It costs the sound nothing: it is drawn in the screen's loop,
in place of the page.

| # | Style | What it shows |
|---|---|---|
| 1 | OSCILLOSCOPE | the mix's wave, standing still (caught on a rising edge), with a glow |
| 2 | SLOOP | the living logo: the mainsail's 5 bands are the tracks (they grow with their level, light up on a note), the dial the mix's level, the boat rolls on the wave |
| 3 | WIRES | a string per track, set ringing by its notes |
| 4 | VU METERS | the 5 tracks in their colours and the mix (L, R), peaks held, a dB scale |
| 5 | SPECTRUM | 40 bands from 40 Hz to 10 kHz, in the tracks' colours, with falling caps |
| 6 | LCD | the tempo on a 7-segment display, bar and beat, the 5 tracks as bars |

During a save (or a transfer from the editor), the visualiser freezes for a fraction of a second: its working memory
is the saves', lent when it is free.

## SLICE: a sample in slices

Choose BREAK 16, CHOP 8, REVERSE or USR SLICE in PRESETS (category FX). Each key plays a slice, **the lowest
first** (in MIDI, note 60 is slice 1); the sequencer plays the slices in the order you write.

| Value | What it does |
|---|---|
| SRC | **BREAK** (a bar of acoustic drums at 120 BPM, built in) or **USR1–4** (the slot's first sample, or a break cut in the editor: its slices; empty: BREAK) |
| DIV | **4, 8, 16, 32** equal slices, or **AUTO**: one per hit (found for you); **EDIT**: a break cut in the editor, its own slices |
| STRT | shifts the slices along the keys |
| PTCH | faster = higher (the key doesn't change the pitch) |
| MODE | **ONE**: the slice to its end; **GATE**: while the key is held; **LOOP**: looped while it is held |
| REV | backwards |
| DCAY | a fade over the slice (127: none) |
| TONE | a gentle filter |

At 120 BPM and DIV 16, the BREAK's slices fall on the steps: playing the 16 in order gives the bar back.

**Your own breaks (3.3.6), with the web editor:** Samples, **as a BREAK (slices)**, choose the file (or ● RECORD).
Put START and END around the bars, then cut it: **AUTO** (a slice per hit, up to 8, 12 or 16), **GRID** (4, 8 or 16
equal slices) or by hand (double-click adds a cut or takes one out; drag a cut; click a slice or its number to hear it;
**SNAP 1/16** puts the cuts on the 16ths). Choose how many bars it is: the tempo is shown. **Send to the FM-1**: the
break goes into the slot, a slice per zone, and, if you leave them ticked, **on the track you choose**: a synth track
plays it with **SLICE** (its SRC that slot), or **the drum track** plays it as one of **YOUR KITS**, named by the sample (slice 1 on the
first pad, up to 16: your break is your drums, and the four synth tracks stay free). Choose another kit and come back
to it any time: on the drum track, PRESETS (or EDIT > SOUNDS) has a category **YOUR KITS** after the factory kits, your
four sample slots by their sample's name and your SYN1–4 kits; it is kept with the project; **the break as the pattern playing** (each
slice where it was: PLAY gives the break back) and **its tempo**. Then play the slices on the keys, rewrite the pattern, add ratchets, reverse a slice (REV)… On the FM-1, DIV
shows **EDIT**: the slices are yours. On the drum track (a kit of your samples) and in SLOOP 2.8, the same slot plays
its slices on the 16 pads.

## The factory samples

The SAMPLE and GRAIN engines and the sampled drum kits play SLOOP 2.8.5's CC0 sets (piano, upright bass, vibes,
horns, strings, flute, scratch and an acoustic GM kit, see `assets/CREDITS-SAMPLES.txt`). To fit the FM-1's flash
they are stored at 11 kHz (the bass), 16 kHz (the tonal sets and the drums), 24 kHz (the cymbals and hats) and
22 kHz (the small bright percussion). Before 3.3.1, a mistake in the tool that builds the bank made many keys play
another set's sound: fixed in 3.3.1.

## Rescue

- **OCT- held at power-on: RESCUE.** PLAY starts with an empty project; your saved project stays, never overwritten
  by it (HOME held, OPEN brings it back; SAVE writes over it only when asked twice: AGAIN TO SAVE OVER IT);
  REC puts the chip in USB download mode (M-Flasher).
- **PLAY held at power-on: m-bootloader's menu** (BOOT APP, DOWNLOAD MODE, MIDI MODE to install again).
- **OCT- and OCT+ held: the calibration** of the buttons and knobs.

## The web editor

Open **[isod89.github.io/sloop3-fm1/editor.html](https://isod89.github.io/sloop3-fm1/editor.html)** (or
`editor-sloop3.html` from a release) in Chrome or Edge, connect the FM-1 running SLOOP 3 by USB, **Connect**. The
tabs on the left, FR / EN, **view: essentials / all**, **full screen** at the top; a **?** on each card explains it.

- **LIVE**: the FM-1 played from here, like SLOOP 2.8.5's editor LIVE.
  - **Transport**: PLAY / STOP, REC, FILL (held) and a one-bar FILL, BPM and SWING, TAP, SONG and SONG REC,
    OCT- / OCT+, and the FM-1's buttons held (FX, SEL, ENV, LFO, EDIT, GLO, HOME, SAVE, ARP, SEQ: the layers from the
    page).
  - **Patterns**: the 8 of each track (lit: it has notes; full: it plays; blinking: it comes next); ALL n: every
    track. A **chain** of patterns that follow each other.
  - **Scenes**: 8, each keeps the mix and the master; Shift-click to keep one; **MORPH** from A to B on the crossfader.
  - **Mix**: each track's level, meter, pan, filter, chorus / delay / reverb sends, mute, solo.
  - **Punch-in effects**: the 27 (STUT, REV, GATE, ECHO…) held, or **LATCH**.
  - **Keyboard**: the FM-1's 27 keys (and the computer's keyboard: Q 2 W 3 E… Z S X D C V G B); **drum pads**: play
    (loud at the top of the pad), **ROLL** (repeated at 1/8, 1/16, 1/32…), **ERASE** (the pad held erases its hits).
  - **Master**: volume (KNOB gives it back to the MASTER knob), DJ filter, DUST, DUCK, delay and reverb THROW.
  - **Set list**: your .s3p files; CUE puts them in a place and opens them at their tempo; GO plays them.
  - **Record the set**: the FM-1's USB audio recorded as a WAV.
  - **MIDI LEARN** (touch a control on the page, then move a knob of your controller), **LOCK** (hold to unlock),
    **PANIC**. Shortcuts: Space PLAY/STOP, Shift+R REC, Shift+1–8 a pattern everywhere, ← → the track, ↑ ↓ the
    octave, F fill, Esc PANIC.
- **Sound**: the 48 values of each track's sound as knobs, heard at once; the FM-1's sounds to choose from (by
  category, search); **A / B**, **COPY / PASTE**, **RANDOM** (by group, with an amount); **keep among your sounds**
  (one of your 32 places, with a name and a category).
- **Sequencer**: the piano roll (drums: the grid of the 16 sounds). Add, move, lengthen, select (Shift, box); for the
  chosen notes: velocity, length, offset, **condition** (FILL, %, 1:2…), **ratchet**, **ACCENT**, **SLIDE**; the
  velocity lane; the **lock** lane of a value; COPY / PASTE, ◀ ▶ a step, ±1 / ±12, DOUBLE, QUANTIZE, HUMANIZE, ↶ ↷;
  the track's DIV, GATE, SWING, QUANT; **MIDI files** in and out.
- **Mixer**: the whole tracks (DRIVE too), the shared delay, reverb and chorus, the master.
- **Song**: the song line by line (bar, each track's pattern, mutes), its length; play, send.
- **Library**: your 32 sounds (rename, category, move, erase, put on a track, .s3u file), your 16 **favourites**, the
  FM-1's sounds and kits.
- **Samples**: audio files into USR1–4, as an instrument, a 16-pad kit or **a BREAK cut into slices for SLICE** (see
  SLICE), **drawn, trimmed (START / END), looped (LOOP), heard, reversed, recorded** from the computer's input, saved as
  WAV; the **quality** fits long sounds (about 7 s a slot at 22 kHz, 10 s at 16 kHz, 14 s at 11 kHz); where SLOOP 2.8
  keeps them: it reads them too (keep 22 kHz for it).
- **Drum kit**: the kits; each pad's TUNE and DECAY. **Drum synth**: your SYN1–4 kits, every value of the 16 sounds.
  **FM6 bank**: B1–B27, DX7 voices (.syx) placed at once in an empty bank, every value, saved as .syx.
- **Projects**: the project saved on the FM-1 (file out, file in, open), the project as it is, **BACK UP EVERYTHING**
  (the project and your sounds in a .s3b file) and **RESTORE** (a backup from before 3.3.5, with several projects:
  you choose the one that goes on the FM-1). Keep your other projects as files; the **set list** (LIVE) plays them
  one after the other.
- **Settings**: everything in HOME + SELECT and the MIDI pages (COLOR, ACCEL, LIGHTS, KEYS, NOTES, SPEAKER, USB, RATE,
  RESO, PREVIEW, CORE 2; MIDI IN, SYNC, OUT, CLOCK OUT, DRUM MAP, REMOTE, FX, BEND; the REC mode, bars and count-in).
- **Theme**: your colours, with a preview, then chosen in HOME > SCREEN > COLOR (YOURS).

SLOOP 2.8's FM6 bank and SYN kits are never written: at the first save, SLOOP 3 keeps its own (after the application
area) and plays them instead. Until you save anything, they are SLOOP 2.8's.

While the editor sends something (a project, a sample, your FM6 bank or your SYN kits), the FM-1 neither saves nor
loads: it shows **EDITOR BUSY** and the autosave waits for the end. A transfer stopped halfway (page closed, cable
pulled) is dropped after 10 s (**SAMPLE: STOPPED** for a sample: the slot stays empty, send it again).
