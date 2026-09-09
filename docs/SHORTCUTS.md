# NAVA Extra 9 — key combinations

Every control on the panel, legacy and new. **Where** says which modes a
combination is live in; anything not listed for a mode does nothing there.

This is the index by key combination. [MANUAL.md](MANUAL.md) is the same ground
explained by *what the machine does*, and has every screen drawn out.

Mode abbreviations:

| | |
|---|---|
| **STEP** | pattern step edit (`PTRN` toggles into it) |
| **TAP** | pattern tap record (`SHIFT`+`TAP`) |
| **PLAY** | pattern play (`PTRN` toggles into it) |
| **TRKP** | track play (`TRK`) |
| **TRKW** | track write (`SHIFT`+`TRK`) |
| **MUTE** | mute mode (`MUTE`) |
| **KEYB** | keyboard / external-instrument mode |
| **all** | every mode above |

New in this firmware is marked with its feature number. Everything unmarked is
stock v1.028beta behaviour, preserved.

## Transport

| Keys | Does | Where |
|---|---|---|
| `PLAY` | start from step 1 | all |
| `STOP` | stop. Press again to continue from where it stopped | all |
| `SHIFT`+`PLAY` | arm / disarm tap recording. TAP LED blinks when armed | all **F9** |
| `TEMPO` held | show tempo; encoder sets BPM; pads show live triggers | all **6G** |
| `SHIFT`+encoder | set BPM in tenths | all but TAP **F30** |
| `TEMPO` tapped ×4 | tap tempo. Master sync only | all **F12** |
| `GUIDE` tapped | metronome — only when idle, and not within 0.8 s of effects use | all **3W** |

Under external MIDI clock the tempo knob sets nothing — the rate is measured from
the clock, and shown on the bottom line with an `S` in front of it (**3H**).

**Incoming notes play the voices in expander mode only** (`syn` = `EXP`). Master
and slave disconnect the note handlers, so a note does nothing there. The note
map is in [MANUAL.md §18](MANUAL.md#18-midi).

Shuffle, per-instrument swing, the sub-step retriggers and the roll all work when
slaved, which they did not before: the four internal ticks belonging to each clock
byte used to fire at the same instant, so anything finer than a step had nowhere
to land.

---

## Modes

| Keys | Does | Where |
|---|---|---|
| `PTRN` | toggle pattern step ⇄ pattern play | all |
| `SHIFT`+`PTRN` | pattern step | all |
| `SHIFT`+`TAP` | pattern tap record | all |
| `TRK` | track play | all |
| `SHIFT`+`TRK` | track write | all |
| `MUTE` tapped | enter / leave mute mode | all |
| `MUTE` held ≈⅓s | mute mode for as long as it is down, then back | all **3F** |
| `INST`+`PTRN` | pattern utilities — clear, move and group-paste ranges | all **F25 4T** |
| `INST`+`NUM` | browse patterns by name, or tracks in a track mode | all **F26** |
| `SHIFT`+`TEMPO` | config pages 1 → … → 6 → out; 1–3 and 6 while running | all **3T 4E 4X 5G** |
| `INST`+`GUIDE` | trig conditions, then links, then out | all **F18 F21** |
| `SHIFT`+`ENTER` | name the pattern, or the track in a track mode | all **F26** |
| `NUM` | keyboard mode, when EXT is the selected instrument | STEP |

Config pages, in order — ordered by how often a hand reaches for them mid-set,
so the rig setup is at the back:

| # | Page | Live |
|---|---|---|
| 1 | behaviour — `pCh` `sTr` `mut` `cym` | yes |
| 2 | hi-hats — `hhP` `mte` `oh ` | yes **4X** |

| 4 | sync and MIDI — `syn` `bpm` `mTX` `mRX` (each 1–16 or `OFF`) | no **5I** |
| 5 | SysEx dump | no |
| 6 | firmware — version, commit date, commit. Read-only | yes **4X** |

The paste mask no longer has a page: hold `SHIFT`+`BANK` or `SHIFT`+`MUTE` and the
pads are the mask (**5G**). `mTX`/`mRX` set to `OFF` send or receive **transport
only** — no notes, no program change, but clock, start/stop/continue and SysEx all
still pass (**5I**).

Settings change live on pages 1 to 4; **saving still waits for the stop.**

---

## Selecting

| Keys | Does | Where |
|---|---|---|
| `INST`+step | select instrument | STEP TAP |
| `INST`+ both steps of a pair | arm flam — BD, SD, LT, MT, HT only | STEP TAP |
| `INST`+steps 13+14 | select OH (open hat) | STEP TAP |
| `INST`+`ENTER` | select total accent; encoder sets it, the page stays up | STEP TAP **3E** |
| `INST`+`STOP` | select trigger out | STEP TAP |
| `SHIFT`+`GUIDE` | select external instrument | STEP TAP |
| step | select pattern (stopped in STEP, any time in PLAY) | STEP PLAY TRKW |
| two steps together | chain those patterns, first…second, round and round | STEP PLAY |
| one step | end the chain and select that pattern | STEP PLAY |
| `BANK`+step | select bank A–H | STEP PLAY TRKW |
| `NUM`+step | select track | TRKW |
| step | select track | TRKP |
| encoder press | move the field cursor on any page with one | all |
| MIDI program change | select that pattern, 0 = A01 … 127 = H16 | not TRKP/TRKW **3I** |

---

## Editing steps

| Keys | Does | Where |
|---|---|---|
| step | cycle the step: weak → strong → off | STEP |
| step | play, and record if armed | TAP |
| `LASTSTEP` held+step | set pattern length; every lane goes back to following it. Held only, never latched | STEP TAP **3Q 6K** |
| `INST`+`LASTSTEP` held+step | set that instrument's own loop length. Held only | STEP TAP **F27 6K** |
| double-tap `LASTSTEP` | nothing — `LASTSTEP` does not latch (**6M**) | all |
| double-tap `INST` / `SHIFT` / `CLEAR` / `SHUF` / `GUIDE` | same latch; the LED blinks while latched | all **F28** |
| double-tap `GUIDE` | …and the metronome switches off with it | all **3Y** |
| `SCALE` | cycle 1/16 → 1/32 → 1/8T → 1/16T | STEP TAP |
| `SHIFT`+`SCALE` / `SHIFT`+`LASTSTEP` | +OCT / −OCT | KEYB |
| `LASTSTEP` held + encoder | nudge the pattern length | STEP TAP **3C** |
| `INST` held + encoder | walk the voices | STEP TAP **3C** |
| `BANK` held + encoder | walk the patterns, with a preview; commits on release | all **3C** |
| `TRK` held + encoder | walk the tracks, with a preview; commits on release | all **3C** |
| `SHUF` held | show shuffle and swing; steps set shuffle and flam | STEP TAP |
| `SHUF` held + encoder | the selected instrument's own swing | STEP TAP **F19** |
| `SHUF` held + `CLEAR` | put every instrument back on the pattern's shuffle | STEP TAP **F19** |
| pad held + encoder | roll rate | TAP **F10 F11** |
| `SHIFT`+encoder | roll rate — no pad needed | TAP **F10 F11** |
| `TAP` | shift the whole pattern left (on release, if no pad joined) | STEP TAP PLAY |
| `DIR` | shift the whole pattern right (on release, if no pad joined) | STEP TAP PLAY |
| `TAP` held + pad | delete that step; the later ones pull left | STEP TAP **F8** |
| `DIR` held + pad | insert a blank there; the later ones push right | STEP TAP **F8** |
| `<<` / `>>` held + pad | the same two, on panels that have them | STEP TAP **F8** |
| `INST` + any of the above | act on the selected instrument only | STEP TAP |

---

## Clearing and undo

| Keys | Does | Where |
|---|---|---|
| `CLEAR` held | erase the selected instrument as the playhead passes | STEP (running) |
| `CLEAR`+step(s) | erase those instruments as the playhead passes | TAP **F5 3M** |
| `CLEAR`+`INST`+step(s) | wipe those whole instruments at once, on release; follows `mte` | **all** **F5 3K** |
| `CLEAR`+`PTRN` | clear the whole pattern, running or stopped | all **3E** |
| `SHIFT`+`CLEAR` | undo, four levels. Shows `UNDO n/4` | STEP TAP PLAY MUTE **F4** |
| `INST`+`PLAY` | blocked — INST is half of nine chords and START is next to them (**6V**) | all |
| `SHIFT` or `INST` **latched** | `PLAY`/`STOP` do not transport — the layer owns them (**6X**) | all |
| `SHIFT`+`INST`+`CLEAR` | redo. Shows `REDO n/4`. Undo and redo share the four levels | STEP TAP PLAY MUTE **6S** |
| `CLEAR`+`ENTER` | revert to the saved pattern. Shows `REVERTED` | all **3C** |
| `CLEAR` | rewind the track to its first measure | TRKP TRKW |

Undo covers one *gesture*, not one step: a held clear, a bar of tap recording,
a paste, a shift, or a generator roll each cost one level. It is discarded when
the pattern changes, and it does not cover the pattern utilities — those are
multi-record EEprom rewrites, and the ENTER-hold is their confirmation instead.

**None of these reach the chip.** Clearing is an edit like any other: it changes
the pattern you are hearing and leaves the saved copy alone until `ENTER`. The
`ENTER` LED blinks while anything is unsaved, `CLEAR`+`ENTER` puts the saved
version back, and changing pattern reloads the record — which quietly discards an
unsaved clear. `CLR` in the pattern utilities is the exception: it writes the
EEprom directly and at once.

---

## Copy, paste, save

| Keys | Does | Where |
|---|---|---|
| `SHIFT`+`BANK` | copy the pattern (on the press; a copy is unmasked) | all |
| `SHIFT`+`MUTE` | paste it — on release of `MUTE`, with the mask as it stands | all **3U 5G** |
|   release `SHIFT`, keep `BANK`/`MUTE` | the mask stays open with a hand free | all **5G** |
| `SHIFT`+`BANK`/`MUTE` held + step | toggle that paste-mask group | all **3T** |
| `SHIFT`+`BANK` or `SHIFT`+`MUTE` held + step | choose what a paste transfers; the first eight LEDs show it | all **F29 5G** |
| config page 4 | send a SysEx dump — encoder picks, `ENTER` held sends | stopped **F23** |
| `ENTER` | save the pattern (LED blinks when unsaved) | STEP TAP PLAY |
| `ENTER` held | save the track | TRKW |
| `ENTER` | save the setup | config pages |
| `ENTER` release | write the pattern at this track position, advance | TRKW |

---

## Performance fill and cyclic record

| Keys | Does | Where |
|---|---|---|
|   + pad | cycle that voice: left alone → rising → falling | **4I** |
|   + encoder | intensity for the whole kit; centre is the pattern as written | **4I** |
| leaving | level back to zero, map kept | **4I** |
| `SHIFT`+`PLAY` | arm recording | TAP **F9** |
| `SHIFT`+`STOP` | cyclic record on/off — screen says `CYC` | TAP **4H** |

`SHIFT`+`PLAY` and `SHIFT`+`STOP` need `SHIFT` **held**, not latched — a latch
never captures the transport (**4U**).

The fill never writes the pattern, so the centre detent is the original exactly —
nothing to save, nothing to undo. Cyclic record is not saved: power-on is always
plain record.

---

## Hi-hats: which pad, and when

| The pads are… | Open hat is | Where |
|---|---|---|
| playing or erasing | **both pads together**, always | tap record, roll, `CLEAR`+pads, `CLEAR`+`INST`+pads |

Where a pad plays or erases, the chord is the gesture itself and does not
change. Where it only names a voice, both answers are defensible, so it is a
setting — `2nd` by default, since ten of eleven voices already take one press
and it sidesteps the leading-pad flicker `BTH` cannot avoid. The mute row is
unaffected either way; it has `mte` of its own.

---

## Mute and solo

| Keys | Does | Where |
|---|---|---|
| step | toggle that instrument's mute | MUTE |
| `INST` held + steps, then release | change them all together | MUTE **F13** |
| `CLEAR` held + step | solo that instrument; several can be soloed | MUTE **F16** |
| `CLEAR`+`MUTE` | solo the selected instrument, from any mode | all **F16** |
| encoder press | clear every mute and every solo | MUTE |

Solo overrides the mutes rather than merging with them, so leaving solo restores
exactly the mute state you had before. Soloing CH or OH keeps the hat audible.

Mute mode reads as one sentence: a **pad** toggles one mute, **`INST`**+pads
builds a group that changes together on release, **`CLEAR`**+pad solos. The
modifier says what kind of change and the pad says which voice.

Solo is a mask, so several voices can be soloed at once — press a soloed voice
again to drop it. Soloed voices blink on the step LEDs, because solo silences
everything else and the row would otherwise just read "all muted".

`mte` on config page 2 decides whether the two hi-hat lanes move together for
single mutes and for solo. **The group always keeps them separate**, whatever the
setting says — deliberately, and the one asymmetry in the mode. A group is
assembled before it is applied: you can see every pad you have picked blinking,
correct it, and only then let go. That is exactly where naming one hi-hat lane on
its own is worth the inconsistency, and the only place a wrong guess costs
nothing to fix.

---

## Performance

| Keys | Does | Where |
|---|---|---|
| `SHIFT`+`DIR` | cycle the pattern's own direction, permanently | PLAY |
| pad held | roll that instrument, if the rate is not `--` | TAP **F10 F11** |
| pad held + encoder | set the roll rate | TAP **F10 F11** |
| `SHIFT`+encoder | set the roll rate without holding a pad | TAP **F10 F11** |

### Backing up from the panel — config page 4 — **F23**

`SHIFT`+`TEMPO` four times, stopped. The encoder picks **everything**, **this
pattern**, **all tracks** or **setup only**, and `ENTER` *held* sends it.

**everything** means patterns, tracks *and* setup. Tracks used to be missing from
the dump entirely, which made "back up everything" a promise the format did not
keep — a restored machine came back with its patterns and none of the songs built
out of them.

Held, not pressed, for two reasons. A plain `ENTER` on a config page means "save
the setup" everywhere else and has to keep meaning that; and a full bank dump
blocks the panel for about forty-six seconds with no way to call it back, which
is the same class of irreversible as the pattern utilities and gets the same
deliberate gesture. The display says `HOLD` so the page is not a guessing game
about which key fires it.

The step LEDs show which of the three the encoder is on, and the dump then
drives the same LEDs as a progress bar — the choice becomes the progress without
the page having to explain itself.

**Restoring needs nothing here.** Incoming SysEx is handled whenever it arrives,
in any mode, on any RX channel, so a restore is just a matter of playing the file
back into the machine. Patterns, tracks and setup all come back in; once the
transfer has been quiet for half a second the current pattern and track are
reloaded from the bank and the display says `SAVED`.

There is also an **identity request** — command `0x03` — which answers with the
record format, the bank geometry and the firmware version string. That is the
handshake a host-side tool needs before it sends anything: a restore aimed at the
wrong format is the one mistake that costs somebody their bank, and it is cheap
to make impossible to walk into.

| Command | Direction | Meaning |
|---|---|---|
| `0x01` | in | dump everything |
| `0x02` `n` | in | dump pattern `n` |
| `0x03` | in | identity request |
| `0x10` `n` `p` … | both | pattern `n`, page `p` |
| `0x11` `0` `0` … | both | the setup page |
| `0x12` `n` `p` … | both | track `n`, page `p` |
| `0x13` … | out | identity reply |
| `0x7F` | out | end of dump |

All under manufacturer ID `0x7D` (non-commercial), device `0x4E`. Page payloads
are the usual 8-bit-to-7-bit packing, 64 raw bytes to 74.

### Browsing by name — `INST`+`NUM` — **F26**

| Keys | Does |
|---|---|
| encoder | move through the list |
| `INST`+encoder | jump a bank at a time |
| pad | jump to that entry in the current bank |
| encoder press | select it and close |
| `NUM` | swap between the pattern list and the track list |
| `CLEAR` | go back to the one actually playing |

The other half of naming. A name was worth adding because a pattern is easier to
remember as `intro` than as C3, but until now the only way to *reach* one was
still to know which pad it was under — so the name was a label you could read and
never a thing you could look for.

The number stays on the top line with a `→` against the entry actually playing,
and the step LEDs show where the cursor is inside its bank with the playing one
blinking. Scrolling away and finding your way back is the common case, so it is
one key.

Selecting queues the change exactly as a pad press does: on the bar line while
running, immediately when stopped. In a track mode the list is tracks instead,
and `NUM`+pad still selects one directly.

Names are read from the bank one at a time as the cursor moves, not cached — 128
names is 1536 bytes of RAM to hold something the hand moves through one entry at
a time, and a twelve-byte read at 400 kHz is well under a millisecond. A record
that has never been written shows blank rather than sixteen blocks.

### Insert and delete a step — **F8**

Hold `TAP` or `DIR` and press the pad where you want the cut. `TAP` pulls the
later steps left over the one you pressed; `DIR` pushes them right and leaves a
blank in its place — the same directions those two keys already shift in, with
the pad saying where. `INST` narrows it to the selected lane, exactly as it does
for the plain shifts. `<<` and `>>` do the same thing on panels that have them.

A shift rotates a lane and keeps everything. These change how many steps sit
*between* the notes, which is the edit you want when a part is right except that
one hit lands a step too early.

Both work inside the lane's own span rather than the whole sixteen. A lane with a
lane length of seven is seven steps long, and pushing its notes out into
steps it never plays would look like the edit had eaten them. Insert drops the
last step off the end and delete leaves a blank there — the alternative is moving
the pattern length underneath your hand, and a gesture that silently makes the
bar longer is not an edit. Trigs, flams, velocities and trig conditions all
travel together. One undo level per press.

Without a pad they still shift the whole pattern. All four moved to release for
this, and fire only when no pad joined the hold, so reaching for an insert cannot
shift on the way in.

### Swing per instrument — **F19**

Hold `SHUF`. Steps 1–7 set the pattern's shuffle and steps 9–16 set flam, as
they always did; the encoder sets the **selected instrument's own swing**, and
the display names whose it is. `CLEAR` puts every instrument back.

Shuffle and flam each get a bar, since each has a row of pads behind it. Swing is
the digit in the top right, beside the instrument it belongs to — it is the one
value on the page the pads cannot reach, so it sits next to the knob that sets it
rather than taking a line of its own (**3E**). A dash there means *follow the
pattern*, which is where every instrument starts: the page opens saying the
pattern is in charge and you take voices off it one at a time. A swung hat over a
straight kick is two values.

How it works is worth knowing, because it changed the shape of the fire path. The
step now starts on the grid and each instrument's trigger is held back by its own
number of ticks, rather than the whole step being shifted by one offset. With no
per-instrument values set every instrument lands in the same group and the result
is note-for-note what the old whole-step offset produced — the pattern shuffle
is simply the default every instrument inherits now.

Delay is capped at half a step minus a tick. That keeps the last instrument clear
of the half-step precompute, which recharges the sample-and-hold caps for the
step firing next: an instrument leaving after it would play at the wrong
velocity. The cap also retires a stock bug — at 1/32 the deepest shuffle put the
swung step on the precompute tick exactly, so it took the next step's levels.
That combination is a tick earlier than it used to be, and correct now.

The hi-hat follows whichever of `CH` or `OH` won the step, because `HH` is the
trigger they fire through rather than a lane of its own — the decay select has
to arrive with the hit that needs it. Rolls and the `DIR` repeats ride the
pattern's shuffle rather than a per-instrument one.

### Tempo in tenths — **F30**

`SHIFT` and the encoder move tempo a tenth of a BPM at a time, carrying into the
whole number at either end so the fine control is not a separate range you can
get stuck inside. Plain turns still move whole BPM, because a bar's worth of
turning to cross ten of them would be worse than no fine control at all. Tap
tempo lands on a tenth too — four taps rarely land on a whole number, and
rounding threw away most of what the hand gave.

Tap mode is the exception: `SHIFT`+encoder is the roll rate there. Tempo is not
what a hand is doing in tap mode.

The tenths came with a fix to the tick calculation, which used to truncate
`BPM×96/60` to a whole number before dividing. That put whole-BPM tempi off by
up to 1.5 % anywhere the BPM was not a multiple of five — 31 BPM ran 1.2 % fast.
Tempi that are multiples of five, 120 included, are unchanged.

### Instrument roll — **F10 F11**

Hold a pad in tap mode and that instrument repeats. The rate is on the top line
of the tap display, in the column that shows lane length in step mode, and the
encoder sets it while a pad is down — the one moment in tap mode when a hand is
free and tempo is the last thing you want. Release the pad and the knob goes
back to tempo.

`--` is off and is the default, so nothing repeats until you ask it to. `x1` to
`x8` are hits per *step*, not per beat, so a roll follows the scale: `x4` is four
to a step whether the step is a 1/16 or a 1/8 triplet. `x3` and `x6` are the
triplet settings against a straight scale. `/2`, `/3` and `/4` go the other way
— one hit every two, three or four steps — so the same control reaches from a
slow half-time pulse up to a buzz.

Rolls ride the shuffle, so a roll against a swung pattern swings with it rather
than playing straight and arriving as a flam on every downbeat. Several pads can
roll at once, and each repeats at the velocity of the pad that is down — the
weak button rolls weak.

With recording armed the roll records, quantised: each hit is written at the step
the recorder is pointing at, so rates finer than one per step fill the steps they
pass over. That is the quantising — the pattern holds one trig per step.

The one limit is the 2 ms trigger pulse: `x8` at 1/32 comes out as six hits
rather than eight, because a repeat the drum never sees a falling edge for is not
a repeat.

---

## The step-edit display

```
ptr len lan ins
A1   16  --  BD
```

`ptr` pattern · `len` pattern length · `lan` the selected instrument's own loop
length, or `--` when it simply follows the pattern · `ins` selected instrument.

`lan` replaced the scale readout, which the faceplate LEDs already show. A lane
length changes what you hear and had no indication anywhere but the lane
page.

---

## Trig conditions — `INST`+`GUIDE`

| Keys | Does |
|---|---|
| step | choose the step to edit |
| `INST`+step | choose the instrument | **F17** |
| encoder press | move between Step, Condition, Probability, Instrument |
| encoder | edit |
| `CLEAR` | take the condition off whatever is selected |

Every step carries a **condition** and a **probability**, and both must pass for
the note to sound.

A condition is `X:Y` — play on the Xth of every Y passes of the pattern. `1:2`
is every other bar, `3:4` the third of four, `1:8` one bar in eight. Probability
is independent and applies after it, so `1:4` at 50 % is a note that happens on
one bar in four, and only half of those.

**Step `ALL`** is the instrument as a whole. Instrument and step conditions both
have to pass, so a lane can be thinned without touching the steps inside it.

The step LEDs show which steps carry a condition, with the one being edited
blinking — a conditional trig looks identical to any other in the pattern, so
this is the only place it is visible.

Conditions are decided half a step before the trigger fires, not at the trigger
itself, so they cost nothing in timing.

---

## Links — `INST`+`GUIDE` twice

```
src tgt rule
CH  BD  NOT
```

An instrument may be made to depend on another. Encoder press moves between
source, target and rule; step buttons pick the source; `CLEAR` removes the rule.

| Rule | Source plays only when the target |
|---|---|
| `AND` | is sounding on this step |
| `NOT` | is **not** sounding on this step |
| `PRE` | sounded on the previous step |
| `NPR` | did not sound on the previous step |

`NOT` is selective choke. Separate analog voices cannot cut a note that has
already started — there is no gate to close — so the only choke available is to
not start it, which is what this does. The hi-hat is the exception and has real
hardware choking, set by `hhP` on config page 2.

Rules read whether the target was *allowed to sound*, before the target's own
rule is applied. Two instruments pointing at each other therefore resolve in one
pass instead of chasing one another: mutual `NOT` silences both, which is at
least predictable.

---

## Track write

| Keys | Does |
|---|---|
| `<<` / `>>` | previous / next position in the track |
| `CLEAR` | back to the first position |
| `SHIFT`+`NUM` | jump to the last position |
| `SHIFT`+`<<` | delete the pattern at this position |
| `SHIFT`+`>>` | insert the current pattern here |
| `ENTER` release | write the current pattern here and advance |
| `ENTER` held | save the track |
| encoder | edit the field under the cursor: position, pattern, length |

---

## Keyboard mode

Reached with `NUM` while EXT is the selected instrument, in pattern step mode.
The sequencer stops on entry.

| Keys | Does |
|---|---|
| step | play and enter a note |
| `SCALE` | octave up |
| `LASTSTEP` | octave down |
| `<<` / `>>` | previous / next note in the sequence |
| `CLEAR` | back to the first note |

---

## Config pages — `SHIFT`+`TEMPO`, stopped only

Encoder press moves the cursor, encoder edits, `ENTER` saves.

**Page 1** — `syn bpm mTX mRX`: sync source (master / slave / expander),
default tempo, MIDI transmit channel, MIDI receive channel.

**Page 2** — `pCh mte hhP sTr`: pattern change timing (sync to bar / free),
hi-hat mute pairing, **hi-hat priority** — which of CH/OH sounds when both have
a trig on the same step (**3B**) — and **single trigger**, which makes step edit
a plain on/off with no weak/strong (**F7**).

**Page 3** — paste contents (**F29**). The step buttons are the toggles:

| Step | | Step | |
|---|---|---|---|
| 1 | **T**rig | 5 | **N**ame |
| 2 | **V**elocity | 6 | e**X**t notes |
| 3 | **F**lam | 7 | **E**uclid |
| 4 | **S**etup | 8 | **C**onditions |

Upper case on the display means included in a paste, lower case excluded.
Name is off by default.

---

## Pattern utilities — `INST`+`PTRN`

| Keys | Does |
|---|---|
| encoder press | move between Operation, First, To, Destination / Source |
| encoder | edit |
| step | jump the field to a bank boundary |
| `ENTER` held 1 s | run it |
| `CLEAR` | leave |

The range is a pair of endpoints: `frm 009 to 016` is eight patterns. Moving
`frm` leaves `to` where it is and the range grows or shrinks against it (**3E**).

`CLR` blanks a range — clear-all is that across all 128. `MOV` moves a range and
slides everything between to fill the gap. Stopped only. Tracks are remapped so
songs keep playing the same music, and group membership is cleared across the
moved span because a group is defined by position.

`PST` overlays the copy buffer onto every pattern in the range, through the
paste mask (**4T**) — copy a pattern first, set the mask with the copy chord held, and
*“give these thirty-two patterns the same choke rules”* is one gesture. Only
what the mask selects is written. The fourth
field shows which pattern the buffer holds. Nothing copied means `ENTER`
refuses rather than blanking the range.

Not covered by undo. Take a SysEx dump first if the bank matters.

---

## Naming — `SHIFT`+`ENTER`

| Keys | Does |
|---|---|
| step 1–11 | move the cursor — the step LEDs show where it is |
| encoder | change the character under the cursor |
| `<<` / `>>` | nudge the cursor |
| `CLEAR` | blank the whole name |
| `ENTER` | leave, and save |

Eleven characters from space, `A–Z`, `0–9`, `- . / # +`. A blank cell starts
from the last character you chose, so a run of similar letters is a small turn.
Moving the cursor never writes anything.

---

## Held at power-on

| Keys | Does |
|---|---|
| `PLAY`+`STOP` | offer to initialise the EEprom — then `PLAY`+`ENTER` to confirm |
| `STOP`+`ENTER` | EEprom self-test, about 25 s. Reports pass or the failing address |
| `ENTER` | TM2 velocity trim: holds the DAC at full scale |

The first boot after a firmware update may also offer to convert a v1 pattern
bank to format v2. `PLAY` converts in place and keeps everything; `ENTER` gives
a fresh empty bank.

---

## Leaving a page

Any of `PLAY`, `STOP`, `PTRN`, `TRK`, `MUTE` or `SHIFT`+`TAP` closes whatever
page is open — conditions, links, pattern utilities, name edit or the paste
config — and then does its own job as well. The key that opened a page still
closes it too; this is an extra way out, not a replacement.

Plain `TAP` is deliberately not an exit: it shifts the pattern, and leaving a
page should not edit one.

Step buttons still held when a page closes are ignored until released, so
leaving a page with a pad down cannot select a pattern or edit a step with it.

---

## Notes on modifiers

`SHIFT`, `INST`, `CLEAR`, `BANK`, `NUM`, `LASTSTEP`, `SHUF` and `GUIDE` all act as
modifiers while held, and the step buttons belong to whichever is down rather
than to the mode. `TEMPO` is a modifier only with `SHIFT`.

`TAP` and `DIR` shift the pattern left and right on release, and carry insert and
delete when a pad joins them. Neither latches: a release that edits the pattern
cannot coexist with a double-tap, because the first tap of the pair is
indistinguishable from an ordinary quick press at the moment it is let go.

`GUIDE` does latch, because nothing about it edits on release — its metronome
toggle waits out the double-tap window before deciding.

### Latching — **F28**

`INST`, `SHIFT` and `CLEAR` latch on a double-tap. `GUIDE`, `LASTSTEP` and `SHUF` do not (**6M 6T 6X**)
and release on a single press. Asymmetric on purpose: getting *out* of a mode
should never need a gesture you have to time. Their LEDs go solid while held and
blink while latched, because a lit LED with your hands off the panel has to look
different from one you are holding down.

`PLAY`, `STOP`, `SCALE` and the mode keys do not latch — their press does
something, so latching them would only mean repeating it.

A latched modifier means exactly what a held one means, with **one exception:
it never captures `PLAY` or `STOP`**. Those two read the physical key.

That exception was argued the other way round for a long time — a modifier that
half-applies is worse than one that applies everywhere, and a latch you can see
blinking and clear with one press is not a latch you forgot. Both halves fail in
the one place it matters. Walking the config pages is `SHIFT`+`TEMPO` pressed
over and over, two of which inside the double-tap window latch `SHIFT` with
nobody having decided it should be; from there `PLAY` armed tap record and
`STOP` toggled cyclic record, so neither transport key answered. The blinking
LED is not where anyone looks when the question is why `PLAY` does nothing, and
*the transport will not answer* is the worst failure this panel has (**4U**).

So hold `SHIFT` for `SHIFT`+`PLAY` and `SHIFT`+`STOP`. Every other chord still
honours the latch, which is what the latch is for.

Two more holes, both about damage rather than surprise:

- **`CLEAR` still needs a real hold**, both under the playhead and with `PTRN`.
  Held `CLEAR` erases the step being played, every bar; latched, it would quietly
  eat the whole instrument while nobody was touching the panel. And a pattern
  wiped by a latch nobody remembered setting is the one outcome no blinking LED
  makes acceptable. A latch may still *arm* a destructive chord —
  `CLEAR`+`INST`+pads works latched — but it must not sustain a destructive act,
  and it must not stand in for a hand on the key that fires one.

### `INST`+step means "choose an instrument"

Wherever the step buttons already mean something else — conditions, links,
mutes, pattern length, the clear chord — holding `INST` and pressing a pad picks
the instrument instead. One idiom, so no page has to invent its own way of
changing what it is aimed at.


---

## The display — **3B**

The pattern screens and mute share one shape: the top line is what you are
pointed at, the bottom line is where you are, and the last three cells of the top
line are the state zone — cell 13 the latched modifier (`S I C F L G`, `*` for
several), cells 14–15 the running effect as its pad number.

Edit pages do not carry it: they need all four labels, and you are editing rather
than performing while you are on one.

The tempo is on the bottom line of every pattern screen now, so `TEMPO` held is
only needed in the modes that do not show it. **In slave sync it is the tempo
arriving**, measured from the incoming clock and marked `S` in the cell in front
of it; dashes there mean no clock is coming in (**3H**).

---

## The two grammars — **3C**

The front panel prints a second function in orange under most keys. That is the
`SHIFT` layer, and this firmware no longer contradicts it:

**`SHIFT` + key = the orange word.** `WRITE` `DEL` `INS` `LAST MEAS` `STEP` `TAP`
`DIR` `EXT INST` `COPY` `PASTE` `CONFIG` `−OCT` `+OCT`. No exceptions.

**`INST` + key = open the page about that thing.** `SCALE`→lane timing,
`SHUF`→generator, `GUIDE`→conditions, `PTRN`→utilities, `NUM`→browser,
 `ENTER`→total accent, pad→select instrument.

**`CLEAR` resets the subject of whatever page is open.** The lane on the
condition on the conditions page, the link on links, the
range on utilities, the swing on the shuffle page,
and the cursor on the browser. With no page open it keeps its own clears.

**The encoder** is owned by a page if one is open, otherwise by the held
modifier, otherwise it is the tempo.


---

## Three ways back — **3C**

| Gesture | Goes back to |
|---|---|
| `SHIFT`+`CLEAR` | the last four edits, one press each |
| `SHIFT`+`MUTE` (`PASTE`) | whatever you last copied with `SHIFT`+`BANK` (`COPY`) |
| `CLEAR`+`ENTER` | **what is actually saved on the chip** |

The third was the missing one. Undo walks back four edits and the snapshot goes
to wherever you last copied — after ten minutes of live mangling the edits are
past counting and the snapshot was taken somewhere you no longer remember. This
reloads the record, which is the only fixed point there is.

It stands aside while a pattern change is queued: the back buffer belongs to the
incoming pattern and the bar line is about to replace everything anyway.


---

## Messages — **3D**

Save, undo, revert, copy and paste each own the whole display for a moment.
Nothing can paint over one, which is what went wrong before: they were written
into the frame from the button handler and the next redraw in the same pass wiped
them, so `UNDO n` was never once visible and only the "nothing to undo" case
survived — because a failed undo changes nothing and nothing asks for a redraw.

Most last a second. **Copy and paste stay up as long as their keys are held**,
plus a short tail, and show the paste mask while they do.
