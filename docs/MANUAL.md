# NAVA Extra 9 — User Manual

Firmware **2.0-dev**, a fork of e-licktronic's NAVA v1.028beta.

This manual covers everything the firmware does, from the stock TR-909-style core
outwards. It is the reference for *what the machine does*;
[05-SHORTCUTS.md](05-SHORTCUTS.md) is the same ground indexed by key combination,
and is the faster lookup once you know what you are looking for.

Every screen in the firmware is drawn in §16.

---

## Contents

1. [Before anything else](#1-before-anything-else)
2. [The panel](#2-the-panel)
3. [Transport](#3-transport)
4. [The seven modes](#4-the-seven-modes)
5. [Instruments](#5-instruments)
6. [Writing a pattern](#6-writing-a-pattern)
7. [Length, scale and feel](#7-length-scale-and-feel)
8. [Clearing and undo](#8-clearing-and-undo)
9. [Patterns, banks and groups](#9-patterns-banks-and-groups)
10. [Tracks](#10-tracks)
11. [Naming and browsing](#11-naming-and-browsing)
13. [Conditions and links](#13-conditions-and-links)
14. [Performance](#14-performance)
15. [Configuration](#15-configuration)
16. [Screens with no page of their own](#16-screens-with-no-page-of-their-own)
17. [Boot-time functions](#17-boot-time-functions)
18. [MIDI](#18-midi)
19. [Keeping this file current](#19-keeping-this-file-current)

---

## 1. Before anything else

**The hardware.** NAVA Extra 9, ATmega1284 at 16 MHz, 16×2 character display, 16
step pads, a rotary encoder with a push switch, and a 24LC1025 EEPROM holding 128
patterns, 16 tracks and the global setup.

**The one thing to know.** Everything this fork adds sits on top of the stock
panel as a chord. Nothing that worked on a stock NAVA has moved. If a combination
in this manual does nothing on your machine, it is a bug — there is one firmware
and everything in here is always available.

**Two grammars.** `SHIFT` + a key means the orange word printed under it on the
panel. `INST` + a key opens the page about that thing. Nothing else opens a page,
and `SHIFT` never means anything the panel does not say.

**Saving is manual.** Edits live in RAM until you press `ENTER`. The `ENTER` LED
blinks whenever there is unsaved work. Changing pattern discards unsaved edits
without asking, which is stock behaviour and worth remembering.

---

## 2. The panel

### Buttons that do something on their own

| Key | Alone |
|---|---|
| `PLAY` | start from step 1 |
| `STOP` | stop; press again to continue from where it stopped |
| `PTRN` | toggle pattern step ⇄ pattern play |
| `TRK` | track play |
| `TAP` | shift the whole pattern left (acts on release) |
| `DIR` | shift the whole pattern right (acts on release) |
| `SCALE` | cycle 1/16 → 1/32 → 1/8T → 1/16T |
| `GUIDE` | metronome on/off — on a **tap** only |
| `MUTE` | tap: enter/leave mute mode · hold: mute mode until you let go |
| `ENTER` | save |
| `TEMPO` | hold to show tempo; tap ×4 for tap tempo |

`TAP`, `DIR` and `GUIDE` act on **release**, and only if no pad joined them. That
is what lets each of them carry a chord without doing its own job on the way in.
On this panel `<<` and `>>` are the direct legends of the `TAP` and `DIR` keys.

`GUIDE` is stricter still, because it is the busiest key on the panel. **The
metronome is what it means when it means nothing else.** Four things take the
press away from it:

- a modifier alongside it — `SHIFT` or `INST`, in either release order;
- a hold rather than a tap;
- the single press that releases a latch, which is the only way out of one;
- **an effect running, or having run in the last two seconds.** Effects have
  priority. A tap while one is going does nothing at all: it will not toggle the
  metronome, and it will not cut the effect short either — an effect ends at the
  bar line and no key press moves that. The two seconds afterwards matter as
  much: leaving the latched menu is one press, so the *next* press is a
  technically idle `GUIDE` while your hand is still in the middle of using the
  effects. The window reads those as one continuous gesture rather than two that
  happen to share a key. The window is about eight tenths of a second.

So to toggle the metronome, `GUIDE` has to be idle — nothing held, nothing
latched, nothing running.

**Latching the effects menu switches the metronome off.** Refusing to toggle it
was not enough: one already running carried on underneath the effects, which is
both of the key's jobs happening at once. A double-tap is a declaration that
`GUIDE` is the effects key now, so it takes the key's other job with it. It stays
off when you leave the latch — putting a click back under your hands at the end of
a performance gesture is the worst moment to be surprised by one, and it is a
single tap away once the key is idle again.

### Modifiers — held, not pressed

`SHIFT`, `INST`, `CLEAR`, `SHUF`, `LASTSTEP`, `BANK`, `NUM`, `GUIDE`, `TAP`,
`DIR`. While one of these is down the step pads belong to it, not to the mode.

**Latching.** `INST`, `SHIFT` and `CLEAR` latch on a
**double-tap** and release on a single press. Asymmetric on purpose: getting out
of a mode should never need a gesture you have to time.

The **top right of the home screens** names whichever modifiers are in play —
**held or latched**, since either is a reason the pads are doing something other
than what the mode says. Up to three letters in cells 13–15: `S`hift, `I`nst,
`C`lear, shu`F`, `L`aststep, `G`uide, always in that order so a key keeps its
place. Four or more and the third cell becomes `*`. While an effect is running
the slot number takes two of those cells back and the zone collapses to one,
with `*` for more than one.

**`LASTSTEP` and `SHUF` do not latch at all.** While `SHUF` is down pads 1–7 set
the pattern shuffle and 9–16 the flam interval, both absolutely and both audible
on the next step, so a latch nobody meant to set turns every pad press into a
change of feel. `LASTSTEP` is worse still: it is the most destructive unmodified
gesture on the panel — one pad sets the pattern length absolutely and returns
every lane to following it, so pad 1 turns what you were playing into a one-step
loop with its polyrhythms gone. Two quick presses are a double-tap whether or not
you meant one, and a gesture that is one accidental pad press from destroying a
pattern is not one a latch should unlock. Both of its gestures — `LASTSTEP`+pad
and `LASTSTEP`+encoder — are held.

**`GUIDE` does not latch.** `INST`+`GUIDE` walks conditions → links → out, which
is the same key pressed twice in quick succession — a double tap by every measure
the firmware has — so cycling those pages latched the effect menu underneath them.

**A latched `SHIFT` or `INST` owns `PLAY` and `STOP`.** Under either of them, held
or latched, the transport keys belong to that layer and do not start or stop:
`SHIFT`+`PLAY` arms tap recording, `SHIFT`+`STOP` toggles cyclic record,
`INST`+`STOP` names the trigger output, and `INST`+`PLAY` does nothing at all. One
key, one job, decided by what is down.

> **If the transport stops answering, look at the top-right corner.** `S` or `I`
> there means a latch is holding the key; one press of that modifier releases it.
> This is the one place where a latch you did not mean to set can be confusing,
> which is why only three keys latch at all and why both of them are named on
> screen and blink on the panel.

**A latch never captures `PLAY` or `STOP` for a *third* meaning.** They read the
physical key for everything else, so
a latched `SHIFT` gives you every one of its chords on the pads and still lets
the transport answer. This matters more than it sounds: walking the config pages
is `SHIFT`+`TEMPO` pressed over and over, two of which inside the double-tap
window latch `SHIFT` without anyone deciding it should be — and from there both
transport keys used to be on their `SHIFT` meanings, so the machine would
neither start nor stop. Hold `SHIFT` for `SHIFT`+`PLAY` and `SHIFT`+`STOP`. Their LEDs are solid
while held and blink while latched.

Two things never latch, both because they edit on release: `TAP` and `DIR`. The
first tap of a double-tap is indistinguishable from an ordinary quick press at
the moment it is let go, so latching them would shift the pattern twice on the
way in.

`CLEAR` has one deliberate hole: **erasing under the playhead, and `CLEAR`+`PTRN`,
still need a physical hold.** A latch may arm a destructive chord but must not
sustain a destructive act, and a pattern wiped by a latch nobody remembered
setting is the one outcome no blinking LED makes acceptable.

### Which pad means which voice

The two hi-hat pads are the one place a pad pair is two different voices rather
than one voice twice. What they mean depends on what the pads are doing:

| The pads are… | Open hat is | Where |
|---|---|---|
| **playing or erasing** a voice | **both pads together**, always | tap record, the roll, `CLEAR`+pads, `CLEAR`+`INST`+pads |

Where a pad **plays or erases**, the chord is not a convention to pick — it is
the gesture itself. It is the chord you play an open hat with, and reading it as
two closed hats would be reading the hand wrong. That never changes.

Where a pad only **names** a voice, both answers are defensible and the choice
is a habit, so `oh ` on config page 2 decides:

| `oh ` | Open hat is |
|---|---|
| `2nd` *(default)* | the second hat pad on its own — one press, like the other ten voices |
| `BTH` | both pads together, the same gesture as everywhere else |

`2nd` is the default because ten of eleven voices already take one press, and it
sidesteps an artifact `BTH` cannot avoid: two pads never land in the same scan,
so under `BTH` the first pad down briefly names the *closed* hat before the
chord completes. On a page that only highlights a voice that is a flicker; it is
the same mechanism that used to delete hi-hat steps.

**The mute row is not affected either way.** It has always had one pad per lane,
and `mte` on config page 2 already says whether the two travel together.

The instrument-select LEDs follow the setting, so the pads always show which
press would get you where you are: one pad each under `2nd`, both under `BTH`.

### The encoder

Turns to edit, presses to move the field cursor on any page that has one. What it
edits depends on where you are; the page's top line shows four field names with
the active one in **upper case**.

Without a page open the encoder is the tempo. `SHIFT`+encoder sets tenths of a
BPM.

### The LEDs

The 16 step LEDs show whatever is most useful where you are: the selected
instrument's pattern in step mode, the pattern number in play mode, mutes in mute
mode, a menu while a modifier is held, a preview on the generator pages. The
playhead blinks through whichever of those is showing.

---

## 3. Transport

| Keys | Does |
|---|---|
| `PLAY` | start from step 1 |
| `STOP` | stop; again to continue from where it stopped |
| `SHIFT`+`PLAY` | arm / disarm tap recording. The `TAP` LED blinks when armed |
| `INST`+`PLAY` | **nothing** — blocked, so a hand reaching for an `INST` chord cannot start the sequencer |
| `CLEAR`+`PTRN` | clear the whole pattern, running or stopped |
| `TEMPO` held | show tempo; encoder sets BPM; **pads become a live trigger monitor** |
| `SHIFT`+encoder | set BPM in tenths |
| `TEMPO` tapped ×4 | tap tempo (master sync only) |
| `GUIDE` tapped | metronome on/off |

Tempo is 30.0–250.0 BPM. Under external MIDI clock the tempo control does
nothing — the rate is measured from the clock.

**The trigger monitor.** Hold `TEMPO` on its own and the sixteen pads stop being
one lane's steps and become every voice at once, each lighting as it fires — the
same pad layout as the mute row, so a voice's two pads both light for it. Muted
voices stay dark, because they are not triggering.

Everything that fires shows: the sequencer, a roll, a repeat, a pad you play by
hand, a note arriving on MIDI in. It is the answer to "what is actually playing",
which the step LEDs cannot give — they show one lane's sixteen steps, which is
right while you are editing that lane and no answer at all once the lanes have
their own lengths and have parted company with the bar and with each other.

`SHIFT`+`TEMPO` and `INST`+`TEMPO` are unaffected: those are the config pages and
the fill page, and both want the pads for themselves.

---

## 4. The seven modes

### Pattern step — `PTRN`

```
┌────────────────┐
│ BD lan07     12│   instrument · this lane's own length · state
│A01  120.0 len16│
└────────────────┘
```

`lan--` means the lane simply follows the pattern. The instrument's second letter
drops to lower case when flam is armed: `Bd`.

Write a pattern a step at a time. The pads are the sixteen steps of the selected
instrument. Stopped, the pads select patterns instead; running, they edit.

### Pattern play — `PTRN` again

```
┌────────────────┐
│INTRO        F03│   the name, and the state zone
│A01  120.0 len16│   bank+slot · tempo · pattern length
└────────────────┘
```

The name gets the whole top line, because it is the one thing on this screen that
is not also on a LED somewhere.

**In slave sync the tempo is the one arriving**, measured from the incoming clock
and marked with an `S` in the cell in front of it:

```
┌────────────────┐
│A01 S120.0 len16│   following a master at 120.0
│A01 S  --- len16│   slave, but no clock arriving
└────────────────┘
```

It is averaged over a quarter note before being shown — a single clock interval
carries every bit of jitter the sender and the cable put into it, and a readout
flickering between 119.4 and 120.7 tells you less than none. Dashes rather than a
frozen number when the clock stops, because a frozen reading and a live one look
identical. `TEMPO` held shows the same thing, with the `S` in place of its dash.

Play patterns and switch between them. The pads select a pattern within the
current bank; two pads together select a *group* that plays in sequence.


**A pad only changes the pattern when nothing else is held.** `SHIFT`, `INST`,
`SHUF`, `LASTSTEP`, `SCALE`, `NUM`, `TEMPO`, `TRK`, `PTRN` and `ENTER` all block
it, and so does any page or held chord. In step mode those keys have pad meanings
of their own; in play mode they have none, so a pad under them used to fall
straight through to a pattern change — and that is the one action here you cannot
take back, because the pattern you were playing has already left the buffer by the
time you notice. `BANK`+pad still selects a bank, which is what `BANK` is for.

### Pattern tap — `SHIFT`+`TAP`

```
┌────────────────┐
│ BD rolx4REC    │   roll rate replaces lane length; REC when armed
│A01  120.0 len16│
└────────────────┘
```

Play the instruments live on the pads. Recording is separate: `SHIFT`+`PLAY` arms
it, and the `TAP` LED blinks while armed. Notes land on the nearest step.

### Track write — `SHIFT`+`TRK`

```
┌────────────────┐
│Pos ptr len num │
│1    A1  0    1 │   position · pattern there · track length · track number
└────────────────┘
```

Build a song out of patterns. Each position holds one pattern; `ENTER` released
writes the current pattern at the current position and advances.

### Track play — `TRK`

```
┌────────────────┐
│T01:LIVE SET    │
│pos:1   ptrn:A1 │
└────────────────┘
```

Play a song. The pads select a track.

### Mute — `MUTE`

```
┌────────────────┐
│MUTE      m03   │   three voices muted, nothing soloed
│A01  120.0 len16│
└────────────────┘

┌────────────────┐
│MUTE SOLO m03   │   …and solo engaged
└────────────────┘
```

`SOLO` earns its place because it is a mask that outlives the gesture that set
it: a voice missing because something else is soloed looks exactly like a voice
that is broken. The number is how many voices are down — *which* ones is on the
step LEDs, and a count is what you check when something is missing and you
cannot say what.

**Tap `MUTE` and it latches; hold it and it does not.** Letting go of a held
`MUTE` puts you back in the mode you came from, so punching a voice out for a bar
is one gesture rather than three — and mute mode stops being somewhere you can be
left standing without noticing.

“Held” is about a third of a second, not the full second the panel's other holds
use. Those are confirmations, where deliberateness is the point; this one is a
performance gesture, and a momentary mute you have to lean on is not momentary.

```
┌────────────────┐
│MUTE SOLO m03 04│   solo engaged · three voices muted · state zone
│A01  120.0 len16│
└────────────────┘
```

The pads mute and unmute instruments. One sentence, three modifiers: a **pad**
toggles one mute, **`INST`** held collects a group that changes together when you
let go, **`CLEAR`** held + a pad solos that voice. The modifier says what kind of
change and the pad says which voice.

Solo is a mask — several voices can be soloed at once, and pressing a soloed one
again drops it. Soloed voices blink on the step LEDs. `CLEAR`+`MUTE` solos the
*selected* instrument from any mode.

`mte` on config page 2 decides whether the two hi-hat lanes move together for
single mutes, for solo, and for `CLEAR`+`INST`+pads instrument delete. **A group
always keeps them separate**, whatever the setting says — a group is assembled before it is applied, so it is the one place
where naming a single hi-hat lane costs nothing if you change your mind.

A page opened from mute mode owns the pads and the encoder press — the mute
handling stands aside while it is up. Otherwise the encoder press clears every
mute *and* every solo — it is the same question,
which voices can be heard, and leaving one of the two standing is how you end up
hunting for why the machine is still nearly silent.

### Keyboard — `NUM` with EXT selected

```
┌────────────────┐
│Idx not len oct │
│07  C#3 08  C3  │
└────────────────┘
```

Type the external instrument's note stack, and hear it as you type. See
§5 for the whole external-instrument workflow — it is the one feature here
that needs three steps in the right order before it makes a sound.

---

## 5. Instruments

`INST` + a pad selects an instrument. The pads are in low/high pairs — pressing
either one of a pair selects that voice.

| | |
|---|---|
| `INST`+pad | select |
| `INST`+ both pads of a pair | arm flam — BD, SD, LT, MT, HT only |
| `INST`+pads 13+14 | select OH (open hat) |
| `INST`+`ENTER` | total accent |
| `INST`+`STOP` | trigger out |
| `SHIFT`+`GUIDE` | external instrument |

### Total accent — `INST`+`ENTER`

```
┌────────────────┐
│Total accent  07│
│███████░░░░░░   │
└────────────────┘
```

`INST`+`ENTER` selects total accent and the page stays up; the encoder sets the
value, in step mode and in tap.

Shuffle, flam and total accent all read the same way: a bar on a track, solid up
to the value and light beyond it. Three pages that each set one number had three
different ways of drawing it. The glyphs are the firmware's own rather than the
display's character ROM — the older bars were built from characters 161 and 219,
which are a dotted track and a solid block only on some panels, and a corner dot
and an outlined square on others.

### The hi-hat

CH and OH are **one physical voice**. The trigger fires it and a second line
picks the decay, so at a step where both lanes carry a note only one can sound.

This firmware keeps **both** lanes' data. Stock firmware deleted the loser's
note, which is why editing one hi-hat lane used to destroy work in the other.
Here nothing is deleted and `hhP` on config page 2 decides which is heard —
flip it and the other lane comes straight back.

### The external instrument — sending notes out

`EXT` is a twelfth lane that sends **MIDI notes on the TX channel** instead of
firing a voice. It plays a *stack* of up to 100 notes: the lane says **when** a
note happens, the stack says **which**, and each trig takes the next note in the
stack and wraps at the end. So a four-note stack against trigs on 1, 5, 9 and 13
is an arpeggio, and the same stack against one trig a bar is a chord progression
a bar at a time.

It needs three steps, in this order, and it makes no sound until all three are
done — which is the usual reason it appears not to work:

1. **Select it.** `SHIFT`+`GUIDE`. The display shows `EXT`.
2. **Give the lane trigs.** In pattern step mode, the pads write the EXT lane
   exactly as they write any other instrument. *No trigs, no notes* — this is
   the step people miss, because the stack can be full and the lane still empty.
3. **Type the stack.** Press `NUM` to enter keyboard mode (EXT selected, pattern
   step mode). The sequencer stops, the pads become a keyboard, and each pad you
   press appends its note to the stack and sounds it. `NUM` again to leave.

In keyboard mode:

| Key | Does |
|---|---|
| `SCALE` / `LASTSTEP` | octave up / down |
| track `◀` / `▶` | step through the stack, playing each note as you land on it |
| pad | write a note at the cursor and move on |
| `CLEAR` | back to the first note |
| `CLEAR` **held** | empty the stack — the screen says `EXT STACK EMPTY` |

`LASTSTEP` does not latch while keyboard mode is open. Two quick presses down two
octaves are indistinguishable from a double tap, and a latched `LASTSTEP` turns
every pad into a pattern-length control while you are trying to type notes.

The stack keys are the **track transport arrows**, not `<<` and `>>` — earlier
editions of this manual said `<<`/`>>` and were wrong. While keyboard mode is
open those keys no longer shift the pattern either; they belong to the stack
until you leave.

**Pad 1 is always C of the current octave** and the sixteen pads run up from it,
which is what the `oct` field shows — `C3`, not an offset to convert.

**An empty slot plays nothing either.** A slot that was never written holds zero,
and zero was being sent as a note — note 12 once the octave offset is added — so
any gap in a stack came out as a stream of the same low note. The cost is that a
stack cannot hold pad 1 at octave 0: that is MIDI note 12, a C five octaves below
middle C, and the alternative was a stored-format change that would have broken
compatibility with stock firmware.

**An empty stack plays nothing.** It used to play note 12 on every trig — `extLength`
is the index of the last note, so 0 means “one note in slot 0”, and an emptied
stack is written exactly the same way. Slot 0 then held a zero, which is a real
note as far as the sender was concerned. An empty stack is now read as empty, and
the first note you type after a wipe goes into slot 0 rather than slot 1.

**The cursor stops at the end of the stack**, plus one slot for appending. It used
to run to the end of the *array*, so walking forward left your notes behind and
read slots that had never been written — and they play. That is why the cursor
and playback disagreed: playback was reading the right slots all along.

**The stack is as long as the furthest note you wrote**, not as far as you walked
the cursor. Stepping out past the end to look at something and then entering one
note used to stretch the stack to cover every slot you had passed — and an
unwritten slot plays as note 12, so the lane sent that over and over while the
keyboard page showed the notes you meant. If a stack has picked up strays, hold
`CLEAR` to empty it and type it again.

The notes are pattern data: they are saved with the pattern by `ENTER`, and they
travel with a copy and paste only if `X` is on in the paste mask (§9).

The TX channel is set on config page 4. If the stack sounds nothing, check that
first and the lane's trigs second.

### Flam

Flam is a second hit a few milliseconds after the first, and belongs only to BD,
SD, LT, MT and HT — the five voices with a low/high pad pair whose double-push
arms it. Arm it with `INST`+both pads of a pair; the instrument name shows its
second letter in lower case while armed. The interval is set on the `SHUF` page.

---

## 6. Writing a pattern

### Step mode

| Keys | Does |
|---|---|
| pad | cycle the step: weak → strong → off |
| pad, with flam armed | weak flam → strong flam → off |
| pad, in single-trigger mode | on → off, always full velocity |

Single-trigger mode (`sTr`, config page 1) makes every instrument two-state,
which is faster for entering a part live. OH, RIDE, CRASH, total accent, trigger
out and EXT are always two-state.

### Tap mode

Pads play. With recording armed they also write, quantised to the nearest step.

**Rolls.** Hold a pad and the instrument repeats. The rate is on the encoder
while a pad is down — the one moment in tap mode when a hand is free and tempo is
the last thing you want — and it shows in the display's third column. `--` is
off, which is the default. `x1` to `x8` are hits per *step*, so a roll follows the
scale; `x3` and `x6` are the triplet settings against a straight one. Rolls ride
the shuffle, several pads can roll at once, and each repeats at the velocity of
the pad that is down.

With recording armed a roll records, quantised — rates finer than one per step
fill the steps they pass over, because the pattern holds one note per step.

### Insert and delete a step

Hold `TAP` or `DIR` and press the pad where the cut goes. `TAP` pulls the later
steps left over the one you pressed; `DIR` pushes them right and leaves a blank.
`INST` narrows it to the selected lane.

A shift rotates a lane and keeps everything; these change how many steps sit
*between* the notes. Both work inside the lane's own span, so a lane with a
lane length of seven is seven steps long. Notes, flams, velocities and
conditions all travel together.

### Shifting

| Keys | Does |
|---|---|
| `TAP` | shift the whole pattern left |
| `DIR` | shift the whole pattern right |
| `INST`+ either | shift only the selected instrument |

---

## 7. Length, scale and feel

### Shuffle, flam and swing — `SHUF` held

```
┌────────────────┐
│Shuf███░░░░  BD3│   pattern shuffle (pads 1-7) · the instrument's swing
│Flam████░░░░   3│   flam (pads 9-16)
└────────────────┘
```

The two values with pads behind them get the bars. Swing is the digit at the top
right, next to the instrument it belongs to and next to the encoder that sets it;
a dash there means "follow the pattern".

Each bar is as wide as the row of pads that sets it — seven cells for shuffle on
pads 1–7, eight for flam on pads 9–16 — so the cell count can be read straight
off the panel. Shuffle has no pad for "none", so an empty shuffle bar is genuinely
no shuffle; flam 0 is the shortest interval rather than the absence of one, and
shows as a single block.

| Keys | Does |
|---|---|
| `LASTSTEP` **held**+pad | set pattern length, and put every lane back on the pattern |
| `INST`+`LASTSTEP` **held**+pad | set that instrument's own loop length |
| `SCALE` | 1/16 → 1/32 → 1/8T → 1/16T |
| `SHUF` held | the shuffle and swing page |

**Per-instrument length.** A lane can run at seven steps against a sixteen-step
pattern and stay there. Set it with `INST`+`LASTSTEP`+pad.

**Shuffle** delays the odd steps. Seven amounts, on pads 1–7 while `SHUF` is
held. **Flam interval** is on pads 9–16 of the same page.

**Swing per instrument.** `SHUF` held + encoder sets the *selected instrument's*
own swing; the page names whose it is. `CLEAR` there puts every instrument back
on the pattern's shuffle. The left-hand end of the bar is "follow the pattern",
which is where all sixteen start — so the page opens saying the pattern is in
charge and you take voices off it one at a time. A swung hat over a straight kick
is two values.

Internally the step now starts on the grid and each instrument's trigger is held
back by its own number of ticks. With no per-instrument value set every
instrument lands together and the result is exactly what a whole-pattern shuffle
always produced.

---

## 8. Clearing and undo

| Keys | Does |
|---|---|
| `CLEAR` held | erase the selected instrument as the playhead passes | 
| `CLEAR`+pads | erase those instruments as the playhead passes (tap mode; stands aside if `INST` joins) |
| `CLEAR`+`INST`+pads | wipe those whole instruments at once, in any mode — applied when the pads come up |
| `CLEAR`+`PTRN` | clear the whole pattern, running or stopped |
| `SHIFT`+`CLEAR` | undo, four levels |
| `SHIFT`+`INST`+`CLEAR` | **redo** |
| `CLEAR` | rewind the track to its first measure (track modes) |

Undo covers one *gesture*, not one step: a held clear, a bar of tap recording, a
paste, a shift or a generator roll each cost one level. It restores the trigs, the
flams, the velocities, the pattern setup, **the lane parameters and the trig
conditions** — so a
polyrhythm you rolled or shortened, or a set of conditions you rolled, comes back
with the rest of the edit rather than after it. `INST`+`LASTSTEP`+pad takes a level of its own, one per pad press. The display
shows `UNDO n/4`, or `NOTHING TO UNDO` when the stack is empty. Undo is discarded
when the pattern changes — the snapshots describe the pattern being left, and
restoring one into a different pattern would be corruption dressed up as a
feature.

**Redo is `SHIFT`+`INST`+`CLEAR`** — the undo grip with one finger moved, since
all three keys are in the same block under the left hand. It walks the same four
levels the other way and shows `REDO n/4`.

There is no second stack. Undo *exchanges* the snapshot with the live pattern
instead of overwriting it, so the state you just left is sitting in the slot you
took the old one from and redo simply swaps it back. Undo and redo therefore share
the four levels between them: three undos leave three redos.

**A new edit ends the redo chain.** The states it led to describe a pattern that no
longer exists, and offering to walk forward into them would be undo's own failure
mode — putting back almost the right thing — under a new name.

### What `CLEAR` resets, page by page

**On a page, `CLEAR` resets the subject of that page** — not the pattern, and not
the page you are on. It is the panel's "put this back" key, so it never needs a
confirmation and never closes anything.

| Page | `CLEAR` gives you | Undo level? |
|---|---|---|
| **Shuffle and swing** `SHUF` held | **every** instrument back on the pattern's shuffle. Not the shuffle or flam values themselves — those have a pad for every setting | yes |
| **Conditions** `INST`+`GUIDE` | the **selected slot only** — `ALL`, or the one step — both cycle and probability back to `---` | yes |
| **Links** `INST`+`GUIDE` twice | the selected instrument's rule: target `---`, rule `---` | yes |
| **Utilities** `INST`+`PTRN` | `op CLR`, `frm 001`, `to 001`, `dst ---`. Nothing is executed | no |
| **Browser** `INST`+`NUM` | the cursor jumps back to the entry actually playing | no |
| **Name editor** `SHIFT`+`ENTER` | all eleven characters blank, caret to the first | no |
| **Keyboard** `NUM` with EXT | tapped: cursor to slot 1. **Held**: the whole stack emptied, screen says `EXT STACK EMPTY` | held: yes |
| **Track play** `TRK` | rewinds to the track's first measure | no |
| **Config** `SHIFT`+`TEMPO` | **nothing.** Settings are changed one field at a time and saved with `ENTER` | — |

**The roll rate has no reset either.** It is set by the encoder in tap mode and
lives across patterns like a performance control rather than a pattern setting;
wind it back to `off` by hand. It stops the moment the pads come up and whenever
the transport stops.

### Three ways back

| Gesture | Goes back to |
|---|---|
| `SHIFT`+`CLEAR` | the last four edits, one press each |
| `SHIFT`+`MUTE` (`PASTE`) | whatever you last copied with `SHIFT`+`BANK` (`COPY`) |
| `CLEAR`+`ENTER` | what is actually saved on the chip |

The third is for after ten minutes of live mangling, when the edits are past
counting and the snapshot was taken somewhere you no longer remember. It reloads
the record, which is the only fixed point there is, and shows `[REVERT]`. It
stands aside while a pattern change is queued — the bar line is about to replace
everything anyway.

**Held `CLEAR` erases from the moment you press it**, and gives up for good if
`INST` joins the press. `CLEAR`+`INST`+pads is a chord and `CLEAR` goes down
first, so for the moment before `INST` arrives the machine cannot tell the two
gestures apart — it erases, and then puts the step back when `INST` shows up
promptly, undo level and all. Reach for `INST` slowly, well after the erasing
has started, and it is taken as two separate gestures: the erasing stands.

**Instrument delete waits for the pads to come up**, because `OH` is the chord
13+14 and two pads never arrive in the same scan. Acting on each press would
name `CH` first — wiping the lane you were reaching past — and `OH` second. The
pads are collected while the chord is held and the instruments worked out once,
from all of them, so the gesture costs exactly one undo level.

It follows `mte` on config page 2, the same as a single mute and as solo: with
the hi-hat lanes paired, deleting one deletes both; with them separate, `INST`+
13+14 takes `OH` and leaves `CH` alone. Deleting a lane and silencing one are the
same question asked of the same pair of pads.

### Clearing is not saving

**Every edit on this list, clearing included, happens in RAM only.** `CLEAR`+`PTRN`
empties the pattern you are hearing; the copy on the chip is untouched until you
press `ENTER`. Until you do:

- the `ENTER` LED blinks, which is the panel saying *there is something here that
  is not on the chip*;
- `CLEAR`+`ENTER` puts the saved version back;
- selecting another pattern and returning also brings the saved version back,
  because changing pattern reloads the record — and it does that without warning,
  so an unsaved clear is one pad press away from being undone by accident.

That cuts both ways. A pattern you cleared and did not save is still on the chip,
so it will come back; a pattern you cleared and *did* save is gone, and only a
SysEx backup will bring it back.

The one exception is **`CLR` in the pattern utilities** (§9), which writes the
EEPROM directly, at once, across a whole range. No `ENTER`, no undo, no going
back. That is why it asks for a one-second hold, and why this manual keeps
telling you to take a dump first.

Undo does **not** cover the pattern utilities. Those are multi-record EEPROM
rewrites; the `ENTER`-hold is their confirmation instead.

---

## 9. Patterns, banks and groups

128 patterns in 8 banks of 16.

| Keys | Does |
|---|---|
| pad | select a pattern (stopped in step mode, any time in play mode) |
| two pads together | select a group: first…second, played in sequence |
| `BANK`+pad | select bank A–H |
| `SHIFT`+`BANK` | copy the pattern |
| `SHIFT`+`MUTE` | paste it |
| `ENTER` | save |

### Chains

Two pads together play those patterns in sequence, first to second, round and
round. Select one pattern on its own and the chain ends.

**Changing chains mid-bar is clean.** Pick a new pair while one is running and
the old chain stops where it is — the next bar line starts the new one, with no
stray pattern in between. A change is armed a whole bar before it lands, so the
one already queued has to be thrown away when you change your mind, and it is.

**A chain is not saved.** It lives for as long as you leave it selected and is
gone at power-off — it is a performance choice, like a mute, not a property of
the patterns in it.

It used to be storable: two pads plus `ENTER` wrote each pattern's place in the
block into its record, and selecting any member replayed the whole chain. That
is gone, because it did not work. Every member kept its own copy of where the
chain began, the chain re-derived itself from those copies once a bar, and if
they disagreed — which they did whenever only part of a block had ever been
written — it re-seated differently each time round and played its first pattern
two or three times before moving on. Clearing the patterns fixed it, which was
the tell: the fault was in the stored bytes, not in the chain.

There was a second cost, and it is the reason not to bring the feature back in
the same shape: a saved chain is the one piece of pattern state with no marker
on the panel. A pattern that chains and a pattern that does not looked
identical, so a bank could carry chains nobody remembered making.

Nothing needs cleaning up. The two bytes are zeroed as each record loads, so a
bank made by an older build cleans itself as you use it — and the record format
is unchanged, so patterns still travel both ways with stock firmware.

**When a change lands** is set by `pCh` on config page 1: `SYN` waits for the bar
line, `FRE` changes immediately. The incoming pattern is loaded the moment you
press the pad, so a synced change is a pointer flip on the bar line with no
loading delay eating the first step.

Copy and paste both flash a message showing the mask they are working through:

```
┌────────────────┐
│PASTED   into A │
│T V F S n E U C │
└────────────────┘
```

Upper case is carried, lower case is not. **The pads toggle the mask while the
chord is held**, and the first eight LEDs show it too — lit is carried — so the
one moment you most want to change it is no longer the one moment you could only
read it.

**Let go of `SHIFT` and the second key holds it open.** Keep `BANK` or `MUTE` down
and the mask stays on screen with the pads still live, which gives you a whole
hand back for them — the same thing `INST`+`TEMPO` does for the fill page. Release
that second key to finish.

This is the only editor for the mask; there is no config page for it. **`SHIFT`+
`BANK` is the side-effect-free way in** — a copy only overwrites the copy buffer,
so “hold copy, set the mask, let go” changes nothing else. `SHIFT`+`MUTE` arms a
paste and is not the place to browse.

**A paste happens when you let go**, once, carrying the mask as it stands at that
moment. Press to see what it will bring across, adjust it if you want to, release
to commit. Firing on the press would have applied the mask as it was *before* you
looked at it, which makes the message a report on a decision already taken.

**A copy still happens on the press**, because the mask has no bearing on it: a
copy takes the whole record and the mask is a paste-time filter. There is nothing
about a copy for the pads to change, and so nothing to wait for.

What a paste transfers is set once and then forgotten, and the
failure it produces is silent: the pattern arrives with its notes and none of its
feel and nothing says why. This is that answer, at the moment it matters.

**Selective paste.** Config page 3 chooses which field groups a paste carries —
trigs, velocities, flams, setup, name, external notes, lane parameters,
conditions. The mask is applied at paste time, not copy time, so one copy can be
pasted as several different subsets: take a pattern's feel without its notes, or
its notes without its name.

### Pattern utilities — `INST`+`PTRN`

```
┌────────────────┐
│Op  frm to  dst │
│CLR 001 008 --- │   ENTER held executes: clears patterns 1 to 8
└────────────────┘

┌────────────────┐
│Op  frm to  src │
│PST 009 040 A01 │   overlays the copy of A01 onto patterns 9 to 40,
└────────────────┘   through the paste mask
```

Clear, move, or group-paste ranges of patterns.

| Field | |
|---|---|
| `op` | `CLR`, `MOV` or `PST` |
| `frm` | first pattern |
| `to` | last pattern — the range is inclusive of both ends |
| `dst` | where to move them (`MOV` only) |
| `src` | where the copy buffer came from (`PST` only, read-only) |

Moving `frm` leaves `to` where it is, so the range grows or shrinks against the
end you are not touching.

`ENTER` held executes. These are irreversible EEPROM rewrites, which is why they
ask for a hold and why undo does not cover them. They refuse to run while the
sequencer is playing.

#### Group paste — `PST`

**Copy a pattern first** (`SHIFT`+`BANK`), then `PST` overlays that buffer onto
every pattern in the range — through the same paste mask a single paste uses.
So *“give patterns 9 to 40 the same choke rules”* is one gesture instead of
thirty-two visits to a page.

Set the mask first, from inside the copy or paste chord — hold it and the pads are
the mask. See §9.
Only what the mask selects is written; everything else in each pattern survives,
because this is the same overlay a single paste performs, from the same code.
Turn everything off but `C` and you have moved a set of conditions and links
across thirty-two patterns without touching a note of any of them.

The `src` field shows which pattern the buffer holds, because *“paste what,
exactly”* is the question worth answering before rewriting part of thirty-two
patterns — and it is the one thing about a group paste you cannot see anywhere
else on the panel. With nothing copied it reads `---` and `ENTER` refuses,
saying `COPY A PTRN 1ST`: an empty buffer is all zeroes, and pasting it across a
range would blank every pattern in it.

Chain bytes are **left alone** — they are not in the paste mask, and nothing
reads them any more in any case (§9). Records that
fail their magic check are started from a blank rather than overlaid onto
whatever the bytes happened to be.

**This one is preparation, not performance.** It is stopped-only and
irreversible, like its two neighbours. Take a SysEx dump before a big one.

---

## 10. Tracks

16 tracks, up to 496 pattern positions each.

| Keys | Does |
|---|---|
| `NUM`+pad | select a track (track write) |
| pad | select a track (track play) |
| `ENTER` released | write the current pattern here and advance |
| `ENTER` held | save the track |
| `<<` / `>>` | previous / next position |
| `SHIFT`+`<<` | delete the pattern at this position |
| `SHIFT`+`>>` | insert the current pattern here |
| `SHIFT`+`NUM` | jump to the last measure |
| `CLEAR` | rewind to the first measure |

---

## 11. Naming and browsing

**Scrolling without the browser.** `BANK` held + encoder walks the patterns and
`TRK` held + encoder walks the tracks, both with the same name-and-length preview
on the display and the position on the step LEDs. Nothing loads until you let go
— a record read per detent would turn the knob into a stutter.

**Naming — `SHIFT`+`ENTER`.** Names the pattern, or the track in a track mode.
Eleven characters. The encoder picks a character, the encoder press moves on, and
`<<` / `>>` nudge the cursor. The step LEDs mirror the cursor position, and a
caret sits under the character being edited.

```
┌────────────────┐
│001:INTRO       │
│    ^           │   caret under the character being edited
└────────────────┘
```

**Browsing — `INST`+`NUM`.** One list of patterns and tracks.

| Keys | Does |
|---|---|
| encoder | move through the list |
| `INST`+encoder | jump a bank at a time |
| pad | jump to that entry in the current bank |
| `NUM` | swap between the pattern list and the track list |
| encoder press | select it and close |
| `CLEAR` | go back to the one actually playing |

```
┌────────────────┐   ┌────────────────┐
│Ptn A05: 016stp→│   │Trk T03: 024pos │
│INTRO 2         │   │LIVE SET        │
└────────────────┘   └────────────────┘
```

The length is read alongside the name, off the same page, so it costs nothing the
name did not already cost — and choosing a pattern you have not heard yet was the
browser's whole weakness. `→` marks the entry actually playing.

A `→` marks the entry actually playing, and the step LEDs show where the cursor
is inside its bank with the playing one blinking. Selecting queues the change
exactly as a pad press does.

---

## 13. Conditions and links

`INST`+`GUIDE` walks conditions → links → out.

### Conditions

**Field order is `prb cnd stp ins`** — the two values first, the two addresses
after. Probability is what the page is opened for; the step and the instrument
are addresses rather than values, and **both are on the pads**: a bare pad picks
the step, `INST`+pad picks the instrument. So the cursor is the only way to reach
a value and one of two ways to reach an address.


```
┌────────────────┐
│Prb cnd stp ins │
│--- --- ALL  BD │   step ALL is the instrument as a whole
└────────────────┘
```

Every step carries a **condition** and a **probability**, and both must pass for
the note to sound.

A condition is `X:Y` — play on the Xth of every Y passes of the pattern. `1:2` is
every other bar, `3:4` the third of four, `1:8` one bar in eight. Probability is
independent and applies after it, so `1:4` at 50 % is a note that happens on one
bar in four and only half of those.

**Step `ALL`** is the instrument as a whole. Instrument and step conditions both
have to pass, so a lane can be thinned without touching the steps inside it.

`INST`+pad picks the instrument; a pad alone picks the step. The step LEDs show
which steps carry a condition, with the one being edited blinking — a conditional
note looks identical to any other in the pattern, so this is the only place it is
visible.

### Links

**Field order is `tgt rule src`, and the page opens on the target.** A pad picks
the source instrument — with or without `INST` — so the encoder opens on the one
field the pads cannot reach. Read the row right to left and it is the sentence:
`BD` … `NOT` … `CH`, "BD plays only when CH did not".


```
┌────────────────┐
│Tgt rule src    │
│ CH NOT  BD     │   "BD plays only when CH did not"
└────────────────┘
```

"When may this play, given what that one is doing."

| Rule | |
|---|---|
| `AND` | only when the target sounded on this step |
| `NOT` | only when it did not — **this is selective choke** |
| `PRE` | only when it sounded on the previous step |
| `NPR` | only when it did not |

`NOT` is how choke works between separate analog voices: you cannot cut a note
that has already started, so the answer is not to start it.

---

## 14. Performance

### Solo and mute

| Keys | Does |
|---|---|
| pad in mute mode | toggle that instrument's mute |
| `INST` held + pads, then release | change them all together |
| encoder press | clear every mute |
| `CLEAR`+`MUTE` | solo the selected instrument |

Solo overrides the mutes rather than merging with them, so leaving solo restores
exactly the mute state you had. Soloing CH or OH keeps the hat audible.

### Pattern direction

`SHIFT`+`DIR` in play mode cycles the pattern's own permanent direction: forward,
backward, ping-pong, random.

---

### Config pages

In page order. Upper case on the top line is the field the encoder is on.

```
     1 behaviour            2 hi-hats
┌────────────────┐   ┌────────────────┐
│PCh sTr mut     │   │Hhp mte oh      │
│SYN OFF OFF     │   │OH  C/O 2nd     │
└────────────────┘   └────────────────┘

     3 encoder speed        4 paste mask
┌────────────────┐   ┌────────────────┐
│Rol fil bpm ptn │   │paste transfers │
│x1  x1  /8  x1  │   │T V F S n X E C │
└────────────────┘   └────────────────┘

     5 sync and MIDI        6 SysEx dump
┌────────────────┐   ┌────────────────┐
│Syn bpm mTX mRX │   │SysEx dump      │
│MST 120 1   1   │   │everything  HOLD│
└────────────────┘   └────────────────┘

     7 firmware
┌────────────────┐
│3.0-dev 14Aug26 │
│build f319388   │
└────────────────┘
```

`SHIFT`+`TEMPO` walks pages 1 → 2 → … → 6 → out. Grouped by subject, then
ordered by how often a hand reaches for them mid-set — so the rig setup is at
the back and the firmware page is last of all.

### Page 1 — behaviour

| Field | Values |
|---|---|
| `pCh` | `SYN` pattern change on the bar line, `FRE` immediate |
| `sTr` | single-trigger step edit: `ON` / `OFF` |
| `mut` | `ON` a tapped voice sounds through its own mute, `OFF` it obeys it |
| `cym` | weak and strong on **crash and ride**: `ON` / `OFF` (default `OFF`) |

**`cym` — accent on the two cymbals.** Both have a velocity CV like every other
voice, but their weak level was set one count under their strong one, which is the
same as not having one — so step edit treated them as on/off.

With `cym ON` they cycle **weak → strong → off** in step edit, like the drums.
**Tap stays strong-only**, and not by choice: crash and ride have one pad each
where the drums have a low/high pair, so there is no second pad to play weak with.

Turning it **off puts them back to strong**, immediately in the pattern you are on
and on every pattern as it loads. So the setting means the same thing for a pattern
you wrote last week as for one you write now, and nothing you recorded with it on
can leave a quiet cymbal behind after you switch it off.

Off by default — the stock machine has never had it, and a firmware update that
changed how your existing patterns sound would be the wrong kind of surprise.

**Both cymbals are timed like the hi-hat.** Their level is set at the moment the
trigger fires rather than half a step early, because a cymbal is still ringing when
the next step comes round and moving its level mid-decay is heard as a second
articulation — a flam. The hi-hat needed the same treatment for the same reason.

### Page 2 — the hi-hats

Everything about the two shared-voice lanes, in one place. These used to be
spread over two pages, which is one subject filed under two headings.

| Field | Values |
|---|---|
| `hhP` | which lane wins a shared step: `OH` or `CH` |
| `mte` | mute pairing: `C/O` the lanes mute together, `C\|O` separately |
| `oh ` | which pad names the open hat: `2nd` or `BTH` — see §2 |

`mut` covers the roll as well as the single tap. Before it existed the two
disagreed — a tapped pad ignored the mutes and a roll from that same held pad
obeyed them, which is one finger getting two answers.

There is deliberately no *choke* field beside it. Choke is a sequencer rule,
applied when the step mask is built; a tapped pad goes straight to the voice and
has never passed through it. **Tap play already overrides choke and always has** —
a switch for it would have been a control that does nothing.

#### Cyclic record — `SHIFT`+`STOP`

`SHIFT`+`PLAY` arms recording; **`SHIFT`+`STOP` makes that recording cyclic.**
The tap-mode display says `CYC` instead of `REC` while it is on. `STOP` on its
own still stops — the stop key declines the press while `SHIFT` is down, so the
chord cannot arm the mode and halt the take in the same motion.

It is on a key rather than in the menu because it is a decision you make for the
take you are about to play, not a preference you set once. For the same reason it
is **not saved** — power-on is always plain record.

It changes what a tap means in tap mode with record armed, and nothing else.

**Off** — taps add. Play a voice for eight bars and eight bars of taps pile into
the same sixteen steps.

**On** — the first tap a voice receives in a cycle throws away what that voice
played last cycle; every later tap in the same cycle adds to the new take. So
you replace a part by playing over it, one voice at a time, without reaching for
`CLEAR`.

Stop playing and the last take stays: nothing clears a lane except a tap that is
about to write to it. There is no commit gesture to remember and nothing to undo
if you simply stop. The voices you are not touching are untouched.

It is per voice, not per pattern. Replacing the hats leaves the kick alone even
though both are in the same bar.

**The hi-hats keep out of each other's way.** Tapping the `OH` chord replaces the
open hat and leaves the closed hat alone.

This is harder than it sounds. The open hat is a two-pad chord, and two fingers
never land in the same scan — so the first pad down is indistinguishable from a
closed-hat tap, and by the time the second arrives the closed hat has already
been replaced. Nothing downstream can tell the difference, because as far as the
machine could see at that instant, there was none. So the closed hat's lane is
copied before it is wiped and handed back if the chord completes within about a
sixteenth. A destructive guess, made reversible, rather than a delay on every
hi-hat tap waiting to find out what your hand meant.

### Page 4 — sync and MIDI

| Field | Values |
|---|---|
| `syn` | `MST` master, `SLV` external clock, `EXP` expander |
| `bpm` | default tempo at power-on |
| `mTX` | MIDI transmit channel, 1–16 or `OFF` |
| `mRX` | MIDI receive channel, 1–16 or `OFF` |

**`OFF` means transport only.** No notes and no program change in that direction —
but clock, start, stop, continue and SysEx all keep flowing. So the NAVA can drive
a rig's timing without also playing notes at it, or follow a DAW's clock without
answering its notes, and a pattern dump or restore still works either way.

That split is not arbitrary: it is exactly the line between *channel* messages and
*system* messages, which is the same line the MIDI standard draws.

### Page 5 — SysEx dump

The encoder picks **everything**, **this pattern**, **all tracks** or **setup
only**. `ENTER` **held** sends it — held rather than pressed because a plain
`ENTER` on a config page means "save the setup", and because a full dump blocks
the panel for about forty-six seconds.

### Page 6 — firmware

```
┌────────────────┐
│3.0-dev 14Aug26 │
│build f319388   │
└────────────────┘
```

Version, the date of the commit it was built from, and the commit itself. A
trailing `+` on the build means the working tree had uncommitted changes.

**Nothing here is editable**, and the encoder is deliberately dead on this page
rather than falling through to whatever is behind it.

It is the date of the *commit*, not of the compile: two builds of the same
source should say the same thing, and *how old is this firmware* is a fact about
the source, not about when someone last pressed build.

**Readable while playing.** It is the one page that answers rather than asks,
and the question comes up mid-set — stopping to find out which firmware is on
the chip costs you the take you were checking on.

### What opens while playing

**Pages 1 to 3 and page 6 open while the sequencer is running.** When
a pattern change lands, how far a detent goes, what a tapped pad ignores — these
are all decisions you make *while* playing, and having to stop to reach them is
why they get set once and left; and the firmware page answers a question you
would otherwise have to stop to ask. `SHIFT`+`TEMPO` steps over pages 5 and 6
while running: one reconfigures the clock under a running sequencer and the
other blocks the panel for the best part of a minute. So while playing the
cycle is 1 → 2 → 3 → 4 → 7 → out, and stopping on any page leaves it exactly
where it was.

**Saving waits for the stop.** `ENTER` will not write the setup while playing —
it is a multi-page EEprom write, and the pages you can now reach mid-set are
exactly the ones you would be pressing `ENTER` near. The setting is live
immediately either way; only the trip to the chip waits. The `ENTER` LED keeps
blinking until it happens.

`ENTER` saves the setup from any config page — and **only** the setup. It used to
save the pattern too, or instead of, when the pattern had unsaved edits, which
made a config save look like it worked only sometimes. On a config page that key
means one thing.

---

## 16. Screens with no page of their own

**Every other screen is drawn beside the thing it belongs to** — the
page in §12, the conditions page in §13, each mode's screen in §4, and so on. A
gallery of all of them in one place meant looking a page up twice: once to find
out what it does and again to find out what it looks like. What is left here are
the screens that are not a page at all — boot, the corner of the home line, and
the things the panel says to you rather than shows you.

16×2 throughout. `█` is a filled bar cell, `░` an empty one, `^` the name caret.

Instrument names occupy **three cells and are right-aligned**, so the two-letter
voices show a leading space: `␣BD`, `␣CH`, against `TRG` or `CRH`. Every screen
in this manual is drawn as the panel actually renders it, which is why `BD` often
sits one cell further right than it looks like it should.

### Power-on

```
┌────────────────┐   ┌────────────────┐
│  NAVA Extra 9  │   │firmware 3.0-dev│
│ e-licktronic   │   │build f319388   │
└────────────────┘   └────────────────┘
```

The build stamp is the git commit the firmware was compiled from, so *which
firmware is on this chip* is answerable from the front panel rather than by
inference. A trailing `+` means the source tree had uncommitted changes when it
was built.

The second line reads `BAD RECORD FOUND` instead if any pattern failed its format
check — a blank pattern is substituted rather than playing the bytes as if they
meant something, and silently substituting someone's work deserves saying out
loud.

The three pattern screens and mute share one shape. The top line is **what you
are pointed at**, the bottom line is **where you are**, and the last three cells
of the top line are the **state zone** — see below.

Scale is on the faceplate LEDs and the playhead is on the step LEDs, so neither
is repeated here.

### The state zone

The last three cells of the top line, on the four screens above. Edit pages
need all four of their labels and you are not performing while you are on one.

| Cell | Shows |
|---|---|
| 13 | the latched modifier: `S` shift · `I` inst · `C` clear · `F` shuf · `L` last step · `G` guide · `*` more than one · blank for none |
| 14–15 | the running effect, as its pad number `01`–`16` · blank for none |

These are the two things the panel cannot otherwise tell you. A latch is a mode
you entered on purpose and will forget you are in — its own LED blinks, but only
if you happen to look at that key. An effect has a number, and the number is what
you want when you meant slot 12 and got slot 11.

The tempo no longer needs `TEMPO` held to be seen; it is on the bottom line
always. Holding `TEMPO` still shows it in the modes that do not carry it.

### Transient messages

A message owns the whole display while it lives, so nothing can paint over it.
Most last a second; **copy and paste stay up for as long as their keys are held**,
because what a paste carries is worth reading properly.

```
┌────────────────┐   ┌────────────────┐   ┌────────────────┐
│SAVED           │   │UNDO      3/4   │   │NOTHING TO UNDO │
│A01 INTRO       │   │A01 INTRO       │   │A01 INTRO       │
└────────────────┘   └────────────────┘   └────────────────┘

┌────────────────┐   ┌────────────────┐   ┌────────────────┐
│REVERTED        │   │SETUP SAVED     │   │TRACK SAVED     │
│A01 INTRO       │   │A01 INTRO       │   │A01 INTRO       │
└────────────────┘   └────────────────┘   └────────────────┘

┌────────────────┐   ┌────────────────┐
│COPIED  from A01│   │PASTED  into B03│
│T V F S n E U C │   │T V F S n E U C │
└────────────────┘   └────────────────┘
```

The undo count is levels **still in hand** — "how many more times can I do this"
is what the hand on the key is asking. Copy and paste show the paste mask: upper
case is carried, lower case is not.

### Long operations

```
┌────────────────┐   ┌────────────────┐   ┌────────────────┐
│ SysEx dump...  │   │Working...      │   │Done            │
│  do not power  │   │do not power off│   │                │
└────────────────┘   └────────────────┘   └────────────────┘
```

`STOP FIRST` appears if a utility is asked to run while the sequencer is playing.

### Expander mode

```
┌────────────────┐   ┌────────────────┐   ┌────────────────┐
│    Expander    │   │      GATE      │   │      MUTE      │
│                │   │                │   │                │
└────────────────┘   └────────────────┘   └────────────────┘
```

---

## 17. Boot-time functions

Held from power-on.

| Held at power-on | Does |
|---|---|
| `PLAY`+`STOP` | offers to initialise the EEPROM; then `PLAY`+`ENTER` within 5 s to confirm |
| `STOP`+`ENTER` | EEPROM self-test |
| `ENTER` | TM2 velocity trim screen |

```
┌────────────────┐   ┌────────────────┐
│  init EEprom ? │   │Adjust TM2 until│
│press PLAY/ENTER│   │TP1 is +5V...   │
└────────────────┘   └────────────────┘
```

### EEPROM self-test

```
┌────────────────┐   ┌────────────────┐   ┌────────────────┐
│EEprom test 1/2 │   │EEprom test PASS│   │EEprom FAIL at  │
│do not power off│   │ I2C 400kHz OK  │   │0x00010000 400k…│
└────────────────┘   └────────────────┘   └────────────────┘
```

### Bank format check

Runs automatically at every power-on.

```
┌────────────────┐   ┌────────────────┐   ┌────────────────┐
│Bank is not v2  │   │Converting v1-v2│   │Bank ready      │
│PLAY=conv ENT=nw│   │do not power off│   │format v2       │
└────────────────┘   └────────────────┘   └────────────────┘

┌────────────────┐   ┌────────────────┐   ┌────────────────┐
│Bank header     │   │Resuming conv.  │   │Ptn op cut off  │
│rebuilt         │   │do not power off│   │check patterns  │
└────────────────┘   └────────────────┘   └────────────────┘
```

`Bank header rebuilt` is what a SysEx restore leaves behind — the protocol
carries patterns and setup, not the header, so it is rebuilt rather than offering
to erase a bank that was just successfully restored.

`Ptn op cut off` means a range clear or move lost power part-way. Unlike the
format conversion it cannot be resumed, so it says so plainly instead of carrying
on as though the bank were sound.

---

## 18. MIDI

**Clock.** Master sends; slave follows. Under an external clock the four internal
ticks belonging to each clock byte are **spread across the measured interval**
rather than fired at once, so shuffle, per-instrument swing, the sub-step
The roll works when slaved. A stopped clock
stops the sequencer.

**Notes in — expander mode only.** Incoming notes play the voices directly,
with the note velocity driving the level. This is **the only mode that listens
to notes**: master and slave both disconnect the note handlers outright, so an
incoming note does nothing at all in either. If you are trying to play the NAVA
from a keyboard or a DAW and nothing sounds, that is why — set `syn` to `EXP` on
config page 4.

Notes arrive on the **RX channel**, also config page 4.

| Note | | Voice |
|---|---|---|
| 34 | | `RIM` rim shot |
| 35, 36 | `C1`, `C2` | `BD` bass drum |
| 38, 40 | `D2`, `E2` | `SD` snare |
| 39 | `D#2` | `HCL` hand clap |
| 41 | `F2` | `LT` low tom |
| 42 | `F#2` | `CH` closed hat |
| 45, 47, 48 | `A2`, `B2`, `C3` | `MT` mid tom |
| 46 | `A#2` | `OH` open hat |
| 49 | `C#3` | `CRH` crash |
| 50 | `D3` | `HT` high tom |
| 51 | `D#3` | `RID` ride |
| 60 | `C4` | trigger out |

Most of these are the General MIDI drum map, so a GM drum part will land on the
right voices without editing. **Two are not**: 34 for the rim shot, where GM puts
side stick on 37, and 60 for the trigger output, which GM calls a bongo. Octave
numbering varies between hosts — the note *numbers* are what the firmware
matches, so use those if the names in your DAW disagree.

A note-on with velocity 0 is treated as a note-off, as the specification
requires.

**Notes out.** The `EXT` lane sends notes on the TX channel — see §5.

**Program change selects a pattern** (**3I**). There are 128 patterns and 128
program numbers, so the whole bank is reachable in one message and **bank select
is not used at all** — there is nothing left for it to do. Program `n` is the
`n`th pattern:

| Program | Pattern |
|---|---|
| 0 | A01 |
| 15 | A16 |
| 16 | B01 |
| 127 | H16 |

Some sequencers number programs from 1 rather than 0; on those, subtract one.

It arrives on the **RX channel** set on config page 4, and behaves exactly like
choosing the pattern on the pads — the record is preloaded and `pCh` decides
whether the swap lands immediately or on the bar line, so a synced change from a
DAW is as tight as one from the panel. Choosing a pattern this way ends any
chain, the same as picking one on the pads.

**Track modes ignore it.** The track already says what plays and when, and a
program change arriving in the middle of that would be two hands on one wheel.

**External instrument.** A per-pattern stack of up to 100 notes sent on the TX
channel, edited in keyboard mode.

**SysEx.** Manufacturer ID `0x7D` (non-commercial), device `0x4E`. Page payloads
use 8-bit-to-7-bit packing, 64 raw bytes to 74.

| Command | Direction | Meaning |
|---|---|---|
| `0x01` | in | dump everything |
| `0x02` `n` | in | dump pattern `n` |
| `0x03` | in | identity request |
| `0x10` `n` `p` … | both | pattern `n`, page `p` |
| `0x11` `0` `0` … | both | the setup page |
| `0x12` `n` `p` … | both | track `n`, page `p` |
| `0x13` … | out | identity reply: format, geometry, firmware version |
| `0x7F` | out | end of dump |

Restoring needs nothing on the panel — incoming SysEx is handled in any mode on
any RX channel. Once the transfer has been quiet for half a second the current
pattern and track are reloaded and the display says `SAVED`.

**Firmware update over SysEx is not supported.** On this part the flash-write
routines have to live in the bootloader section, so an updater is a second
program behind changed fuses that can only be installed over ISP in the first
place. Use ISP.

---

## 19. Keeping this file current

This manual is part of the deliverable, not a snapshot. Any change that alters
what the machine *does* — a new gesture, a moved binding, a changed screen, a
different default — updates this file in the same commit, alongside
[05-SHORTCUTS.md](05-SHORTCUTS.md) and
[Feature-request-list.md](Feature-request-list.md).

The screens in §16 are transcribed from the layout code in `src/nava/LCD.inc`.
When a layout moves, the box moves with it.

[07-TEST-PLAN.md](07-TEST-PLAN.md) is the checklist for finding what is still
wrong, organised by where this firmware has actually broken before.
