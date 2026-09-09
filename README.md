# MSS NAVA Firmware 1.0

[![Watch the demo](https://img.youtube.com/vi/A_0uRR-4RT0/hqdefault.jpg)](https://youtu.be/A_0uRR-4RT0)

▶ **[Watch the demo](https://youtu.be/A_0uRR-4RT0)** — everything below:

An alternative firmware for the **E-licktronic NAVA / NAVA Extra 9**, built on
v1.028beta. It fixes a long list of bugs — including the ones behind the
timing jitter and unreproducible glitches and adds new features.

**Target:** ATmega1284 @ 16 MHz · MightyCore, Standard pinout.

> **Your patterns are safe.** The EEPROM pattern format is unchanged from stock.
> Patterns written by this firmware are readable by v1.028beta and the other way
> round. Flashing does *not* touch the pattern EEPROM — but read
> [Before you flash](#before-you-flash) anyway.

---

## What it fixes

**Trigger jitter**
**Stack buffer overflows in the LCD code.**
**Synced pattern changes land on the bar line.**
**No dropped tick when you turn the tempo knob**
**Encoder edits no longer bleed between fields.**
**The hi-hat stops flamming between weak and strong hits.**

…and more, each one commented in place.

## What it adds

| | |
|---|---|
| **Undo and redo** | four levels, `SHIFT`+`CLEAR` and `SHIFT`+`INST`+`CLEAR` |
| **Trig conditions** | play a step on the Xth of every Y bars, and/or at a probability |
| **Links** | "this voice plays only when that one did / did not" — selective choke |
| **Per-lane length** | seven steps against sixteen, and it stays there |
| **Pattern chains** | pick two pads and the range loops |
| **Cyclic record** | play over a bar and it replaces last cycle's part |
| **Per-instrument swing** | a swung hat over a straight kick |
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

## Before you flash

**A flash write chip-erases the part.** Your *patterns* live in a separate I²C
EEPROM that avrdude cannot reach, so they survive — but your *firmware* does not,
and if anything goes wrong you want a way back.

1. **Read your existing firmware out first** and keep the file:
   ```
   avrdude -c stk500v1 -P COM8 -b 19200 -p m1284 -U flash:r:backup_original.hex:i
   ```
2. **Back up your patterns** over SysEx once this firmware is running (see below),
   or with a pattern-EEPROM reader before you start.

Use `-p m1284p` if your part is the ATmega1284**P**.

## Flashing

Pre-built `.hex` files are on the
[Releases](../../releases) page — that is all most people need.

<details>
<summary>Building it yourself</summary>

Install [PlatformIO](https://platformio.org/), then:

```bash
pio run                # compile
pio run -t upload      # compile and flash
```

`upload_protocol` in `platformio.ini` is set for **Arduino-as-ISP on COM8 at
19200 baud**. Change the port to match your programmer.

The build is warning-free under `-Wall`. Please keep it that way — `-Wall` is
what found three of the bugs listed above.
</details>

### Using an Arduino Nano as the ISP programmer

No dedicated programmer needed — a Nano (or any ATmega328 Arduino) works over
its six-pin ICSP header.

1. In the Arduino IDE: **File → Examples → 11.ArduinoISP → ArduinoISP**, and
   upload it to the Nano as a normal sketch. This is what turns the Nano itself
   into an `stk500v1` programmer at 19200 baud — the same protocol the
   `avrdude` commands in this file already assume.
2. **Add a 10 µF electrolytic capacitor between the Nano's `RESET` and `GND`
   pins**, positive leg to `RESET`. Without it, the moment `avrdude` opens the
   serial port the Nano's own bootloader sees the DTR reset pulse and resets
   the Nano itself mid-transfer, instead of holding the *target* in reset.
   Remove the capacitor afterwards if you want to reprogram the Nano.
3. Wire per the [connection table](#connections-arduino-nano--atmega1284)
   below.
4. **Power the NAVA from its own supply**, not from the Nano. Switch it on
   before you run `avrdude` — leave `RESET` disconnected on the NAVA side
   until the wiring is otherwise complete, then connect it last.
5. Confirm the programmer can see the chip before writing anything:
   ```bash
   avrdude -c stk500v1 -P COM8 -b 19200 -p m1284 -v
   ```
   A signature readout (`1E 97 05` for the ATmega1284, `1E 97 06` for the
   **1284P**) means the wiring is good. If it times out, check the reset
   capacitor first — a missing one is the most common cause.

**Power notes**

- The NAVA's panel, LEDs and analogue section draw more current than the
  Nano's 5 V regulator is meant to supply, so **do not** tie the Nano's `5V` to
  the NAVA's `VCC`. Power the NAVA from its own PSU and connect grounds only.
- **A common ground between the Nano and the NAVA is not optional** — the SPI
  signals (`MOSI`/`MISO`/`SCK`) are meaningless without a shared 0 V reference,
  and without it programming will fail intermittently or not at all.
- Programming reads and writes flash directly; it does not go through the
  bootloader, so the NAVA's own firmware does not need to be running or even
  valid for this to work.

#### Connections: Arduino Nano ↔ ATmega1284

Pin numbers below are the physical **DIP-40** package pins, matching the
MightyCore **Standard** pinout this repo builds for (see
[`platformio.ini`](platformio.ini)). If your board uses the TQFP-44 package
instead, the signal names are the same but the pin numbers are not — check
MightyCore's pinout diagram for that package.

| Signal | Arduino Nano pin | ATmega1284 pin (DIP-40) | ATmega1284 signal |
|---|---|---|---|
| MOSI | `D11` | 6 | PB5 (MOSI) |
| MISO | `D12` | 7 | PB6 (MISO) |
| SCK | `D13` | 8 | PB7 (SCK) |
| RESET | `D10` | 9 | RESET |
| GND | `GND` | 11 *(or 31)* | GND |

The ATmega1284 has two `GND` pins (11 and 31) and they are tied together
internally — either one works. Do **not** connect the Nano's `5V`/`VCC` pin to
anything on the NAVA; the table above is deliberately five wires, not six.

### Fuses

If your boot section is empty — which it will be after any ISP write with chip
erase — `BOOTRST=0` makes the CPU execute erased flash and wrap to `0x0000`.
That happens to boot, but only by accident. Point reset at the application:

```bash
# check the programmer talks to the chip first
avrdude -c stk500v1 -P COM8 -b 19200 -p m1284 -v

# BOOTRST=1: hfuse 0xDC -> 0xDD. Nothing else changes.
avrdude -c stk500v1 -P COM8 -b 19200 -p m1284 -U hfuse:w:0xDD:m
```

Reversible: write `0xDC` back if you ever obtain the original SysEx bootloader.

| Fuse | Stock | Meaning |
|---|---|---|
| lfuse | `0xC7` | external crystal, CKDIV8 off |
| hfuse | `0xDC` → `0xDD` | SPIEN on, JTAG off, BOOTRST |
| efuse | `0xFD` | BOD 2.7 V |

---

## Pattern backup over SysEx

The firmware speaks `F0 7D 4E <cmd> … F7`. `7D` is the MIDI
non-commercial manufacturer ID, so it cannot collide with real gear.

```bash
# build the "dump everything" request, send it, capture the reply (~143 KB, ~46 s)
python tools/sysex_librarian.py request req.syx
amidi -p hw:1,0,0 -s req.syx
amidi -p hw:1,0,0 -d -t 60 > dump.syx
```

Uploading in simple terms: When uploading your pattern bank to Nava via sysex, set in your
preferred midi sysex program's settings - **Dump upload pause interval: 30ms**

⚠ **A restore has to be paced.** It is ~1,800 messages, and a USB MIDI interface
takes them from the host about fifteen times faster than its DIN output can clock
them out at 31250 baud. It buffers the difference until it runs out and then drops
the rest — which looks like a restore that writes the first few patterns and stops.

```bash
# Windows: paced to the wire rate, no third-party modules needed
python tools/sysex_librarian.py ports
python tools/sysex_librarian.py send restore.syx --port 1
```

`amidi` paces correctly on Linux. In a GUI librarian, find the SysEx delay setting
— MIDI-OX has one under SysEx View → Configure, as a delay after F7. Anything with
no pacing control will drop a transfer this size.

After a restore the unit reloads the current pattern once the transfer has been
quiet for 500 ms.
---

## Reporting a problem

Open an [issue](../../issues) and please include:

- what you pressed, in order if possible, and what happened
- whether the sequencer was running, and at what tempo and scale
- master or slave sync
- the build stamp — config page 5 shows it

The sequence of presses is what makes it fixable.

---

## Credits and licence

Original NAVA firmware © **E-licktronic**. This is a derivative work of
v1.028beta, which itself carries community contributions — notably **[zabox]**,
whose in-source annotations are preserved throughout.

This firmware adds bug fixes, timing work and new features. It is published for
other NAVA owners to use. **It carries no licence of its own and grants no rights
over the original code — check E-licktronic's terms before redistributing.** If
you are E-licktronic and want something changed here, say so and it will be
changed.

No warranty. Flashing firmware and writing fuses can brick hardware. You are
responsible for your own backups.
