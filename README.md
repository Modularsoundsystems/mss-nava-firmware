# MSS NAVA Firmware

[![Watch the demo](https://img.youtube.com/vi/A_0uRR-4RT0/hqdefault.jpg)](https://youtu.be/A_0uRR-4RT0)

▶ **[Watch the demo](https://youtu.be/A_0uRR-4RT0)**

⬇ **[Download the latest firmware](../../releases/latest)** — one `.hex` file, no build needed.

A replacement firmware for the **E-licktronic NAVA / NAVA Extra 9**. It is based
on v1.028beta, fixes a long list of bugs — including the timing jitter and the
random glitches — and adds new features.

**Chip:** ATmega1284 @ 16 MHz · MightyCore, Standard pinout.

> **Your patterns are safe.** The pattern storage format is the same as stock.
> Patterns made with this firmware work on v1.028beta and the other way round.
> Flashing does not touch the pattern memory — but read
> [Before you flash](#before-you-flash) anyway.

---

## What it fixes

- **Trigger timing jitter** that changed with how many voices were playing
- **Memory overflows in the display code** that caused random glitches
- **Pattern changes now land on the bar line** when synced
- **No dropped tick** when you turn the tempo knob while playing
- **Encoder edits stay in their own field** instead of leaking into others
- **The hi-hat no longer flams** between weak and strong hits

…and more. Each fix is explained where it sits in the source.

## What it adds

| | |
|---|---|
| **Undo and redo** | four levels, `SHIFT`+`CLEAR` and `SHIFT`+`INST`+`CLEAR` |
| **Trig conditions** | play a step on the Xth of every Y bars, or at a probability |
| **Links** | "this voice plays only when that one did / did not" |
| **Per-lane length** | e.g. seven steps against sixteen, and it stays there |
| **Pattern chains** | pick two pads and the range loops |
| **Cyclic record** | play over a bar and it replaces last cycle's part |
| **Per-instrument swing** | swing the hats over a straight kick |
| **Flam** | on the five voices that can take it, with its own interval |
| **Total accent** | on a lane of its own |
| **Pattern utilities** | clear, move and paste ranges of patterns |
| **Naming and a browser** | eleven characters, and a list you can read before you commit |
| **Live trigger monitor** | hold `TEMPO` and the pads show every voice as it fires |
| **Cymbal accent** | weak and strong on crash and ride, switchable |
| **SysEx backup** | dump and restore every pattern over MIDI |

## Documentation

- **[MANUAL.md](docs/MANUAL.md)** — everything, in order, with every screen drawn.
- **[SHORTCUTS.md](docs/SHORTCUTS.md)** — the key chart. Print this one.

---

## What to install

You need three things on a fresh computer. Everything is free.

| | What | Why |
|---|---|---|
| 1 | **[Arduino IDE](https://www.arduino.cc/en/software)** | Turns an Arduino Nano into a programmer. It also comes with `avrdude` built in. |
| 2 | **`avrdude`** | The program that actually writes the firmware. Already included with the Arduino IDE — see [Running avrdude](#running-avrdude) below for where it is. |
| 3 | **A USB driver for your Nano** (Windows only, sometimes) | Genuine Nanos work out of the box. Cheap clones use a **CH340** chip and need [its driver](https://www.wch-ic.com/downloads/CH341SER_EXE.html). If the Nano does not show up as a COM port, this is why. |

Optional:

- **[PlatformIO](https://platformio.org/)** — only if you want to build the firmware from source. Most people just use the ready-made `.hex` file.
- **A MIDI SysEx tool** for backing up patterns — [MIDI-OX](http://www.midiox.com/) on Windows, [SysEx Librarian](https://www.snoize.com/SysExLibrarian/) on Mac, `amidi` on Linux.

### Running avrdude

`avrdude` is a command-line program. You run it from a terminal, and you need to
tell the terminal where it is.

**Where it lives (installed with the Arduino IDE):**

| | Path |
|---|---|
| Windows, Arduino IDE 2.x | `%LOCALAPPDATA%\Arduino15\packages\arduino\tools\avrdude\<version>\bin\avrdude.exe` |
| Windows, Arduino IDE 1.8 | `C:\Program Files (x86)\Arduino\hardware\tools\avr\bin\avrdude.exe` |
| Mac | `~/Library/Arduino15/packages/arduino/tools/avrdude/<version>/bin/avrdude` — or just `brew install avrdude` |
| Linux | `sudo apt install avrdude` (Debian/Ubuntu) or `~/.arduino15/packages/arduino/tools/avrdude/<version>/bin/avrdude` |

`<version>` is a folder like `6.3.0-arduino17` — look inside the `avrdude`
folder and use whatever is there.

**On Windows, the easiest way:** open the `bin` folder above in File Explorer,
click the address bar, type `cmd` and press Enter. A terminal opens in that
folder, and you can type `avrdude` commands directly.

**One thing that catches everyone:** the Arduino IDE's copy of `avrdude` needs
to be told where its config file is, or it fails with *"can't find avrdude.conf"*.
Add `-C` pointing at the `etc` folder next to `bin`:

```
avrdude -C "..\etc\avrdude.conf" -c stk500v1 -P COM8 -b 19200 -p m1284 -v
```

Every `avrdude` command in this file assumes that `-C` is there when you use
the Arduino copy. If you installed `avrdude` on its own (Homebrew, apt, or a
[standalone release](https://github.com/avrdudes/avrdude/releases)), you can
leave `-C` out.

**Finding your COM port (Windows):** open Device Manager → *Ports (COM & LPT)*
and plug the Nano in. Whatever appears is your port — this file uses `COM8` as
an example, yours will probably be different. On Mac and Linux it is something
like `/dev/tty.usbserial-XXXX` or `/dev/ttyUSB0`.

---

## Before you flash

**Flashing erases the whole chip first.** Your *patterns* live on a separate
memory chip and survive — but the *firmware* that is on there now does not, and
if anything goes wrong you want a way back.

1. **Save a copy of the current firmware first** and keep the file somewhere safe:
   ```
   avrdude -c stk500v1 -P COM8 -b 19200 -p m1284 -U flash:r:backup_original.hex:i
   ```
2. **Back up your patterns** over SysEx once this firmware is running (see
   [below](#pattern-backup-over-sysex)), or with a pattern-memory reader before
   you start.

Use `-p m1284p` if your chip says ATmega1284**P** on it.

## Flashing

Ready-made `.hex` files are on the [Releases](../../releases) page. That is all
most people need.

```
avrdude -c stk500v1 -P COM8 -b 19200 -p m1284 -U flash:w:MSS-NAVA-1.0.1.hex:i
```

<details>
<summary>Building it yourself</summary>

Install [PlatformIO](https://platformio.org/), then:

```bash
pio run                # compile
pio run -t upload      # compile and flash
```

`upload_protocol` in `platformio.ini` is set for **Arduino-as-ISP on COM8 at
19200 baud**. Change the port to match yours.

The build has no compiler warnings. Please keep it that way — the warnings are
what found three of the bugs listed above.
</details>

### Finding the programming header inside the case

Open the case. On the main board, find the **ATmega1284** — the big 40-pin chip.
Near it there is a **6-pin programming header**, two rows of three pins, usually
labelled **ICSP** or **ISP** on the board.

This is the standard AVR programming header. Looking at it with the label the
right way up:

```
        ┌─────┬─────┐
 MISO   │  1  │  2  │  VCC
        ├─────┼─────┤
 SCK    │  3  │  4  │  MOSI
        ├─────┼─────┤
 RESET  │  5  │  6  │  GND
        └─────┴─────┘
```

**Pin 1 is marked** — a square solder pad where the others are round, a small
dot or arrow printed next to it, or a notch in the outline. Find that mark
first; everything else follows from it. Wire the five signals per the
[connection table](#connections-arduino-nano--atmega1284) below, and leave
pin 2 (`VCC`) unconnected.

If your board has no header fitted — just six empty holes — you can either
solder one in, or clip directly onto the chip's legs using the ATmega1284 pin
numbers in the same table.

### Using an Arduino Nano as the programmer

You do not need a dedicated programmer. A Nano — or any Arduino with an
ATmega328 — does the job.

1. In the Arduino IDE: **File → Examples → 11.ArduinoISP → ArduinoISP**, and
   upload it to the Nano like any other sketch. This turns the Nano into a
   programmer.
2. **Put a 10 µF capacitor between the Nano's `RESET` and `GND` pins**, the
   `+` leg on `RESET`. Without it, the Nano resets itself the moment `avrdude`
   opens the port, instead of resetting the NAVA. Take it off again if you ever
   want to reprogram the Nano.
3. Wire it up per the table below.
4. **Power the NAVA from its own power supply**, not from the Nano. Switch the
   NAVA on before you run `avrdude`.
5. Check that the programmer can see the chip before writing anything:
   ```bash
   avrdude -c stk500v1 -P COM8 -b 19200 -p m1284 -v
   ```
   If you see `1E 97 05` (ATmega1284) or `1E 97 06` (ATmega1284**P**), the
   wiring is right. If it times out, check the capacitor from step 2 first —
   that is the usual cause.

**About power**

- **Do not connect the Nano's `5V` to the NAVA.** The NAVA draws more current
  than the Nano can supply. Power it from its own supply and share only the
  ground.
- **The ground wire is not optional.** Without a shared ground the data lines
  mean nothing and programming will fail.
- Programming writes the chip directly. The NAVA's current firmware does not
  need to be working for this to succeed.

#### Connections: Arduino Nano ↔ ATmega1284

Chip pin numbers are for the **40-pin DIP** package (the big through-hole chip).
If your board has the small square **TQFP-44** chip instead, the signal names
are the same but the pin numbers are not — use the header instead.

| Signal | Arduino Nano | ICSP header pin | ATmega1284 chip pin | Chip signal |
|---|---|---|---|---|
| MOSI | `D11` | 4 | 6 | PB5 (MOSI) |
| MISO | `D12` | 1 | 7 | PB6 (MISO) |
| SCK | `D13` | 3 | 8 | PB7 (SCK) |
| RESET | `D10` | 5 | 9 | RESET |
| GND | `GND` | 6 | 11 *(or 31)* | GND |

Five wires. **No `VCC` wire** — header pin 2 stays empty. The chip has two
`GND` pins (11 and 31) and they are joined inside; either one works.

### Fuses

After a chip erase the boot section is empty, and the stock fuse setting makes
the chip start there anyway. It still works, but only by luck. One fuse change
makes reset go straight to the firmware:

```bash
# make sure the programmer can see the chip
avrdude -c stk500v1 -P COM8 -b 19200 -p m1284 -v

# change hfuse 0xDC -> 0xDD. Nothing else changes.
avrdude -c stk500v1 -P COM8 -b 19200 -p m1284 -U hfuse:w:0xDD:m
```

You can undo this by writing `0xDC` back.

| Fuse | Stock | Meaning |
|---|---|---|
| lfuse | `0xC7` | external crystal, CKDIV8 off |
| hfuse | `0xDC` → `0xDD` | SPIEN on, JTAG off, BOOTRST |
| efuse | `0xFD` | brown-out at 2.7 V |

---

## Pattern backup over SysEx

The firmware speaks `F0 7D 4E <cmd> … F7`. `7D` is the MIDI "non-commercial"
manufacturer ID, so it cannot clash with real gear.

**The short version:** when you *send* a pattern bank back to the NAVA, set your
SysEx program's delay between messages to **30 ms**. Without it, most of the
transfer is silently lost.

```bash
# build the "dump everything" request, send it, capture the reply (~143 KB, ~46 s)
python tools/sysex_librarian.py request req.syx
amidi -p hw:1,0,0 -s req.syx
amidi -p hw:1,0,0 -d -t 60 > dump.syx
```

**Why the delay matters:** a full restore is about 1,800 messages. A USB MIDI
interface accepts them from the computer far faster than it can send them down
the MIDI cable. It buffers the difference until it runs out, then drops the
rest — and it looks like a restore that writes a few patterns and stops.

```bash
# Windows: paced correctly, no extra modules needed
python tools/sysex_librarian.py ports
python tools/sysex_librarian.py send restore.syx --port 1
```

`amidi` on Linux paces correctly by itself. In MIDI-OX the setting is under
*SysEx View → Configure*, as a delay after F7. Any tool with no delay setting
will drop a transfer this size.

After a restore the NAVA reloads the current pattern by itself once the transfer
has been quiet for half a second.

---

## Enjoy

That is everything. It exists so you spend less time fighting the panel and
more time playing it.

If you make something with it, we would like to hear it. And if you flash it
and it just works, say so too — that is as useful to know as a bug.

---

## Reporting a problem

Open an [issue](../../issues) and include:

- what you pressed, in order, and what happened
- whether the sequencer was running, and at what tempo and scale
- master or slave sync
- the build number — config page 5 shows it

The order of presses is what makes a bug fixable.

---

## Credits and licence

Original NAVA firmware © **E-licktronic**. This is a derivative work of
v1.028beta, which itself includes community contributions — notably from
**[zabox]**, whose notes are kept throughout the source.

This firmware adds bug fixes, timing work and new features, and is published
for other NAVA owners to use. **It has no licence of its own and claims no
rights over the original code — check E-licktronic's terms before sharing it
on.** If you are E-licktronic and want something changed here, say so and it
will be changed.

No warranty. Flashing firmware and writing fuses can damage hardware. You are
responsible for your own backups.
